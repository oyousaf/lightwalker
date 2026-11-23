"""
Writes the menu theme as MIDI, one file per part plus a combined multitrack, for building in FL Studio.

An original tune in the spirit of early-90s 16-bit platformers (Mega Drive FM / Amiga): a bouncing FM slap bass
with walk-ups between chords, off-beat stabs that turn into brass hits in the chorus, a 16th-note arpeggio, a hooky
lead that answers itself and punchy drums with fills. 140 BPM, A minor lifting to C major in the chorus.

  Bars 1-4    intro (one-shot): pads and arp, drums building, the lead's pickup into the hook
  Bars 5-28   loop: A (the hook, 8 bars), A2 (the hook, ending higher, 8), B (the chorus, 8), then back to bar 5

Run with any Python 3 (no packages needed), from the project root:
  python Tools/music/compose_menu_theme.py
Output: SourceArt/Music/MenuTheme/*.mid
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from midi_writer import BAR, PPQ, STEP, note, track_chunk, write  # noqa: E402,F401  (PPQ: used by preview_menu_theme)
from midi_writer import CLAP, CRASH, HAT, KICK, OPEN_HAT, SNARE, TOM_HIGH, TOM_LOW, TOM_MID  # noqa: E402

TEMPO_BPM = 140

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

INTRO = ["Am", "F", "G", "E"]
SECTION_A = ["Am", "F", "C", "G", "Am", "F", "G", "E"]
SECTION_B = ["F", "G", "Em", "Am", "Dm", "G", "C", "E"]
SONG = INTRO + SECTION_A + SECTION_A + SECTION_B  # 28 bars
A_START, A2_START, B_START = len(INTRO), len(INTRO) + 8, len(INTRO) + 16

# ---------------------------------------------------------------------------------------------------------------
# Lead melody: per bar, (note name, start step, length in steps)

PICKUP = [("C5", 12, 2), ("D5", 14, 2)]  # end of the intro, into the hook

LEAD_A = [
    [("E5", 0, 2), ("A5", 2, 2), ("C6", 4, 1), ("B5", 5, 1), ("A5", 6, 2), ("E6", 8, 3), ("D6", 11, 1), ("C6", 12, 2), ("B5", 14, 2)],
    [("C6", 0, 3), ("A5", 3, 1), ("F5", 4, 2), ("A5", 6, 2), ("C6", 8, 2), ("F6", 10, 2), ("E6", 12, 2), ("C6", 14, 2)],
    [("D6", 0, 2), ("E6", 2, 2), ("G6", 4, 4), ("E6", 8, 2), ("C6", 10, 2), ("G5", 12, 4)],
    [("B5", 0, 2), ("D6", 2, 2), ("B5", 4, 2), ("G5", 6, 2), ("A5", 8, 2), ("B5", 10, 2), ("D6", 12, 4)],
    [("E5", 0, 2), ("A5", 2, 2), ("C6", 4, 1), ("B5", 5, 1), ("A5", 6, 2), ("E6", 8, 3), ("D6", 11, 1), ("C6", 12, 2), ("B5", 14, 2)],
    [("C6", 0, 3), ("A5", 3, 1), ("F5", 4, 2), ("A5", 6, 2), ("C6", 8, 2), ("F6", 10, 2), ("A6", 12, 4)],
    [("G6", 0, 3), ("F6", 3, 1), ("E6", 4, 2), ("D6", 6, 2), ("B5", 8, 4), ("D6", 12, 2), ("E6", 14, 2)],
    [("E6", 0, 6), ("D6", 6, 1), ("C6", 7, 1), ("B5", 8, 2), ("G#5", 10, 2), ("C5", 12, 2), ("D5", 14, 2)],
]
LEAD_A2_ENDING = [  # the second time round, climbing into the chorus instead of falling back
    [("G6", 0, 3), ("F6", 3, 1), ("E6", 4, 2), ("D6", 6, 2), ("E6", 8, 2), ("F6", 10, 2), ("G6", 12, 4)],
    [("G#6", 0, 4), ("B6", 4, 4), ("E7", 8, 4), ("D7", 12, 2), ("C7", 14, 2)],
]

LEAD_B = [
    [("A5", 0, 4), ("C6", 4, 4), ("F6", 8, 6), ("E6", 14, 2)],
    [("D6", 0, 4), ("B5", 4, 4), ("G5", 8, 6), ("A5", 14, 2)],
    [("B5", 0, 4), ("E6", 4, 4), ("G6", 8, 6), ("F#6", 14, 2)],
    [("E6", 0, 8), ("C6", 8, 4), ("A5", 12, 4)],
    [("F6", 0, 4), ("E6", 4, 2), ("D6", 6, 2), ("A5", 8, 6), ("D6", 14, 2)],
    [("D6", 0, 4), ("C6", 4, 2), ("B5", 6, 2), ("G5", 8, 6), ("B5", 14, 2)],
    [("C6", 0, 2), ("E6", 2, 2), ("G6", 4, 2), ("C7", 6, 6), ("B6", 12, 2), ("G6", 14, 2)],
    [("G#6", 0, 4), ("B6", 4, 4), ("E6", 8, 4), ("C5", 12, 2), ("D5", 14, 2)],
]


# ---------------------------------------------------------------------------------------------------------------
# Parts: each returns a list of (tick, length, note, velocity)

def lead():
    events = []
    bars = [(len(INTRO) - 1, PICKUP)]
    bars += [(A_START + i, lines) for i, lines in enumerate(LEAD_A)]
    bars += [(A2_START + i, lines) for i, lines in enumerate(LEAD_A[:6] + LEAD_A2_ENDING)]
    bars += [(B_START + i, lines) for i, lines in enumerate(LEAD_B)]
    for bar, lines in bars:
        base = bar * BAR
        for name, start, length in lines:
            accent = 114 if start % 4 == 0 else 98
            events.append((base + start * STEP, length * STEP - 10, note(name), accent))
    return events


def bass():
    # Bouncing FM slap: root, octave and fifth on a syncopated 16th pattern, walking up into each new chord.
    pattern = [(0, 2, 0), (2, 1, 12), (3, 1, 0), (4, 2, 12), (6, 2, 0), (8, 2, 0), (10, 1, 12), (11, 1, 0), (12, 2, 7)]
    events = []
    for bar, chord in enumerate(SONG):
        root = CHORDS[chord][1]
        base = bar * BAR
        if bar < 2:  # intro: long roots to set the mood before the bounce kicks in
            events.append((base, BAR - 20, root, 100))
            continue
        chorus = bar >= B_START
        for start, length, interval in pattern:
            events.append((base + start * STEP, length * STEP - 15, root + interval, 120 if start % 4 == 0 else 96))
        # Last two 16ths: a walk towards the next root (chromatic from below), or the octave if it doesn't move.
        next_root = CHORDS[SONG[(bar + 1) if bar + 1 < len(SONG) else A_START]][1]
        if next_root != root:
            approach = [next_root - 2, next_root - 1] if chorus or bar % 2 else [root + 7, next_root - 1]
        else:
            approach = [root + 12, root + 7]
        for index, pitch in enumerate(approach):
            events.append((base + (14 + index) * STEP, STEP - 15, pitch, 100))
    return events


def stabs():
    # Off-beat chord stabs in the hook; brass-style hits on the chorus rhythm; long pads in the intro.
    events = []
    for bar, chord in enumerate(SONG):
        voicing = CHORDS[chord][0]
        base = bar * BAR
        if bar < len(INTRO):
            for pitch in voicing:
                events.append((base, BAR - 20, pitch, 76))
            continue
        if bar >= B_START:
            for start, length in ((0, 3), (3, 3), (6, 2), (10, 2), (12, 4)):
                for pitch in voicing:
                    events.append((base + start * STEP, length * STEP - 20, pitch + 12, 96 if start == 0 else 86))
            continue
        for start in (2, 6, 10, 14):
            for pitch in voicing:
                events.append((base + start * STEP, STEP, pitch, 90))
    return events


def arp():
    # 16ths over two octaves of the chord: up-and-down in the hook, leaping octaves in the chorus.
    hook_shape = [0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5]
    chorus_shape = [0, 3, 1, 4, 2, 5, 3, 4, 0, 3, 1, 4, 2, 5, 4, 3]
    events = []
    for bar, chord in enumerate(SONG):
        if bar < 2:  # silent for the first two intro bars, then fades in
            continue
        voicing = sorted(CHORDS[chord][0])
        tones = [p + 12 for p in voicing] + [p + 24 for p in voicing]
        base = bar * BAR
        shape = chorus_shape if bar >= B_START else hook_shape
        velocity = 56 + (bar - 2) * 6 if bar < len(INTRO) else 76
        for step, index in enumerate(shape):
            events.append((base + step * STEP, STEP - 10, tones[index], velocity + (10 if step % 4 == 0 else 0)))
    return events


def drums():
    events = []
    section_starts = {A_START, A2_START, B_START}
    for bar in range(len(SONG)):
        base = bar * BAR
        last_of_section = (bar + 1) in section_starts or bar == len(SONG) - 1
        chorus = bar >= B_START

        def hit(step, pitch, velocity, length=STEP):
            events.append((base + step * STEP, length, pitch, velocity))

        if bar < 2:
            for step in range(0, 16, 4):
                hit(step, HAT, 55)
            continue
        if bar < len(INTRO):
            for step in (0, 4, 8, 12):
                hit(step, KICK, 110)
            for step in range(0, 16, 2):
                hit(step, HAT, 70)
            if bar == len(INTRO) - 1:  # snare build into the hook
                for step in range(8, 16):
                    hit(step, SNARE, 70 + step * 3)
            continue

        if bar in section_starts:
            hit(0, CRASH, 112, BAR)
        if last_of_section:
            # Tom fill across the last beat and a half into the next section.
            for step in (0, 6, 8):
                hit(step, KICK, 118)
            hit(4, SNARE, 112)
            for step, tom in zip(range(10, 16), (TOM_HIGH, TOM_HIGH, TOM_MID, TOM_MID, TOM_LOW, TOM_LOW)):
                hit(step, tom, 100)
            continue

        for step in (0, 6, 8, 11) if bar % 2 == 0 else (0, 3, 8, 10):
            hit(step, KICK, 120 if step in (0, 8) else 100)
        hit(4, SNARE, 116)
        hit(12, SNARE, 116)
        hit(12, CLAP, 90)
        if bar % 4 == 3:
            hit(15, SNARE, 70)  # ghost into the next bar
        # Hats: 8ths in the hook, 16ths with accents in the chorus; an open hat to lift each half-bar's end.
        for step in range(0, 16, 1 if chorus else 2):
            pitch = OPEN_HAT if step == 14 else HAT
            hit(step, pitch, 86 if step % 4 == 0 else 66 if step % 2 == 0 else 50)
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
