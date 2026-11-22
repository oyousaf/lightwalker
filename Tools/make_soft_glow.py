"""
Writes SourceArt/UI/Sliced/SoftGlow.png: a soft warm dot of light with a bright core, the sprite for the motes of
light rising through the menus (tinted per mote). Plain Python, no libraries.

  python Tools/make_soft_glow.py   then   Tools/import_ui.py  ->  /Game/UI/T_SoftGlow
"""
import math
import os
import struct
import zlib

SIZE = 256
COLOUR = (255, 236, 200)  # warm lantern white
OUT = os.path.join(os.path.dirname(__file__), "..", "SourceArt", "UI", "Sliced", "SoftGlow.png")


def alpha(r):
    # A bright core and a soft bloom, faded fully out before the edge.
    core = 0.9 * math.exp(-(r / 0.07) ** 2)
    bloom = 0.35 * math.exp(-(r / 0.22) ** 2)
    edge = max(0.0, min(1.0, (0.5 - r) / 0.12))
    return min(1.0, (core + bloom) * edge)


rows = bytearray()
for y in range(SIZE):
    rows.append(0)
    for x in range(SIZE):
        r = math.hypot((x + 0.5) / SIZE - 0.5, (y + 0.5) / SIZE - 0.5)
        rows += bytes((*COLOUR, round(255 * alpha(r))))


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 6, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")
with open(OUT, "wb") as f:
    f.write(png)
print("wrote", os.path.normpath(OUT))
