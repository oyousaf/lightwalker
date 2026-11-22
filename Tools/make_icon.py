"""
Draws the game's app icon: a neon "LW" (pink L, cyan W, like the logo's LIGHT and WALKER) leaning forward on a dark
rounded tile with a pink-to-cyan edge, in the sunset-neon style of the UI. Writes:

  SourceArt/Icon/LW_Icon.png       1024 px master
  Build/Windows/Application.ico    16 to 256 px, used for the packaged LightWalker.exe

Needs Pillow (ComfyUI's Python has it):
  C:\\ComfyUI\\.venv\\Scripts\\python.exe -I Tools/make_icon.py
"""
import os

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
FONT = os.path.join(ROOT, "Content", "Slate", "Fonts", "Audiowide-Regular.ttf")
SIZE = 1024
PINK = (255, 40, 140)
CYAN = (40, 225, 255)


def gradient(size, top, bottom, horizontal=False):
    """A two-colour linear gradient."""
    strip = Image.new("RGB", (256, 1) if horizontal else (1, 256))
    for i in range(256):
        t = i / 255
        strip.putpixel((i, 0) if horizontal else (0, i), tuple(round(a + (b - a) * t) for a, b in zip(top, bottom)))
    return strip.resize(size, Image.BILINEAR)


def rounded_mask(size, inset, radius):
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((inset, inset, size[0] - inset, size[1] - inset), radius, fill=255)
    return mask


def glow(layer, radii):
    """The layer's own light, blurred out at a few radii and added up, as neon tubes do."""
    out = Image.new("RGBA", layer.size, (0, 0, 0, 0))
    for radius, strength in radii:
        blurred = layer.filter(ImageFilter.GaussianBlur(radius))
        if strength != 1:
            r, g, b, a = blurred.split()
            a = a.point(lambda v: min(255, round(v * strength)))
            blurred = Image.merge("RGBA", (r, g, b, a))
        out = Image.alpha_composite(out, blurred)
    return out


def letters():
    """L and W, each in its colour, leaning forward, centred as a pair."""
    font = ImageFont.truetype(FONT, 400)
    layer = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    l_box = draw.textbbox((0, 0), "L", font=font)
    w_box = draw.textbbox((0, 0), "W", font=font)
    gap = 6
    width = (l_box[2] - l_box[0]) + gap + (w_box[2] - w_box[0])
    height = max(l_box[3], w_box[3]) - min(l_box[1], w_box[1])
    x = (SIZE - width) / 2
    y = (SIZE - height) / 2 - min(l_box[1], w_box[1])
    draw.text((x - l_box[0], y), "L", font=font, fill=PINK + (255,))
    draw.text((x + (l_box[2] - l_box[0]) + gap - w_box[0], y), "W", font=font, fill=CYAN + (255,))
    # Lean forward like the logo's italics (shear around the centre).
    shear = 0.2
    return layer.transform(layer.size, Image.AFFINE, (1, shear, -shear * SIZE / 2, 0, 1, 0), Image.BICUBIC)


def main():
    size = (SIZE, SIZE)
    icon = Image.new("RGBA", size, (0, 0, 0, 0))

    # The tile: deep night purple, lit a little from the bottom like the sunset floor.
    tile = gradient(size, (14, 4, 34), (44, 8, 58)).convert("RGBA")
    tile.putalpha(rounded_mask(size, 40, 210))
    icon = Image.alpha_composite(icon, tile)

    # Its neon edge, pink on the left fading to cyan on the right, with a soft glow.
    ring = Image.new("L", size, 0)
    ImageDraw.Draw(ring).rounded_rectangle((64, 64, SIZE - 64, SIZE - 64), 186, outline=255, width=14)
    edge = gradient(size, PINK, CYAN, horizontal=True).convert("RGBA")
    edge.putalpha(ring)
    edge_glow = glow(edge, [(28, 1.4), (10, 1.0)])
    edge_glow.putalpha(ImageChops.multiply(edge_glow.getchannel("A"), rounded_mask(size, 40, 210)))
    icon = Image.alpha_composite(icon, edge_glow)
    icon = Image.alpha_composite(icon, edge)

    # The letters: a wide glow, a tight glow, then a bright core so they read as lit tubes.
    text = letters()
    icon = Image.alpha_composite(icon, glow(text, [(44, 1.6), (16, 1.3)]))
    icon = Image.alpha_composite(icon, text)
    core = text.filter(ImageFilter.MinFilter(9))
    white = Image.new("RGBA", size, (255, 245, 250, 0))
    white.putalpha(core.getchannel("A").point(lambda v: round(v * 0.55)))
    icon = Image.alpha_composite(icon, white.filter(ImageFilter.GaussianBlur(3)))

    master = os.path.join(ROOT, "SourceArt", "Icon", "LW_Icon.png")
    os.makedirs(os.path.dirname(master), exist_ok=True)
    icon.save(master)
    ico = os.path.join(ROOT, "Build", "Windows", "Application.ico")
    os.makedirs(os.path.dirname(ico), exist_ok=True)
    icon.save(ico, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("wrote", master, "and", ico)


main()
