"""
Writes the menu theme as MIDI, one file per part plus a combined multitrack, for building in FL Studio.

An original tune in the style of early-90s 16-bit platformers (Mega Drive FM / Amiga): bouncy FM slap bass,
off-beat stabs, a 16th-note arpeggio, a hooky lead and punchy drums. 132 BPM, A minor with a lift to C major.

  Bars 1-4    intro (one-shot)
  Bars 5-28   loop: A (8 bars), A again (8), B (8), then back to bar 5

Run with any Python 3 (no packages needed), from the project root:
  python Tools/music/compose_menu_theme.py
Output: SourceArt/Music/MenuTheme/*.mid
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from midi_writer import BAR, PPQ, STEP, note, track_chunk, write  # noqa: E402
from midi_writer import CLAP, CRASH, HAT, KICK, OPEN_HAT, SNARE, TOM_HIGH, TOM_LOW, TOM_MID  # noqa: E402

TEMPO_BPM = 132

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "SourceArt", "Music", "MenuTheme")

# ---------------------------------------------------------------------------------------------------------------
# Harmony

CHORDS = {  # stab voicings (MIDI notes) and bass root
    "Am": ([57, 60, 64], 45),
    "F":  ([57, 60, 65], 41),
    "C":  ([55, 60, 64], 48),
    "G":  ([55, 59, 62], 43),
    "E":  ([56, 59, 64], 40),
    "Em": ([55, 59, 64], 40),
    "Dm": ([57, 62, 65], 38),
}

INTRO = ["Am", "Am", "F", "G"]
SECTION_A = ["Am", "F", "C", "G", "Am", "F", "G", "E"]
SECTION_B = ["F", "G", "Em", "Am", "Dm", "G", "C", "E"]
SONG = INTRO + SECTION_A + SECTION_A + SECTION_B  # 28 bars

# ---------------------------------------------------------------------------------------------------------------
# Lead melody: per bar, (note name, start step, length in steps)

LEAD_A = [
    [("A5", 0, 2), ("A5", 2, 1), ("C6", 3, 3), ("B5", 6, 2), ("A5", 8, 2), ("G5", 10, 2), ("E5", 12, 4)],
    [("F5", 0, 2), ("A5", 2, 2), ("C6", 4, 4), ("D6", 8, 2), ("C6", 10, 2), ("A5", 12, 4)],
    [("G5", 0, 2), ("E5", 2, 1), ("G5", 3, 3), ("C6", 6, 2), ("B5", 8, 2), ("G5", 10, 2), ("E5", 12, 2), ("D5", 14, 2)],
    [("D5", 0, 4), ("G5", 4, 2), ("B5", 6, 2), ("D6", 8, 6)],
    [("A5", 0, 2), ("A5", 2, 1), ("C6", 3, 3), ("B5", 6, 2), ("A5", 8, 2), ("G5", 10, 2), ("E5", 12, 4)],
    [("F5", 0, 2), ("A5", 2, 2), ("C6", 4, 2), ("F6", 6, 2), ("E6", 8, 4), ("D6", 12, 2), ("C6", 14, 2)],
    [("B5", 0, 4), ("D6", 4, 2), ("B5", 6, 2), ("G5", 8, 4), ("A5", 12, 2), ("B5", 14, 2)],
    [("G#5", 0, 4), ("B5", 4, 4), ("E6", 8, 6), ("D6", 14, 2)],
]

LEAD_B = [
    [("C6", 0, 6), ("A5", 6, 2), ("C6", 8, 4), ("F6", 12, 4)],
    [("D6", 0, 6), ("B5", 6, 2), ("D6", 8, 4), ("G6", 12, 4)],
    [("E6", 0, 4), ("D6", 4, 2), ("B5", 6, 2), ("G5", 8, 4), ("B5", 12, 4)],
    [("A5", 0, 8), ("C6", 8, 4), ("E6", 12, 4)],
    [("F6", 0, 4), ("E6", 4, 2), ("D6", 6, 2), ("A5", 8, 4), ("D6", 12, 4)],
    [("D6", 0, 4), ("C6", 4, 2), ("B5", 6, 2), ("G5", 8, 4), ("B5", 12, 4)],
    [("C6", 0, 4), ("E6", 4, 4), ("G6", 8, 4), ("E6", 12, 4)],
    [("G#5", 0, 2), ("B5", 2, 2), ("E6", 4, 2), ("G#6", 6, 2), ("B6", 8, 4), ("G#6", 12, 2), ("E6", 14, 2)],
]



# ---------------------------------------------------------------------------------------------------------------
# Parts: each returns a list of (tick, length, note, velocity)

def lead():
    events = []
    for bar, lines in enumerate(LEAD_A + LEAD_A + LEAD_B):
        base = (len(INTRO) + bar) * BAR
        for name, start, length in lines:
            accent = 112 if start % 4 == 0 else 96
            events.append((base + start * STEP, length * STEP - 10, note(name), accent))
    return events


def bass():
    # Bouncing FM slap: root, octave and fifth in a syncopated 16th pattern.
    pattern = [(0, 2, 0), (2, 1, 12), (3, 1, 0), (4, 2, 12), (6, 2, 0), (8, 2, 0), (10, 1, 12), (11, 1, 0), (12, 2, 7), (14, 2, 12)]
    events = []
    for bar, chord in enumerate(SONG):
        root = CHORDS[chord][1]
        base = bar * BAR
        if bar < 2:  # intro: long roots to set the mood before the bounce kicks in
            events.append((base, BAR - 20, root, 100))
            continue
        for start, length, interval in pattern:
            events.append((base + start * STEP, length * STEP - 15, root + interval, 118 if start % 4 == 0 else 95))
    return events


def stabs():
    # Off-beat chord stabs (the "chk" between beats); the intro holds long pads instead.
    events = []
    for bar, chord in enumerate(SONG):
        voicing = CHORDS[chord][0]
        base = bar * BAR
        if bar < len(INTRO):
            for pitch in voicing:
                events.append((base, BAR - 20, pitch, 80))
            continue
        for start in (2, 6, 10, 14):
            for pitch in voicing:
                events.append((base + start * STEP, STEP, pitch, 90))
    return events


def arp():
    # 16th-note arpeggio over two octaves of the chord, up then down.
    shape = [0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5]
    events = []
    for bar, chord in enumerate(SONG):
        if bar < 2:  # silent for the first two intro bars, then fades in
            continue
        voicing = sorted(CHORDS[chord][0])
        tones = [p + 12 for p in voicing] + [p + 24 for p in voicing]
        base = bar * BAR
        velocity = 60 if bar < len(INTRO) else 78
        for step, index in enumerate(shape):
            events.append((base + step * STEP, STEP - 10, tones[index], velocity + (10 if step % 4 == 0 else 0)))
    return events


def drums():
    events = []
    section_starts = {len(INTRO), len(INTRO) + 8, len(INTRO) + 16}
    for bar in range(len(SONG)):
        base = bar * BAR
        last_of_section = (bar + 1) in section_starts or bar == len(SONG) - 1

        def hit(step, pitch, velocity):
            events.append((base + step * STEP, STEP, pitch, velocity))

        if bar < 2:
            for step in range(0, 16, 4):
                hit(step, HAT, 55)
            continue
        if bar < len(INTRO):
            for step in (0, 4, 8, 12):
                hit(step, KICK, 110)
            for step in range(0, 16, 2):
                hit(step, HAT, 70)
            if bar == len(INTRO) - 1:
                for step in range(8, 16):
                    hit(step, SNARE, 70 + step * 3)
            continue

        if bar in section_starts:
            hit(0, CRASH, 110)
        if last_of_section:
            # Tom fill across the last beat and a half into the next section.
            for step in (0, 6, 8):
                hit(step, KICK, 118)
            hit(4, SNARE, 112)
            for step, tom in zip(range(10, 16), (TOM_HIGH, TOM_HIGH, TOM_MID, TOM_MID, TOM_LOW, TOM_LOW)):
                hit(step, tom, 100)
            continue

        for step in (0, 6, 8, 11):
            hit(step, KICK, 120 if step in (0, 8) else 100)
        hit(4, SNARE, 115)
        hit(12, SNARE, 115)
        hit(12, CLAP, 90)
        for step in range(0, 16, 2):
            hit(step, OPEN_HAT if step == 14 else HAT, 85 if step % 4 == 0 else 65)
    return events


MARKERS = [(len(INTRO) * BAR, "Loop start"), (len(SONG) * BAR, "Loop end")]

PARTS = [
    ("Lead", lead, 0),
    ("Bass", bass, 1),
    ("Stabs", stabs, 2),
    ("Arp", arp, 3),
    ("Drums", drums, 9),
]

if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    chunks = []
    for index, (name, build, channel) in enumerate(PARTS, start=1):
        chunk = track_chunk(name, build(), channel)
        chunks.append(chunk)
        write(os.path.join(OUT_DIR, f"{index}_{name}.mid"), TEMPO_BPM, [chunk], MARKERS)
    write(os.path.join(OUT_DIR, "MenuTheme_AllParts.mid"), TEMPO_BPM, chunks, MARKERS)
    seconds = len(SONG) * 4 * 60 / TEMPO_BPM
    print(f"Wrote {len(PARTS)} parts + combined to {os.path.abspath(OUT_DIR)} ({len(SONG)} bars, {seconds:.1f} s)")
