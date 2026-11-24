"""
Writes the two soft glows in SourceArt/UI/Sliced that aren't cut from art. Plain Python, no libraries.

  SoftGlow.png   a warm dot of light with a bright core: the motes rising through the menus (tinted per mote)
  EdgeLight.png  a white strip fading to either side: the arena edges' light on the floor (tinted per edge)

  python Tools/make_soft_glow.py   then   Tools/import_ui.py  ->  /Game/UI/T_SoftGlow, T_EdgeLight
"""
import math
import os
import struct
import zlib

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SourceArt", "UI", "Sliced")


def dot(u, v):
    r = math.hypot(u - 0.5, v - 0.5)
    core = 0.9 * math.exp(-(r / 0.07) ** 2)
    bloom = 0.35 * math.exp(-(r / 0.22) ** 2)
    edge = max(0.0, min(1.0, (0.5 - r) / 0.12))  # faded fully out before the edge
    return min(1.0, (core + bloom) * edge)


def strip(_, v):
    r = abs(v - 0.5) * 2
    return (1 - r) ** 2.2


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write(name, width, height, colour, alpha):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            rows += bytes((*colour, round(255 * alpha((x + 0.5) / width, (y + 0.5) / height))))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")
    with open(os.path.join(OUT, name + ".png"), "wb") as file:
        file.write(png)
    print("wrote", name)


write("SoftGlow", 256, 256, (255, 236, 200), dot)  # warm lantern white
write("EdgeLight", 8, 128, (255, 255, 255), strip)
