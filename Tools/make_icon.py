"""
Draws the game's app icon: "LW" sprayed as neon graffiti (pink L, cyan W, like the logo's LIGHT and WALKER) on a
dark concrete tile with a sprayed pink-to-cyan edge. Writes:

  SourceArt/Icon/LW_Icon.png       1024 px master
  Build/Windows/Application.ico    16 to 256 px, used for the packaged LightWalker.exe

The lettering is Permanent Marker (Apache License 2.0, SourceArt/Icon/Fonts). Needs Pillow and NumPy
(ComfyUI's Python has both):
  C:\\ComfyUI\\.venv\\Scripts\\python.exe -I Tools/make_icon.py
"""
import os
import random

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
FONT = os.path.join(ROOT, "SourceArt", "Icon", "Fonts", "PermanentMarker-Regular.ttf")
SIZE = 1024
PINK = (255, 40, 150)
CYAN = (40, 225, 255)
SEED = 7  # fixed, so the drips and spatter come out the same every run

rng = np.random.default_rng(SEED)
random.seed(SEED)


def noise(size, scale, low=0.0, high=1.0):
    """Blotchy value noise from upscaled random pixels, between low and high."""
    small = Image.fromarray((rng.random((max(1, size // scale), max(1, size // scale))) * 255).astype(np.uint8))
    arr = np.asarray(small.resize((size, size), Image.BICUBIC), dtype=np.float32) / 255
    return low + (high - low) * arr


def layer_from(mask, colour, opacity=1.0):
    """A colour layer whose alpha is mask (0..1 float array) times opacity."""
    rgba = np.zeros((SIZE, SIZE, 4), np.uint8)
    rgba[..., :3] = colour
    rgba[..., 3] = np.clip(mask * opacity * 255, 0, 255).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def blur(mask, radius):
    image = Image.fromarray((np.clip(mask, 0, 1) * 255).astype(np.uint8))
    return np.asarray(image.filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32) / 255


def grow(mask, pixels):
    image = Image.fromarray((mask * 255).astype(np.uint8))
    return np.asarray(image.filter(ImageFilter.MaxFilter(pixels | 1)), dtype=np.float32) / 255


def letter_masks():
    """One mask per letter, the pair centred and tilted up a touch like a tag."""
    font = ImageFont.truetype(FONT, 450)
    canvas = Image.new("L", (SIZE * 2, SIZE * 2), 0)
    draw = ImageDraw.Draw(canvas)
    l_box = draw.textbbox((0, 0), "L", font=font)
    w_box = draw.textbbox((0, 0), "W", font=font)
    overlap = 20  # tucked together like a tag, without the letters merging
    width = (l_box[2] - l_box[0]) + (w_box[2] - w_box[0]) - overlap
    top = min(l_box[1], w_box[1])
    height = max(l_box[3], w_box[3]) - top
    x = SIZE - width / 2
    y = SIZE - height / 2 - top
    masks = []
    for letter, box, dx in (("L", l_box, 0), ("W", w_box, (l_box[2] - l_box[0]) - overlap)):
        image = Image.new("L", canvas.size, 0)
        ImageDraw.Draw(image).text((x + dx - box[0], y), letter, font=font, fill=255)
        image = image.rotate(6, Image.BICUBIC, center=(SIZE, SIZE))
        image = image.crop((SIZE // 2, SIZE // 2 + 20, SIZE // 2 + SIZE, SIZE // 2 + SIZE + 20))
        masks.append(np.asarray(image, dtype=np.float32) / 255)
    return masks


def drips(mask, count, colour):
    """Paint running down from the lowest edges of a letter."""
    out = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(out)
    columns = np.where(mask.max(axis=0) > 0.5)[0]
    for _ in range(count):
        x = int(rng.choice(columns))
        y = int(np.where(mask[:, x] > 0.5)[0].max())
        length = int(rng.uniform(40, 150))
        width = int(rng.uniform(9, 16))
        draw.rounded_rectangle((x - width // 2, y - 10, x + width // 2, y + length), width // 2, fill=colour + (255,))
        draw.ellipse((x - width * 0.8, y + length - width, x + width * 0.8, y + length + width * 0.6), fill=colour + (255,))
    return out


def spatter(mask, colour, amount):
    """Overspray: fine specks scattered around a letter."""
    halo = blur(grow(mask, 31), 30) - mask
    specks = (rng.random((SIZE, SIZE)) < halo * amount).astype(np.float32)
    return layer_from(blur(specks, 0.8) * 1.6, colour)


def main():
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    tile_mask = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(tile_mask).rounded_rectangle((40, 40, SIZE - 40, SIZE - 40), 210, fill=255)
    tile_alpha = np.asarray(tile_mask, dtype=np.float32) / 255

    # Dark night concrete: a purple gradient, blotchy patches and fine grit.
    y = np.linspace(0, 1, SIZE, dtype=np.float32)[:, None]
    base = np.zeros((SIZE, SIZE, 3), np.float32)
    base[..., 0] = 16 + 26 * y
    base[..., 1] = 6 + 4 * y
    base[..., 2] = 34 + 22 * y
    blotches = noise(SIZE, 48, 0.75, 1.2) * noise(SIZE, 12, 0.9, 1.1)
    grit = 1 + (rng.random((SIZE, SIZE)) - 0.5) * 0.35
    concrete = np.clip(base * (blotches * grit)[..., None], 0, 255).astype(np.uint8)
    tile = Image.fromarray(np.dstack([concrete, (tile_alpha * 255).astype(np.uint8)]), "RGBA")
    icon = Image.alpha_composite(icon, tile)

    # A sprayed edge, pink on the left fading to cyan on the right, soft and uneven like a can's line.
    ring = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(ring).rounded_rectangle((70, 70, SIZE - 70, SIZE - 70), 180, outline=255, width=18)
    ring = np.asarray(ring.filter(ImageFilter.GaussianBlur(5)), dtype=np.float32) / 255
    ring *= noise(SIZE, 20, 0.55, 1.15)
    across = np.linspace(0, 1, SIZE, dtype=np.float32)[None, :, None]
    edge_colour = (np.array(PINK, np.float32) * (1 - across) + np.array(CYAN, np.float32) * across).repeat(SIZE, 0)
    for radius, strength in ((26, 0.9), (0, 1.0)):
        alpha = (blur(ring, radius) if radius else ring) * strength * tile_alpha
        icon = Image.alpha_composite(icon, Image.fromarray(np.dstack([edge_colour.astype(np.uint8), np.clip(alpha * 255, 0, 255).astype(np.uint8)]), "RGBA"))

    # The tag: each letter gets a neon glow, a dark outline, drips, its fill, overspray and a bright highlight.
    l_mask, w_mask = letter_masks()
    both = np.maximum(l_mask, w_mask)
    for radius, strength in ((60, 0.9), (24, 0.8)):
        for mask, colour in ((l_mask, PINK), (w_mask, CYAN)):
            icon = Image.alpha_composite(icon, layer_from(blur(mask, radius) * tile_alpha, colour, strength))
    icon = Image.alpha_composite(icon, layer_from(grow(both, 23), (8, 2, 16), 0.95))
    for mask, colour, count in ((l_mask, PINK, 3), (w_mask, CYAN, 4)):
        icon = Image.alpha_composite(icon, drips(mask, count, colour))
    # W over L where they overlap, as a tag is painted left to right.
    for mask, colour in ((l_mask, PINK), (w_mask, CYAN)):
        shade = noise(SIZE, 30, 0.82, 1.0)[..., None]
        fill = layer_from(mask, colour)
        fill_rgb = np.asarray(fill, dtype=np.float32)
        fill_rgb[..., :3] *= shade
        icon = Image.alpha_composite(icon, Image.fromarray(fill_rgb.astype(np.uint8), "RGBA"))
        icon = Image.alpha_composite(icon, spatter(mask, colour, 0.05))
        core = np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).filter(ImageFilter.MinFilter(21)), dtype=np.float32) / 255
        icon = Image.alpha_composite(icon, layer_from(blur(core, 6), (255, 246, 252), 0.55))

    master = os.path.join(ROOT, "SourceArt", "Icon", "LW_Icon.png")
    os.makedirs(os.path.dirname(master), exist_ok=True)
    icon.save(master)
    ico = os.path.join(ROOT, "Build", "Windows", "Application.ico")
    os.makedirs(os.path.dirname(ico), exist_ok=True)
    icon.save(ico, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("wrote", master, "and", ico)


main()
