"""
Minimal Standard MIDI File writer (type 1, no dependencies), shared by the compose scripts.

Events are (tick, length, note, velocity) tuples. Overlapping notes on one track are kept as they are,
which FL Studio turns into slides when the channel's Porta (portamento) is on.
"""

import struct

PPQ = 480
STEP = PPQ // 4          # one 16th note
BAR = STEP * 16
TRIPLET_16 = PPQ // 6    # one 16th-note triplet

NOTE_INDEX = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}

# General MIDI drums (FPC's default pad layout)
KICK, RIM, SNARE, CLAP, HAT, OPEN_HAT, CRASH, TOM_LOW, TOM_MID, TOM_HIGH = 36, 37, 38, 39, 42, 46, 49, 45, 47, 50


def note(name):
    """'C#5' -> 73. 'B#' is accepted for harmonic-minor spelling."""
    pitch, octave = name[:-1], int(name[-1])
    if pitch == "B#":
        return 12 * (octave + 2)
    return 12 * (octave + 1) + NOTE_INDEX[pitch]


def var_len(value):
    out = bytearray([value & 0x7F])
    value >>= 7
    while value:
        out.insert(0, (value & 0x7F) | 0x80)
        value >>= 7
    return bytes(out)


def track_chunk(name, events, channel, program=None):
    """program: a General MIDI instrument (0-127), or on the drum channel a kit (e.g. 25 = TR-808 in GS banks)."""
    messages = []
    if program is not None:
        messages.append((0, 0, bytes([0xC0 | channel, program])))
    for tick, length, pitch, velocity in events:
        messages.append((tick, 1, bytes([0x90 | channel, pitch, velocity])))
        messages.append((tick + max(length, 1), 0, bytes([0x80 | channel, pitch, 0])))
    messages.sort(key=lambda m: (m[0], m[1]))

    data = bytearray()
    data += var_len(0) + b"\xFF\x03" + var_len(len(name)) + name.encode()
    last = 0
    for tick, _, message in messages:
        data += var_len(tick - last) + message
        last = tick
    data += var_len(0) + b"\xFF\x2F\x00"
    return b"MTrk" + struct.pack(">I", len(data)) + data


def tempo_chunk(bpm, markers=()):
    """markers: (tick, text) pairs, e.g. loop points."""
    data = bytearray()
    data += var_len(0) + b"\xFF\x51\x03" + (60_000_000 // bpm).to_bytes(3, "big")
    data += var_len(0) + b"\xFF\x58\x04\x04\x02\x18\x08"  # 4/4
    last = 0
    for tick, text in sorted(markers):
        data += var_len(tick - last) + b"\xFF\x06" + var_len(len(text)) + text.encode()
        last = tick
    data += var_len(0) + b"\xFF\x2F\x00"
    return b"MTrk" + struct.pack(">I", len(data)) + data


def write(path, bpm, tracks, markers=()):
    header = b"MThd" + struct.pack(">IHHH", 6, 1, len(tracks) + 1, PPQ)
    with open(path, "wb") as file:
        file.write(header + tempo_chunk(bpm, markers) + b"".join(tracks))
