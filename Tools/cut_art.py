"""
Cuts the ChatGPT art (SourceArt/UI/Originals) into the images the game uses (SourceArt/UI/Sliced), at the sizes it needs
them, then squeezes every PNG losslessly. Tools/import_ui.py brings the results in as /Game/UI/T_<Name>.

Needs Pillow, NumPy and SciPy (ComfyUI's Python has them), and pyoxipng for the squeeze (skipped if it isn't there):
  C:\\ComfyUI\\.venv\\Scripts\\python.exe Tools/cut_art.py

Every size is a multiple of 4, which BC7 compression needs. SoftGlow.png isn't cut from art (Tools/make_soft_glow.py).
"""
import os

import numpy as np
from PIL import Image
from scipy import ndimage

try:
    import oxipng
except ImportError:
    oxipng = None

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SOURCE = os.path.join(ROOT, "SourceArt", "UI", "Originals")
OUT = os.path.join(ROOT, "SourceArt", "UI", "Sliced")


def load(name):
    """RGBA floats, 0-1. ChatGPT's solid areas come out at alpha 253-254 and its empty ones at 1-3: make them exact."""
    image = np.asarray(Image.open(os.path.join(SOURCE, name + ".png")).convert("RGBA"), np.float32) / 255
    image = image.copy()
    image[..., 3] = np.clip((image[..., 3] - 3 / 255) / (250 / 255), 0, 1)
    return image


def resize(image, width, height):
    """Lanczos on premultiplied colour, so see-through edges don't pick up fringes."""
    alpha = image[..., 3:]
    planes = np.concatenate([image[..., :3] * alpha, alpha], axis=2)
    planes = np.dstack([
        np.asarray(Image.fromarray(planes[..., channel], "F").resize((width, height), Image.LANCZOS)) for channel in range(4)])
    alpha = np.clip(planes[..., 3:], 0, 1)
    colour = np.where(alpha > 1 / 512, np.clip(planes[..., :3] / np.maximum(alpha, 1 / 512), 0, 1), 0)
    return np.concatenate([colour, alpha], axis=2)


def save(name, image, opaque=False):
    pixels = (np.clip(image, 0, 1) * 255).round().astype(np.uint8)
    if opaque:
        picture = Image.fromarray(pixels[..., :3], "RGB")
    else:
        # Colour carries a few pixels past the edge (for filtering and compression), then it's cleared so it costs nothing.
        near = ndimage.binary_dilation(pixels[..., 3] > 0, iterations=4)
        nearest = ndimage.distance_transform_edt(pixels[..., 3] == 0, return_distances=False, return_indices=True)
        pixels[..., :3] = np.where(near[..., None], pixels[nearest[0], nearest[1]][..., :3], 0)
        picture = Image.fromarray(pixels, "RGBA")
    picture.save(os.path.join(OUT, name + ".png"))
    print(f"{name}: {picture.width}x{picture.height}")


def cover(name, width, height):
    """The whole picture scaled to cover, cropped to the middle (a little above it: the horizon sits high)."""
    image = load(name)
    scale = max(width / image.shape[1], height / image.shape[0])
    crop_w, crop_h = round(width / scale), round(height / scale)
    left, top = (image.shape[1] - crop_w) // 2, (image.shape[0] - crop_h) // 2
    save(name, resize(image[top:top + crop_h, left:left + crop_w], width, height), opaque=True)


# ---------------------------------------------------------------------------------------------------------------------
# Menus and HUD

def menus():
    cover("MenuBackground", 1920, 1080)
    cover("MenuBackground_21x9", 2560, 1096)
    cover("MenuBackground_32x9", 3840, 1080)
    cover("Victory", 2560, 1100)
    cover("GameOver", 2560, 1100)

    logo = load("Logo")
    save("Logo", resize(logo[135:605, 36:2136], 1680, 376))
    buttons = load("Buttons")
    for name, top in (("ButtonNormal", 30), ("ButtonHover", 371), ("ButtonPressed", 713)):
        save(name, resize(buttons[top:top + 280, 48:1498], 1160, 224))
    save("Panel", load("Panel"))


def icons():
    grid = load("Icons")
    labels, count = ndimage.label(ndimage.binary_dilation(grid[..., 3] > 0.12, iterations=6))
    boxes = ndimage.find_objects(labels)
    big = [i for i in range(count) if (labels[boxes[i]] == i + 1).sum() > 2000]
    # Each pixel of glow belongs to the icon it's nearest, so neighbours don't bleed in.
    mask = np.isin(labels, [i + 1 for i in big])
    nearest = ndimage.distance_transform_edt(~mask, return_distances=False, return_indices=True)
    owner = labels[nearest[0], nearest[1]]

    names = {(0, 0): "IconLightOrb", (0, 1): "IconSpreadShot", (0, 2): "IconPiercingBeam",
             (1, 0): "IconShield", (1, 1): "IconBook", (1, 2): "IconLife"}
    for i in big:
        rows, columns = boxes[i]
        middle_x, middle_y = (columns.start + columns.stop) / 2, (rows.start + rows.stop) / 2
        name = names[(int(middle_y > 512), min(2, int(middle_x // 512)))]
        half = int(max(columns.stop - columns.start, rows.stop - rows.start) * 0.56)
        padded = np.pad(grid, ((half, half), (half, half), (0, 0)))
        owned = np.pad(owner == i + 1, half)
        left, top = round(middle_x), round(middle_y)  # already shifted by the padding, less half
        icon = padded[top:top + half * 2, left:left + half * 2].copy()
        icon[..., 3] *= owned[top:top + half * 2, left:left + half * 2]
        # The glow fades out before the edge of the picture.
        ramp = np.clip(np.minimum(np.arange(half * 2), np.arange(half * 2)[::-1]) / (half * 0.12), 0, 1)
        ramp = ramp * ramp * (3 - 2 * ramp)
        icon[..., 3] *= ramp[:, None] * ramp[None, :]
        save(name, resize(icon, 256, 256))


# ---------------------------------------------------------------------------------------------------------------------
# The Path

def path():
    gate = load("PathGate")
    save("PathGate", resize(gate, 768, 1152))
    # The same box round both lanterns (they're 752 px apart), so the lit one lies exactly over the other.
    lanterns = load("Lanterns")
    for name, left in (("LanternUnlit", 225), ("LanternLit", 977)):
        save(name, lanterns[12:1012, left:left + 392])
    spikes = load("Spikes")
    save("Spikes", resize(spikes[48:942, 24:1512], 1024, 616))

    # Rock: made to tile by fading each edge into the opposite one.
    rock = load("PathRock")[..., :3]
    blend = 150
    for axis in (0, 1):
        rock = np.moveaxis(rock, axis, 0)
        length = rock.shape[0] - blend
        ramp = np.linspace(0, 1, blend)[:, None, None]
        ramp = ramp * ramp * (3 - 2 * ramp)
        joined = rock[:length].copy()
        joined[:blend] = rock[:blend] * ramp + rock[length:] * (1 - ramp)
        rock = np.moveaxis(joined, 0, axis)
    save("PathRock", resize(np.dstack([rock, np.ones(rock.shape[:2], np.float32)]), 1024, 1024), opaque=True)


# ---------------------------------------------------------------------------------------------------------------------
# The Shield's wall

# The blocks are 1.63 times as wide as they're deep; the art's are nearly square. Bands that hold something with a shape
# of its own (an orb, rivets, an iris) keep it, and the plain stone between takes up the difference.
TILES = {
    # name: (column, row, x bands kept, y bands kept, squash of the y between)
    "BlockShadow": (0, 0, [], [], 0.75),
    "BlockHard": (1, 0, [(30, 70), (165, 200), (290, 330)], [(138, 192)], 0.68),
    "BlockStone": (2, 0, [(40, 80), (165, 205), (285, 325)], [(60, 116), (222, 274)], 0.6),
    "BlockVessel": (3, 0, [(88, 272)], [(72, 254)], 0.5),
    "BlockBook": (0, 1, [(62, 292)], [(60, 280)], 0.4),
    "BlockCage": (1, 1, [(118, 246)], [(105, 205)], 0.6),
    "BlockEye": (2, 1, [(128, 244)], [(90, 228)], 0.5),
    "BlockEyeBroken": (3, 1, [(110, 260)], [(60, 250)], 0.5),
}
TILE_X = (17, 397, 776, 1158)
TILE_Y = (145, 553)
TILE_W, TILE_H = 363, 330
IRIS = (186, 157, 45)  # in the eye's tile: centre and radius
LID = 117  # the upper lid's edge above it


def knots(length, kept, stretch):
    """Where each edge between bands lands when the bands between the kept ones are stretched."""
    points, lands, at, landed = [0], [0], 0, 0
    for start, stop in kept:
        for edge, scale in ((start, stretch), (stop, 1)):
            landed += (edge - at) * scale
            at = edge
            points.append(at)
            lands.append(landed)
    lands.append(landed + (length - at) * stretch)
    points.append(length)
    return np.array(points, float), np.array(lands, float)


def reshape(tile, kept_x, kept_y, squash, aspect, width, height):
    points_y, lands_y = knots(TILE_H, kept_y, squash)
    solid = sum(stop - start for start, stop in kept_x)
    stretch = (aspect * lands_y[-1] - solid) / (TILE_W - solid)
    points_x, lands_x = knots(TILE_W, kept_x, stretch)
    x = np.interp((np.arange(width * 2) + 0.5) / (width * 2) * lands_x[-1], lands_x, points_x) - 0.5
    y = np.interp((np.arange(height * 2) + 0.5) / (height * 2) * lands_y[-1], lands_y, points_y) - 0.5
    grid = np.meshgrid(y, x, indexing="ij")
    big = np.dstack([ndimage.map_coordinates(tile[..., channel], grid, order=1, mode="nearest") for channel in range(4)])
    return resize(big, width, height)


def blocks():
    sheet = load("Blocks")
    for name, (column, row, kept_x, kept_y, squash) in TILES.items():
        left, top = TILE_X[column], TILE_Y[row]
        tile = sheet[top:top + TILE_H, left:left + TILE_W].copy()
        if name == "BlockEye":
            # The iris comes out as its own image, so it can follow Lightwalker; the white is filled in behind it, from
            # under the lid down.
            cx, cy, radius = IRIS
            yy, xx = np.mgrid[:TILE_H, :TILE_W]
            distance = np.hypot(xx - cx, yy - cy)
            size = radius + 6
            iris = tile[cy - size:cy + size, cx - size:cx + size].copy()
            below_lid = np.clip((yy - LID) / 4, 0, 1)
            iris[..., 3] = (np.clip((radius - distance) / 4, 0, 1) * below_lid)[cy - size:cy + size, cx - size:cx + size]
            save("BlockIris", resize(iris, 128, 128))
            filled = tile[..., :3].copy()
            for y in range(LID, cy + radius + 5):
                half = int(np.sqrt(max(0, (radius + 4) ** 2 - (y - cy) ** 2)))
                left, right = cx - half - 2, cx + half + 2
                t = np.linspace(0, 1, right - left + 1)[:, None]
                shade = 0.6 + 0.4 * min(1, (y - LID) / 22)
                filled[y, left:right + 1] = (filled[y, left] * (1 - t) + filled[y, right] * t) * shade
            smooth = np.dstack([ndimage.gaussian_filter(filled[..., channel], (5, 2)) for channel in range(3)])
            hole = (np.clip((radius + 3 - distance) / 4, 0, 1) * np.clip((yy - LID - 1) / 4, 0, 1))[..., None]
            tile[..., :3] = tile[..., :3] * (1 - hole) + smooth * hole
        save(name, reshape(tile, kept_x, kept_y, squash, 186 / 114, 256, 156))
        if name in ("BlockEye", "BlockEyeBroken"):
            save(name.replace("Block", "BlockGreat"), reshape(tile, kept_x, kept_y, 0.35, 786 / 242, 832, 256))


def squeeze():
    """Every PNG in SourceArt, losslessly: the same pixels, less space."""
    if not oxipng:
        print("pyoxipng isn't installed, so the PNGs weren't squeezed")
        return
    before = after = 0
    for folder, _, files in os.walk(os.path.join(ROOT, "SourceArt")):
        for file in files:
            if file.lower().endswith(".png"):
                path = os.path.join(folder, file)
                before += os.path.getsize(path)
                oxipng.optimize(path, level=5, strip=oxipng.StripChunks.safe())
                after += os.path.getsize(path)
    print(f"PNGs: {before / 1e6:.1f} MB to {after / 1e6:.1f} MB")


if __name__ == "__main__":
    menus()
    icons()
    path()
    blocks()
    squeeze()
