"""
Builds the One-Eyed Liar's model from its concept art (SourceArt/Models/one_eyed_liar.png): a double pyramid with the
art laid onto its faces, the eye swelling out of the front and the cracks sunk in. The two upper faces the art shows are
used as they are; the hidden ones reuse the left face, mirrored where that keeps the edges matching.

  SourceArt/Models/one_eyed_liar.glb   then   Tools/import_models.py  ->  /Game/Enemies/SM_OneEyedLiar

  "C:/Program Files/Blender Foundation/Blender 5.3/blender.exe" -b --factory-startup -P Tools/make_boss_model.py

If the art is replaced, re-measure the corners below (pixels from the top left). An image-to-3D model saved over the .glb
works just as well.
"""
import os

import bpy
import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ART = os.path.join(ROOT, "SourceArt", "Models", "one_eyed_liar.png")
OUT = os.path.join(ROOT, "SourceArt", "Models", "one_eyed_liar.glb")
TEXTURE = 1024
STEPS = 36  # each face is STEPS * STEPS triangles

# In the art: the top, the far left corner, the near corner, the right corner, the bottom, and the iris.
TOP, LEFT, NEAR, RIGHT, BOTTOM = (617, 8), (16, 851), (428, 886), (1238, 858), (621, 1222)
IRIS, EYE = (792, 508), (300, 150)  # centre, and the half-width and half-height of the swell

# The model: half the base, and how far the points reach above and below it.
HALF, ABOVE, BELOW = 1.0, 1.55, 0.7


def blur(image, radius, passes=3):
    for _ in range(passes):
        for axis in (0, 1):
            padded = np.concatenate([np.repeat(image.take([0], axis), radius + 1, axis), image, np.repeat(image.take([-1], axis), radius, axis)], axis)
            total = np.cumsum(padded, axis)
            size = image.shape[axis]
            image = (total.take(range(2 * radius + 1, 2 * radius + 1 + size), axis) - total.take(range(size), axis)) / (2 * radius + 1)
    return image


def read_art():
    image = bpy.data.images.load(ART)
    width, height = image.size
    pixels = np.array(image.pixels[:], np.float32).reshape(height, width, -1)[::-1, :, :3]
    # Grow the art outwards over the white round it, so no white shows along the edges.
    known = pixels.min(axis=2) < 0.82
    # Highlights inside the eye are white too: only the white outside the outline goes.
    outside = np.zeros_like(known)
    for y in range(height):
        columns = np.flatnonzero(known[y])
        if len(columns):
            outside[y, :columns[0] + 3] = outside[y, columns[-1] - 2:] = True
        else:
            outside[y] = True
    known = ~outside
    pixels = pixels.copy()
    pixels[~known] = 0
    for _ in range(48):
        total, count = np.zeros_like(pixels), np.zeros(known.shape)
        for shift, axis in ((1, 0), (-1, 0), (1, 1), (-1, 1)):
            total += np.roll(pixels * known[..., None], shift, axis)
            count += np.roll(known, shift, axis)
        grow = ~known & (count > 0)
        pixels[grow] = total[grow] / count[grow][:, None]
        known |= grow
    return pixels


def make_image(name, pixels, colour):
    image = bpy.data.images.new(name, pixels.shape[1], pixels.shape[0])
    image.colorspace_settings.name = "sRGB" if colour else "Non-Color"
    full = np.ones((*pixels.shape[:2], 4), np.float32)
    full[..., :3] = pixels[::-1]
    image.pixels.foreach_set(full.ravel())
    image.scale(TEXTURE, TEXTURE)
    image.pack()
    return image


def sample(image, points):
    x = np.clip(points[:, 0].round().astype(int), 0, image.shape[1] - 1)
    y = np.clip(points[:, 1].round().astype(int), 0, image.shape[0] - 1)
    return image[y, x]


art = read_art()
size = art.shape[0]
height_map = blur((art @ np.array([0.35, 0.5, 0.15], np.float32)).astype(np.float64), 4)

# A normal map from the art's own light and dark.
fine = blur((art @ np.array([0.35, 0.5, 0.15], np.float32)).astype(np.float64), 1, 2)
down, across = np.gradient(fine)
normal = np.dstack([-across * 5, down * 5, np.ones_like(fine)])
normal /= np.linalg.norm(normal, axis=2, keepdims=True)

T, D = (0, 0, ABOVE), (0, 0, -BELOW)
FL, FR, BR, BL = (-HALF, -HALF, 0), (HALF, -HALF, 0), (HALF, HALF, 0), (-HALF, HALF, 0)
FACES = [  # corners, anticlockwise from outside, and where each lies in the art
    ((T, FL, FR), (TOP, NEAR, RIGHT)),
    ((T, BL, FL), (TOP, LEFT, NEAR)),
    ((T, FR, BR), (TOP, NEAR, LEFT)),
    ((T, BR, BL), (TOP, NEAR, LEFT)),
    ((D, FR, FL), (BOTTOM, RIGHT, NEAR)),
    ((D, FL, BL), (BOTTOM, NEAR, LEFT)),
    ((D, BR, FR), (BOTTOM, LEFT, NEAR)),
    ((D, BL, BR), (BOTTOM, LEFT, NEAR)),
]

vertices, uvs, triangles = [], [], []
for index, (corners, spots) in enumerate(FACES):
    corners, spots = np.array(corners, float), np.array(spots, float)
    spots = spots + (spots.mean(axis=0) - spots) * 0.012  # just inside the art's outline
    weights = np.array([(1 - (i / STEPS), (i - j) / STEPS, j / STEPS) for i in range(STEPS + 1) for j in range(i + 1)])
    points, pixels = weights @ corners, weights @ spots
    outwards = np.cross(corners[1] - corners[0], corners[2] - corners[0])
    outwards /= np.linalg.norm(outwards)

    # Relief fades out at the edges, so neighbouring faces still meet.
    fade = np.clip(weights.min(axis=1) * 7, 0, 1)
    relief = 0.16 * (sample(height_map, pixels) - 0.35)
    if index == 0:
        away = ((pixels[:, 0] - IRIS[0]) / EYE[0]) ** 2 + ((pixels[:, 1] - IRIS[1]) / EYE[1]) ** 2
        relief += 0.2 * np.sqrt(np.clip(1 - away, 0, 1))
    start = len(vertices)
    vertices += (points + outwards * (relief * fade)[:, None]).tolist()
    uvs += np.column_stack([pixels[:, 0] / size, 1 - pixels[:, 1] / size]).tolist()
    for i in range(STEPS):
        row, below = start + i * (i + 1) // 2, start + (i + 1) * (i + 2) // 2
        for j in range(i + 1):
            triangles.append((row + j, below + j, below + j + 1))
            if j < i:
                triangles.append((row + j, below + j + 1, row + j + 1))

bpy.ops.wm.read_homefile(use_empty=True)
mesh = bpy.data.meshes.new("OneEyedLiar")
mesh.from_pydata(vertices, [], triangles)
layer = mesh.uv_layers.new(name="UVMap")
layer.data.foreach_set("uv", np.array([uvs[v] for triangle in triangles for v in triangle], np.float32).ravel())
mesh.polygons.foreach_set("use_smooth", [True] * len(triangles))
thing = bpy.data.objects.new("OneEyedLiar", mesh)
bpy.context.scene.collection.objects.link(thing)

material = bpy.data.materials.new("one_eyed_liar")
material.use_nodes = True
nodes, links = material.node_tree.nodes, material.node_tree.links
shader = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
colour = nodes.new("ShaderNodeTexImage")
colour.image = make_image("one_eyed_liar_colour", art, True)
links.new(colour.outputs["Color"], shader.inputs["Base Color"])
bumps = nodes.new("ShaderNodeTexImage")
bumps.image = make_image("one_eyed_liar_normal", (normal * 0.5 + 0.5).astype(np.float32), False)
mapping = nodes.new("ShaderNodeNormalMap")
links.new(bumps.outputs["Color"], mapping.inputs["Color"])
links.new(mapping.outputs["Normal"], shader.inputs["Normal"])
mesh.materials.append(material)

bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_image_format="AUTO", export_tangents=True)
print(f"wrote {OUT}: {len(triangles)} triangles")
