"""
Builds the game's app icon from Omar's ChatGPT art (SourceArt/Icon/LW_Icon_Source.png: a neon "LW" in the logo's
lettering on a rounded tile, drawn on black). Cuts the tile out so the corners are transparent, keeps the neon edge's
glow as soft light around it, crops it square and writes:

  SourceArt/Icon/LW_Icon.png       1024 px master
  Build/Windows/Application.ico    16 to 256 px, used for the packaged LightWalker.exe

Needs Pillow and NumPy (ComfyUI's Python has both):
  C:\\ComfyUI\\.venv\\Scripts\\python.exe -I Tools/make_icon.py

If the source art is replaced, re-measure TILE (the neon edge's outer bounds) and RADIUS.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
SOURCE = os.path.join(ROOT, "SourceArt", "Icon", "LW_Icon_Source.png")
TILE = (48, 66, 1202, 1185)  # left, top, right, bottom of the neon edge in the source
RADIUS = 240  # measured from where the edge curves
SIZE = 1024


def main():
    art = Image.open(SOURCE).convert("RGB")
    rgb = np.asarray(art, dtype=np.float32) / 255

    # Inside the edge: solid. Outside: only the glow, its brightness as alpha, so it fades into whatever is behind.
    inside = Image.new("L", art.size, 0)
    left, top, right, bottom = TILE
    ImageDraw.Draw(inside).rounded_rectangle((left + 6, top + 6, right - 6, bottom - 6), RADIUS, fill=255)
    inside = np.asarray(inside.filter(ImageFilter.GaussianBlur(1.5)), dtype=np.float32) / 255
    glow = np.clip(rgb.max(axis=2) * 1.15, 0, 1)
    alpha = np.maximum(inside, glow)

    # Un-premultiply the glow (it was drawn over black) so it keeps its colour once it's see-through.
    colour = np.where(alpha[..., None] > 0.004, np.clip(rgb / np.maximum(alpha[..., None], 0.004), 0, 1), 0)
    icon = Image.fromarray((np.dstack([colour, alpha]) * 255).round().astype(np.uint8), "RGBA")

    # Square around the tile, with room for the glow.
    centre_x, centre_y = (left + right) / 2, (top + bottom) / 2
    half = max(right - left, bottom - top) / 2 + 40
    icon = icon.crop((round(centre_x - half), round(centre_y - half), round(centre_x + half), round(centre_y + half)))
    icon = icon.resize((SIZE, SIZE), Image.LANCZOS)

    master = os.path.join(ROOT, "SourceArt", "Icon", "LW_Icon.png")
    icon.save(master)
    ico = os.path.join(ROOT, "Build", "Windows", "Application.ico")
    os.makedirs(os.path.dirname(ico), exist_ok=True)
    icon.save(ico, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("wrote", master, "and", ico)


main()
