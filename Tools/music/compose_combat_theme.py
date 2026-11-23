"""
Writes the combat theme, a UK drill instrumental, as MIDI stems for building in FL Studio, plus a rough preview.

142 BPM, half-time (snare on beat three), C# minor with the harmonic minor's B# leading tone.
A 32-bar loop in four 8-bar sections: A (the hook), A2 (the hook with a turnaround), B (the lift) and B2 (the lift,
fuller, turning back to A). Chords: C#m | A | F#m | G# in A; A | F#m | C#m | G# in B.

Each part is its own stem so the game can layer them as the waves get harder:

  Waves 1-3    1_Strings + 2_Melody
  Waves 4-7    + 3_808
  Waves 8-11   + 4_Drums
  Waves 12-15  + 5_Counter + 6_HatRolls

What makes it drill, so it survives the move to real sounds in FL Studio:
  - 808: syncopated, mostly off the kick's grid, with glides. Notes that should slide overlap the next note by
    half a 16th, so FL's Porta (or any 808 with glide) slides between them.
  - Drums: half-time snare on beat three with the skippy late snare every other bar, kicks that follow the 808,
    rims on the off-beats, and the 3-3-2 hat grouping with triplet rolls and 32nd stutters.
  - Melody: a dark two-bar bell/piano ostinato on the harmonic minor, answered higher in the lift.
  - Strings: long, low and eerie under the hook; short, rhythmic stabs in the lift.

Also writes two stings: WaveCleared and GameOver.

  python Tools/music/compose_combat_theme.py   ->  SourceArt/Music/CombatTheme/
"""

import os
import random
import sys

sys.path.insert(0, os.path.dirname(__file__))
from midi_writer import BAR, PPQ, STEP, TRIPLET_16, note, track_chunk, write  # noqa: E402
import midi_writer as kit  # noqa: E402
import synth  # noqa: E402

BPM = 142
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "SourceArt", "Music", "CombatTheme")

SECTIONS = ["A", "A2", "B", "B2"]          # 8 bars each (each chord cycle twice)
BARS = len(SECTIONS) * 8                   # 32
CYCLES = {"A": ["C#m", "A", "F#m", "G#"], "B": ["A", "F#m", "C#m", "G#"]}

# Strings: low close voicings for the hook (root in the bass), opened up for the lift.
STRING_VOICINGS = {
    "C#m": [49, 56, 61, 64], "A": [45, 52, 57, 61], "F#m": [42, 49, 54, 57], "G#": [44, 51, 56, 60],
}
ROOT_808 = {"C#m": 37, "A": 33, "F#m": 30, "G#": 32}


def section(bar):
    return SECTIONS[(bar % BARS) // 8]


def chord(bar):
    return CYCLES[section(bar)[0]][bar % 4]


# ---------------------------------------------------------------------------------------------------------------
# Melody: per bar, (note, start step, length in steps).

HOOK = [  # the two-bar ostinato, then its answer
    [("C#6", 0, 2), ("G#5", 2, 2), ("E5", 4, 2), ("G#5", 6, 1), ("C#6", 7, 1), ("D#6", 8, 2), ("E6", 10, 2), ("D#6", 12, 2), ("B#5", 14, 2)],
    [("C#6", 0, 2), ("A5", 2, 2), ("E5", 4, 2), ("A5", 6, 1), ("C#6", 7, 1), ("E6", 8, 4), ("C#6", 12, 4)],
    [("C#6", 0, 2), ("A5", 2, 2), ("F#5", 4, 2), ("A5", 6, 1), ("C#6", 7, 1), ("D#6", 8, 2), ("E6", 10, 2), ("F#6", 12, 2), ("E6", 14, 2)],
    [("D#6", 0, 2), ("B#5", 2, 2), ("G#5", 4, 2), ("B#5", 6, 1), ("D#6", 7, 1), ("G#6", 8, 4), ("F#6", 12, 2), ("D#6", 14, 2)],
]
HOOK_TURN = [("G#6", 0, 2), ("F#6", 2, 2), ("E6", 4, 2), ("D#6", 6, 2), ("B#5", 8, 4), ("G#5", 12, 2), ("B#5", 14, 2)]

LIFT = [  # over A | F#m | C#m | G#: longer, higher notes, room for the strings' stabs
    [("E6", 0, 6), ("C#6", 6, 2), ("A5", 8, 6), ("B#5", 14, 2)],
    [("C#6", 0, 6), ("A5", 6, 2), ("F#5", 8, 4), ("A5", 12, 2), ("C#6", 14, 2)],
    [("G#6", 0, 6), ("E6", 6, 2), ("C#6", 8, 4), ("E6", 12, 2), ("F#6", 14, 2)],
    [("G#6", 0, 4), ("B#6", 4, 4), ("D#7", 8, 6), ("B#6", 14, 2)],
]
LIFT_TURN = [("D#7", 0, 4), ("B#6", 4, 2), ("G#6", 6, 2), ("D#6", 8, 4), ("B#5", 12, 2), ("D#6", 14, 2)]


def melody():
    events = []
    for bar in range(BARS):
        part = section(bar)
        lines = LIFT[bar % 4] if part[0] == "B" else HOOK[bar % 4]
        if bar % 8 == 7:
            lines = HOOK_TURN if part == "A2" else LIFT_TURN if part == "B2" else lines
        for name, start, length in lines:
            velocity = 104 if start % 8 == 0 else 92 if start % 4 == 0 else 80
            events.append((bar * BAR + start * STEP, length * STEP - 12, note(name), velocity))
    return events


def counter():
    # A: high bells answering the hook in the gaps. B: a choir-like line a third or sixth above the lift.
    answers = {"C#m": ("E7", "C#7"), "A": ("E7", "C#7"), "F#m": ("F#7", "C#7"), "G#": ("D#7", "B#6")}
    harmony = {"A": ("C#7", "A6"), "F#m": ("A6", "F#6"), "C#m": ("E7", "C#7"), "G#": ("D#7", "B#6")}
    events = []
    for bar in range(BARS):
        base = bar * BAR
        if section(bar)[0] == "A":
            first, second = answers[chord(bar)]
            if bar % 2 == 1:
                events.append((base + 12 * STEP, 2 * STEP, note(first), 70))
                events.append((base + 14 * STEP, 2 * STEP, note(second), 62))
            if section(bar) == "A2" and bar % 4 == 0:
                events.append((base + 4 * STEP, 3 * STEP, note(first), 58))
        else:
            high, low = harmony[chord(bar)]
            events.append((base, 8 * STEP - 20, note(high), 66))
            events.append((base + 8 * STEP, 8 * STEP - 20, note(low), 60))
    return events


def strings():
    events = []
    for bar in range(BARS):
        base = bar * BAR
        voicing = STRING_VOICINGS[chord(bar)]
        if section(bar)[0] == "A":
            # Long and low, swelling into the second half of each cycle.
            swell = 64 + (bar % 4) * 6
            for index, pitch in enumerate(voicing):
                events.append((base, BAR - 30, pitch, swell + (8 if index == 0 else 0)))
        else:
            # Short stabs on the drill grid (3-3-2), the root held underneath.
            events.append((base, BAR - 30, voicing[0] - 12, 70))
            for start in (0, 3, 6, 8, 11, 14) if section(bar) == "B2" else (0, 3, 6, 10):
                for pitch in voicing[1:]:
                    events.append((base + start * STEP, STEP + STEP // 2, pitch + 12, 84 if start in (0, 8) else 72))
    return events


# ---------------------------------------------------------------------------------------------------------------
# 808: per bar a rhythm of (start step, length, semitones above the root, glide into this note).

BASS_RHYTHMS = [
    [(0, 5, 0, False), (5, 2, 0, False), (7, 3, 12, True), (10, 4, 0, True), (14, 2, 3, False)],
    [(0, 6, 0, False), (6, 3, 7, True), (10, 2, 0, True), (12, 2, 0, False), (14, 2, 12, True)],
    [(0, 3, 0, False), (3, 4, 0, False), (8, 2, 12, True), (10, 3, 10, True), (13, 3, 7, True)],
    [(0, 6, 0, False), (6, 2, 0, False), (9, 3, 12, True), (12, 2, 7, True), (14, 2, 0, False)],
]
BASS_LIFT = [  # busier in the lift, with a drop to the octave below
    [(0, 3, 0, False), (3, 3, 0, False), (6, 2, 12, True), (8, 3, 0, True), (11, 2, -5, True), (13, 3, 0, True)],
    [(0, 4, 0, False), (4, 2, 7, True), (6, 4, 0, True), (10, 2, 0, False), (12, 4, 12, True)],
]


def bass808():
    events = []
    for bar in range(BARS):
        root = ROOT_808[chord(bar)]
        base = bar * BAR
        rhythm = BASS_LIFT[bar % 2] if section(bar)[0] == "B" else BASS_RHYTHMS[bar % 4]
        for start, length, interval, glide in rhythm:
            if glide and events:
                # Stretch the previous note to overlap this one so Porta slides between them.
                tick, _, pitch, velocity = events[-1]
                events[-1] = (tick, base + start * STEP + STEP // 2 - tick, pitch, velocity)
            velocity = 122 if start == 0 else 112 if glide else 104
            events.append((base + start * STEP, length * STEP, root + interval, velocity))
        if bar % 8 == 7:
            # Into the next section: a glide down from the octave onto its first root.
            next_root = ROOT_808[chord(bar + 1)]
            tick, length, pitch, velocity = events[-1]
            events[-1] = (tick, base + 15 * STEP + STEP // 2 - tick, pitch, velocity)
            events.append((base + 15 * STEP, STEP + STEP // 2, next_root + 12, 110))
    return events


# ---------------------------------------------------------------------------------------------------------------
# Drums

def drums():
    events = []
    bass = bass808()
    for bar in range(BARS):
        base = bar * BAR
        part = section(bar)

        def hit(tick, pitch, velocity, length=STEP):
            events.append((base + tick, length, pitch, velocity))

        if bar % 8 == 0:
            hit(0, kit.CRASH, 104, BAR)
        # Kicks follow the 808's longer notes (the short slide notes are left to the 808 alone).
        for tick, length, _, _ in bass:
            if base <= tick < base + BAR and length >= 3 * STEP and (tick - base) % (2 * STEP) != STEP:
                hit(tick - base, kit.KICK, 118)
        # Half-time snare, the skippy late snare every other bar, a ghost before it on the fourth.
        hit(8 * STEP, kit.SNARE, 120)
        hit(8 * STEP, kit.CLAP, 92)
        if bar % 2 == 1:
            hit(15 * STEP, kit.SNARE, 94)
        if bar % 4 == 3:
            hit(13 * STEP, kit.SNARE, 64)
        # Rims on the off-beats for the bounce.
        for step in (3, 11) if bar % 2 == 0 else (6, 11, 13):
            hit(step * STEP, kit.RIM, 66 if step == 11 else 54)
        # Hats in the 3-3-2 grouping, with a triplet roll closing every other bar.
        for step in (0, 3, 6, 8, 11, 14):
            if bar % 2 == 1 and step == 14:
                continue
            hit(step * STEP, kit.HAT, 86 if step in (0, 8) else 68)
        if bar % 2 == 1:
            for index in range(6):
                hit(12 * STEP + index * TRIPLET_16, kit.HAT, 56 + index * 8, STEP // 2)
        if part[0] == "B":
            for step in (2, 10):
                hit(step * STEP + STEP // 2, kit.HAT, 50, STEP // 2)
        if bar % 4 == 3:
            hit(14 * STEP, kit.OPEN_HAT, 76, 2 * STEP)
    return events


def hat_rolls():
    # The busy top layer for the final waves: rolls that speed up, 32nd stutters and tom rolls into sections.
    random.seed(11)
    events = []
    for bar in range(BARS):
        base = bar * BAR
        # A roll into beat three every bar: triplets, then 32nds for the last few.
        for index in range(4):
            events.append((base + 6 * STEP + index * TRIPLET_16, STEP // 2, kit.HAT, 50 + index * 8))
        for index in range(4):
            events.append((base + 7 * STEP + index * STEP // 4, STEP // 4, kit.HAT, 66 + index * 8))
        # Two 32nd stutters, placed differently each bar.
        for step in random.sample([1, 4, 9, 12], 2):
            for index in range(3):
                events.append((base + step * STEP + index * STEP // 2, STEP // 2, kit.HAT, 54 + index * 10))
        if bar % 8 == 7:
            for step, tom in zip(range(12, 16), (kit.TOM_HIGH, kit.TOM_MID, kit.TOM_MID, kit.TOM_LOW)):
                events.append((base + step * STEP, STEP, tom, 96))
            events.append((base + 10 * STEP, 2 * STEP, kit.OPEN_HAT, 82))
    return events


# ---------------------------------------------------------------------------------------------------------------
# Stings

def sting_wave_cleared():
    # A rising bell arpeggio resolving on E major (the relative major: relief).
    notes = ["C#5", "E5", "G#5", "B5", "E6", "G#6"]
    events = [(i * STEP * 2 // 3, 8 * STEP, note(name), 96) for i, name in enumerate(notes)]
    events += [(4 * STEP, BAR - 4 * STEP, note(name), 80) for name in ("E5", "G#5", "B5", "E6")]
    return events


def sting_game_over():
    # A slow fall down the scale onto a low C#, with the 808 dropping beneath it.
    notes = ["G#5", "E5", "D#5", "C#5", "B#4", "C#4"]
    events = [(i * 4 * STEP, 4 * STEP if i < 5 else 3 * BAR // 2, note(name), 90) for i, name in enumerate(notes)]
    events.append((20 * STEP, BAR, ROOT_808["C#m"], 110))
    return events


PARTS = [
    ("Strings", strings, 0),
    ("Melody", melody, 1),
    ("808", bass808, 2),
    ("Drums", drums, 9),
    ("Counter", counter, 3),
    ("HatRolls", hat_rolls, 9),
]


def render_preview(path):
    mix = synth.Mix(BARS * 4 * 60 / BPM + 1.0, BPM, PPQ)
    mix.add(strings(), synth.pad, 0.07)
    mix.add(melody(), synth.bell, 0.22)
    mix.add(bass808(), synth.sub808, 0.45)
    drum_voice = lambda pitch, count: synth.drum(pitch, count, kit)  # noqa: E731
    mix.add(drums(), drum_voice, 0.34)
    mix.add(counter(), synth.bell, 0.12)
    mix.add(hat_rolls(), drum_voice, 0.22)
    mix.save(path)


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    markers = [(0, "Loop start"), (BARS * BAR, "Loop end")] + [(i * 8 * BAR, name) for i, name in enumerate(SECTIONS)]
    chunks = []
    for index, (name, build, channel) in enumerate(PARTS, start=1):
        chunk = track_chunk(name, build(), channel)
        chunks.append(chunk)
        write(os.path.join(OUT_DIR, f"{index}_{name}.mid"), BPM, [chunk], markers)
    write(os.path.join(OUT_DIR, "CombatTheme_AllParts.mid"), BPM, chunks, markers)
    write(os.path.join(OUT_DIR, "Sting_WaveCleared.mid"), BPM, [track_chunk("WaveCleared", sting_wave_cleared(), 1)])
    write(os.path.join(OUT_DIR, "Sting_GameOver.mid"), BPM, [track_chunk("GameOver", sting_game_over(), 1)])

    if "--no-preview" not in sys.argv:
        render_preview(os.path.join(OUT_DIR, "CombatTheme_Preview.wav"))
    print(f"Wrote {len(PARTS)} stems, 2 stings and a preview to {os.path.abspath(OUT_DIR)} ({BARS} bars, {BARS * 4 * 60 / BPM:.1f} s)")
