"""
Writes the combat theme, a UK drill instrumental, as MIDI stems for building in FL Studio, plus a rough preview.

142 BPM in half-time, C# minor (harmonic): C#m | A | F#m | G#. A 32-bar loop: A, A', B, B'.
Each part is its own stem so the game can layer them as the waves get harder:

  Waves 1-3    1_Strings + 2_Melody
  Waves 4-7    + 3_808
  Waves 8-11   + 4_Drums
  Waves 12-15  + 5_Counter + 6_HatRolls

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

CYCLE = ["C#m", "A", "F#m", "G#"]
SECTIONS = ["A", "A2", "B", "B2"]          # 8 bars each (the cycle twice)
BARS = len(SECTIONS) * 8                   # 32

STRING_VOICINGS = {                        # low and eerie in A, opened up an octave in B
    "C#m": [49, 61, 64, 68], "A": [45, 57, 61, 64], "F#m": [42, 54, 57, 61], "G#": [44, 56, 60, 63],
}
ROOT_808 = {"C#m": 37, "A": 33, "F#m": 30, "G#": 32}


def chord(bar):
    return CYCLE[bar % 4]


def section(bar):
    return SECTIONS[bar // 8]


# ---------------------------------------------------------------------------------------------------------------
# Melody: a dark plucked piano/bell line. Per cycle bar: (note, start step, length in steps).

MOTIF = [
    [("G#5", 0, 2), ("E5", 3, 2), ("C#5", 6, 2), ("G#5", 9, 1), ("A5", 10, 2), ("G#5", 12, 2), ("E5", 14, 2)],
    [("E5", 0, 2), ("C#5", 3, 2), ("A4", 6, 2), ("E5", 9, 1), ("F#5", 10, 2), ("E5", 12, 2), ("C#5", 14, 2)],
    [("A5", 0, 2), ("F#5", 3, 2), ("C#5", 6, 2), ("A5", 9, 1), ("B5", 10, 2), ("A5", 12, 2), ("F#5", 14, 2)],
    [("G#5", 0, 3), ("B#5", 3, 3), ("D#6", 6, 4), ("B#5", 10, 2), ("G#5", 12, 4)],
]
MOTIF_TURN = [("D#6", 0, 2), ("B#5", 2, 2), ("G#5", 4, 4), ("G#5", 8, 2), ("D#5", 10, 2), ("G#5", 12, 4)]

LIFT = [  # B sections: longer, higher notes over the same chords
    [("E6", 0, 4), ("D#6", 4, 2), ("C#6", 6, 2), ("G#5", 8, 8)],
    [("E6", 0, 4), ("C#6", 4, 4), ("A5", 8, 8)],
    [("F#6", 0, 4), ("E6", 4, 2), ("C#6", 6, 2), ("A5", 8, 8)],
    [("G#6", 0, 4), ("D#6", 4, 4), ("B#5", 8, 4), ("G#5", 12, 4)],
]


def melody():
    events = []
    for bar in range(BARS):
        part = section(bar)
        lines = LIFT[bar % 4] if part.startswith("B") else MOTIF[bar % 4]
        if bar % 8 == 7 and part == "A2":
            lines = MOTIF_TURN  # a turnaround into the lift
        for name, start, length in lines:
            events.append((bar * BAR + start * STEP, length * STEP - 12, note(name), 100 if start % 4 == 0 else 84))
    return events


def counter():
    # High bells answering the melody: the chord's fifth and third, off the beat.
    tones = {"C#m": ("G#6", "E6"), "A": ("E6", "C#6"), "F#m": ("C#7", "A6"), "G#": ("D#7", "B#6")}
    events = []
    for bar in range(BARS):
        first, second = tones[chord(bar)]
        base = bar * BAR
        events.append((base + 4 * STEP, 4 * STEP, note(first), 72))
        events.append((base + 12 * STEP, 3 * STEP, note(second), 64))
        if section(bar).startswith("B") and bar % 2 == 1:
            events.append((base + 14 * STEP, 2 * STEP, note(first), 58))
    return events


def strings():
    events = []
    for bar in range(BARS):
        lift = 12 if section(bar).startswith("B") else 0
        for index, pitch in enumerate(STRING_VOICINGS[chord(bar)]):
            events.append((bar * BAR, BAR - 30, pitch + (lift if index else 0), 70 if index else 80))
    return events


def bass808():
    # Long 808 hits with slides: notes overlap by a 16th so FL's Porta glides between them.
    events = []
    for bar in range(BARS):
        root = ROOT_808[chord(bar)]
        next_root = ROOT_808[chord(bar + 1)]
        base = bar * BAR
        events.append((base + 0 * STEP, 7 * STEP, root, 120))
        events.append((base + 7 * STEP, 4 * STEP, root, 105))
        events.append((base + 10 * STEP, 5 * STEP, root + 12, 110))     # slide up an octave
        events.append((base + 14 * STEP, 3 * STEP, next_root, 108))      # slide into the next chord
        if bar % 4 == 3:
            events.append((base + 12 * STEP, 2 * STEP, root + 7, 100))   # extra bounce on the last bar
    return events


def drums():
    events = []
    for bar in range(BARS):
        base = bar * BAR

        def hit(tick, pitch, velocity):
            events.append((base + tick, STEP, pitch, velocity))

        if bar % 8 == 0:
            hit(0, kit.CRASH, 100)
        # Kick follows the 808.
        for step in ((0, 7) if bar % 2 == 0 else (0, 10)):
            hit(step * STEP, kit.KICK, 118)
        # Drill snare: the third beat, plus a skipped late hit every other bar.
        hit(8 * STEP, kit.SNARE, 118)
        if bar % 2 == 1:
            hit(15 * STEP, kit.SNARE, 96)
        if bar % 4 == 3:
            hit(13 * STEP, kit.SNARE, 70)
        # Rims for the bounce.
        for step in (3, 11):
            hit(step * STEP, kit.RIM, 60 if bar % 2 else 52)
        # Hats: straight 8ths, a triplet roll on the last beat every fourth bar, a stutter on beat two.
        for step in range(0, 16, 2):
            if bar % 4 == 3 and step >= 12:
                continue
            hit(step * STEP, kit.HAT, 80 if step % 4 == 0 else 62)
        if bar % 4 == 3:
            for index in range(6):
                hit(12 * STEP + index * TRIPLET_16, kit.HAT, 60 + index * 8)
        if bar % 2 == 1:
            hit(6 * STEP + STEP // 2, kit.HAT, 55)
    return events


def hat_rolls():
    # The busy top layer for the final waves: rolls and stutters around the main hats.
    random.seed(7)
    events = []
    for bar in range(BARS):
        base = bar * BAR
        # A 16th-triplet roll leading into beat three every bar.
        for index in range(6):
            events.append((base + 4 * STEP + PPQ // 2 + index * TRIPLET_16 // 2, STEP // 2, kit.HAT, 48 + index * 9))
        # 32nd stutters, varied per bar.
        for step in random.sample([1, 5, 9, 13], 2):
            for index in range(3):
                events.append((base + step * STEP + index * STEP // 2, STEP // 2, kit.HAT, 52 + index * 10))
        if bar % 8 == 7:
            events.append((base + 14 * STEP, 2 * STEP, kit.OPEN_HAT, 80))
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
