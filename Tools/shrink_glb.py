"""
Shrinks a generated model's textures to what the game shows: image-to-3D tools write 2048 px maps, but nothing but the
boss is ever more than a couple of hundred pixels on screen. Rewrites each .glb in SourceArt/Models in place with its
colour and normal maps at most SIZE across, and drops the maps the game doesn't use (it has its own roughness).
Run it on a new model before Tools/import_models.py; models already done are left alone.

Needs Pillow:
  C:\\ComfyUI\\.venv\\Scripts\\python.exe Tools/shrink_glb.py
"""
import glob
import io
import json
import os
import struct

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SIZE = 1024


def shrink(path):
    with open(path, "rb") as file:
        data = file.read()
    json_length = struct.unpack_from("<I", data, 12)[0]
    model = json.loads(data[20:20 + json_length])
    binary = data[20 + json_length + 8:]

    # Only the colour and normal maps are used.
    changed = False
    used = {}
    for material in model.get("materials", []):
        pbr = material.get("pbrMetallicRoughness", {})
        changed |= "metallicRoughnessTexture" in pbr
        if "metallicRoughnessTexture" in pbr:
            del pbr["metallicRoughnessTexture"]
            pbr["metallicFactor"], pbr["roughnessFactor"] = 0, 0.55
        for key in ("occlusionTexture", "emissiveTexture"):
            changed |= material.pop(key, None) is not None
        for slot in (pbr.get("baseColorTexture"), material.get("normalTexture")):
            if slot:
                slot["index"] = used.setdefault(slot["index"], len(used))
    textures = [model["textures"][old] for old in used]

    # The images those textures use, re-encoded.
    images, views, chunks, offset = [], [], [], 0
    image_views = {image["bufferView"] for image in model.get("images", [])}

    def add(chunk, view):
        nonlocal offset
        chunk += b"\0" * (-len(chunk) % 4)
        view = {**view, "buffer": 0, "byteOffset": offset, "byteLength": len(chunk)}
        chunks.append(chunk)
        views.append(view)
        offset += len(chunk)
        return len(views) - 1

    def read(view):
        start = view.get("byteOffset", 0)
        return binary[start:start + view["byteLength"]]

    moved = {index: add(read(view), {k: v for k, v in view.items() if k not in ("byteOffset", "byteLength")})
             for index, view in enumerate(model["bufferViews"]) if index not in image_views}
    for accessor in model["accessors"]:
        accessor["bufferView"] = moved[accessor["bufferView"]]
    kept_images = {}
    for texture in textures:
        source = texture.get("source", texture.get("extensions", {}).get("EXT_texture_webp", {}).get("source"))
        if source not in kept_images:
            image = model["images"][source]
            picture = Image.open(io.BytesIO(read(model["bufferViews"][image["bufferView"]])))
            changed |= max(picture.size) > SIZE or picture.format != "PNG"
            picture = picture.convert("RGB")
            picture.thumbnail((SIZE, SIZE), Image.LANCZOS)
            out = io.BytesIO()
            picture.save(out, "PNG", optimize=True)
            kept_images[source] = len(images)
            images.append({"name": image.get("name", f"image_{source}"), "mimeType": "image/png", "bufferView": add(out.getvalue(), {})})
        texture["source"] = kept_images[source]
        texture.pop("extensions", None)
    if not changed:
        print(f"{os.path.basename(path)}: already done")
        return

    model["textures"], model["images"], model["bufferViews"] = textures, images, views
    model["buffers"] = [{"byteLength": offset}]
    for key in ("extensionsUsed", "extensionsRequired"):
        if key in model:
            model[key] = [name for name in model[key] if name != "EXT_texture_webp"]
            if not model[key]:
                del model[key]
    text = json.dumps(model, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    body = b"".join(chunks)
    with open(path, "wb") as file:
        file.write(struct.pack("<III", 0x46546C67, 2, 28 + len(text) + len(body)))
        file.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        file.write(struct.pack("<II", len(body), 0x004E4942) + body)
    print(f"{os.path.basename(path)}: {len(data) / 1e6:.1f} MB to {os.path.getsize(path) / 1e6:.1f} MB")


for model_path in sorted(glob.glob(os.path.join(ROOT, "SourceArt", "Models", "*.glb"))):
    shrink(model_path)
