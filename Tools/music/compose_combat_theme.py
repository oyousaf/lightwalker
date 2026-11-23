"""
Writes the combat theme, a dark UK drill instrumental, as MIDI stems for building in FL Studio, plus a rough preview.

142 BPM, half-time (snare on beat three), C# minor with the harmonic minor's B# leading tone. About 3 minutes: a
112-bar loop of 8-bar sections that vary so it doesn't feel like a loop:

  A      the hook: a two-bar piano/bell ostinato over long, low strings       C#m | A | F#m | G#
  A2     the hook with a turnaround
  B      the lift: a higher, longer melody, string stabs, a busier 808        A | F#m | C#m | G#
  B2     the lift, fuller, turning back
  Break  strings up high, the hook's notes held, 808 and drums thinned out
  C      a new section: a lower melody over new chords, pulsing strings       F#m | C#m | A | G#
  C2     its second half an octave up
  A3     the hook, its second half an octave up
  A4     the hook with a turnaround
  B3     the lift
  B4     the lift, turning back
  Drop   three bars of near silence, a roll, then everything slams back in
  C3     the new section again
  Turn   the hook thinning out, then the turnaround back to A

Each part is its own stem so the game can layer them as the waves get harder:

  Waves 1-3    1_Strings + 2_Melody
  Waves 4-7    + 3_808
  Waves 8-11   + 4_Drums
  Waves 12-15  + 5_Counter + 6_HatRolls

What makes it drill, so it survives the move to real sounds in FL Studio:
  - 808: syncopated, with glides. Notes that should slide overlap the next note by half a 16th, so FL's Porta
    (or any 808 with glide) slides between them.
  - Drums: half-time snare (with a clap) on beat three, the skippy late snare every other bar, kicks that follow
    the 808, rims on the off-beats, and the 3-3-2 hat grouping with triplet rolls and 32nd stutters.
  - Melody: a dark bell/piano ostinato on the harmonic minor, answered higher in the lift.

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

STRING_VOICINGS = {
    "C#m": [49, 56, 61, 64], "A": [45, 52, 57, 61], "F#m": [42, 49, 54, 57], "G#": [44, 51, 56, 60],
}
ROOT_808 = {"C#m": 37, "A": 33, "F#m": 30, "G#": 32}

HOOK_CHORDS = ["C#m", "A", "F#m", "G#"] * 2
LIFT_CHORDS = ["A", "F#m", "C#m", "G#"] * 2
NEW_CHORDS = ["F#m", "C#m", "A", "G#"] * 2

# ---------------------------------------------------------------------------------------------------------------
# Melody lines: per bar, (note, start step, length in steps).

HOOK = [
    [("C#6", 0, 2), ("G#5", 2, 2), ("E5", 4, 2), ("G#5", 6, 1), ("C#6", 7, 1), ("D#6", 8, 2), ("E6", 10, 2), ("D#6", 12, 2), ("B#5", 14, 2)],
    [("C#6", 0, 2), ("A5", 2, 2), ("E5", 4, 2), ("A5", 6, 1), ("C#6", 7, 1), ("E6", 8, 4), ("C#6", 12, 4)],
    [("C#6", 0, 2), ("A5", 2, 2), ("F#5", 4, 2), ("A5", 6, 1), ("C#6", 7, 1), ("D#6", 8, 2), ("E6", 10, 2), ("F#6", 12, 2), ("E6", 14, 2)],
    [("D#6", 0, 2), ("B#5", 2, 2), ("G#5", 4, 2), ("B#5", 6, 1), ("D#6", 7, 1), ("G#6", 8, 4), ("F#6", 12, 2), ("D#6", 14, 2)],
]
HOOK_TURN = [("G#6", 0, 2), ("F#6", 2, 2), ("E6", 4, 2), ("D#6", 6, 2), ("B#5", 8, 4), ("G#5", 12, 2), ("B#5", 14, 2)]

LIFT = [
    [("E6", 0, 6), ("C#6", 6, 2), ("A5", 8, 6), ("B#5", 14, 2)],
    [("C#6", 0, 6), ("A5", 6, 2), ("F#5", 8, 4), ("A5", 12, 2), ("C#6", 14, 2)],
    [("G#6", 0, 6), ("E6", 6, 2), ("C#6", 8, 4), ("E6", 12, 2), ("F#6", 14, 2)],
    [("G#6", 0, 4), ("B#6", 4, 4), ("D#7", 8, 6), ("B#6", 14, 2)],
]
LIFT_TURN = [("D#7", 0, 4), ("B#6", 4, 2), ("G#6", 6, 2), ("D#6", 8, 4), ("B#5", 12, 2), ("D#6", 14, 2)]

NEW_LINE = [  # lower and darker, over F#m | C#m | A | G#
    [("F#5", 0, 2), ("A5", 2, 2), ("C#6", 4, 2), ("A5", 6, 2), ("F#5", 8, 4), ("E5", 12, 4)],
    [("E5", 0, 2), ("G#5", 2, 2), ("C#6", 4, 2), ("E6", 6, 2), ("C#6", 8, 4), ("G#5", 12, 4)],
    [("A5", 0, 2), ("C#6", 2, 2), ("E6", 4, 2), ("C#6", 6, 2), ("A5", 8, 4), ("E6", 12, 4)],
    [("D#6", 0, 2), ("B#5", 2, 2), ("G#5", 4, 2), ("B#5", 6, 2), ("D#6", 8, 4), ("G#6", 12, 4)],
]


def octave_up(bars):
    return [[(name[:-1] + str(int(name[-1]) + 1), start, length) for name, start, length in lines] for lines in bars]


def held(bars):
    """Each bar's first note held for half a bar: the line's skeleton, for the quiet sections."""
    return [[(lines[0][0], 0, 8)] if lines else [] for lines in bars]


MELODY = {
    "A": HOOK + HOOK,
    "A2": HOOK + HOOK[:3] + [HOOK_TURN],
    "B": LIFT + LIFT,
    "B2": LIFT + LIFT[:3] + [LIFT_TURN],
    "Break": held(HOOK) + HOOK,
    "C": NEW_LINE + NEW_LINE,
    "C2": NEW_LINE + octave_up(NEW_LINE[:3]) + [[("G#6", 0, 4), ("D#6", 4, 4), ("B#5", 8, 4), ("G#5", 12, 4)]],
    "A3": HOOK + octave_up(HOOK),
    "Drop": [[], [], [], []] + HOOK,
    "Turn": HOOK + [[("C#6", 0, 8), ("G#5", 8, 8)], [("A5", 0, 16)], [("F#5", 0, 8), ("A5", 8, 8)], HOOK_TURN],
}

# ---------------------------------------------------------------------------------------------------------------
# Song layout: each section's chords, melody, and the style of each part.

SECTIONS = [
    # name     chords        melody   strings   808     drums    counter    hat rolls
    ("A",      HOOK_CHORDS,  "A",     "long",   "hook", "full",  "answer",  "full"),
    ("A2",     HOOK_CHORDS,  "A2",    "long",   "hook", "full",  "answer",  "full"),
    ("B",      LIFT_CHORDS,  "B",     "stabs",  "lift", "lift",  "harmony", "full"),
    ("B2",     LIFT_CHORDS,  "B2",    "full",   "lift", "lift",  "harmony", "full"),
    ("Break",  HOOK_CHORDS,  "Break", "high",   "long", "break", "high",    "light"),
    ("C",      NEW_CHORDS,   "C",     "pulse",  "new",  "full",  "answer",  "full"),
    ("C2",     NEW_CHORDS,   "C2",    "pulse",  "new",  "lift",  "harmony", "full"),
    ("A3",     HOOK_CHORDS,  "A3",    "long",   "hook", "full",  "answer",  "full"),
    ("A4",     HOOK_CHORDS,  "A2",    "long",   "hook", "full",  "answer",  "full"),
    ("B3",     LIFT_CHORDS,  "B",     "stabs",  "lift", "lift",  "harmony", "full"),
    ("B4",     LIFT_CHORDS,  "B2",    "full",   "lift", "lift",  "harmony", "full"),
    ("Drop",   HOOK_CHORDS,  "Drop",  "drop",   "drop", "drop",  "drop",    "drop"),
    ("C3",     NEW_CHORDS,   "C",     "pulse",  "new",  "full",  "answer",  "full"),
    ("Turn",   HOOK_CHORDS,  "Turn",  "long",   "hook", "full",  "answer",  "full"),
]

LAYOUT = []  # per bar: (section, index in section, chord, melody lines, styles)
for _name, _chords, _melody, _strings, _bass, _drums, _counter, _rolls in SECTIONS:
    for _index, _chord in enumerate(_chords):
        LAYOUT.append((_name, _index, _chord, MELODY[_melody][_index],
                       {"strings": _strings, "808": _bass, "drums": _drums, "counter": _counter, "rolls": _rolls}))
BARS = len(LAYOUT)  # 112


def at(bar):
    return LAYOUT[bar % BARS]


# ---------------------------------------------------------------------------------------------------------------
# Parts: each returns a list of (tick, length, note, velocity)

def melody():
    events = []
    for bar in range(BARS):
        section, _, _, lines, _ = at(bar)
        soft = section in ("Break",) and at(bar)[1] < 4
        for name, start, length in lines:
            velocity = 104 if start % 8 == 0 else 92 if start % 4 == 0 else 80
            events.append((bar * BAR + start * STEP, length * STEP - 12, note(name), velocity - (24 if soft else 0)))
    return events


def counter():
    answers = {"C#m": ("E7", "C#7"), "A": ("E7", "C#7"), "F#m": ("F#7", "C#7"), "G#": ("D#7", "B#6")}
    harmony = {"A": ("C#7", "A6"), "F#m": ("A6", "F#6"), "C#m": ("E7", "C#7"), "G#": ("D#7", "B#6")}
    events = []
    for bar in range(BARS):
        _, index, chord, _, styles = at(bar)
        style, base = styles["counter"], bar * BAR
        if style == "drop":
            style = "answer" if index >= 4 else None
        if style == "answer":
            first, second = answers[chord]
            if bar % 2 == 1:
                events.append((base + 12 * STEP, 2 * STEP, note(first), 70))
                events.append((base + 14 * STEP, 2 * STEP, note(second), 62))
            if index % 4 == 0:
                events.append((base + 4 * STEP, 3 * STEP, note(first), 56))
        elif style == "harmony":
            high, low = harmony[chord]
            events.append((base, 8 * STEP - 20, note(high), 66))
            events.append((base + 8 * STEP, 8 * STEP - 20, note(low), 60))
        elif style == "high":
            events.append((base, BAR - 20, note(harmony[chord][0]), 52))
    return events


def strings():
    events = []
    for bar in range(BARS):
        _, index, chord, _, styles = at(bar)
        style, base = styles["strings"], bar * BAR
        voicing = STRING_VOICINGS[chord]
        if style == "drop":
            if index < 4:
                continue
            style = "long"
        if style == "long":
            swell = 64 + (index % 4) * 6
            for order, pitch in enumerate(voicing):
                events.append((base, BAR - 30, pitch, swell + (8 if order == 0 else 0)))
        elif style == "high":
            for pitch in voicing[1:]:
                events.append((base, BAR - 30, pitch + 24, 52 + index * 3))
        elif style == "pulse":
            # Low chords pulsing on the beat, dark and tense.
            for start in (0, 4, 8, 12):
                for pitch in voicing:
                    events.append((base + start * STEP, 3 * STEP, pitch, 74 if start in (0, 8) else 62))
        else:  # "stabs" / "full": short stabs on the drill grid (3-3-2), the root held underneath
            events.append((base, BAR - 30, voicing[0] - 12, 70))
            for start in (0, 3, 6, 8, 11, 14) if style == "full" else (0, 3, 6, 10):
                for pitch in voicing[1:]:
                    events.append((base + start * STEP, STEP + STEP // 2, pitch + 12, 84 if start in (0, 8) else 72))
    return events


# 808 rhythms: per bar, (start step, length, semitones above the root, glide into this note).
BASS_RHYTHMS = {
    "hook": [
        [(0, 5, 0, False), (5, 2, 0, False), (7, 3, 12, True), (10, 4, 0, True), (14, 2, 3, False)],
        [(0, 6, 0, False), (6, 3, 7, True), (10, 2, 0, True), (12, 2, 0, False), (14, 2, 12, True)],
        [(0, 3, 0, False), (3, 4, 0, False), (8, 2, 12, True), (10, 3, 10, True), (13, 3, 7, True)],
        [(0, 6, 0, False), (6, 2, 0, False), (9, 3, 12, True), (12, 2, 7, True), (14, 2, 0, False)],
    ],
    "lift": [
        [(0, 3, 0, False), (3, 3, 0, False), (6, 2, 12, True), (8, 3, 0, True), (11, 2, -5, True), (13, 3, 0, True)],
        [(0, 4, 0, False), (4, 2, 7, True), (6, 4, 0, True), (10, 2, 0, False), (12, 4, 12, True)],
    ],
    "new": [
        [(0, 8, 0, False), (8, 3, 12, True), (11, 3, 0, True), (14, 2, 7, False)],
        [(0, 4, 0, False), (4, 4, 7, True), (8, 6, 0, True), (14, 2, 12, True)],
    ],
    "long": [[(0, 16, 0, False)]],
}


def bass808():
    events = []
    for bar in range(BARS):
        _, index, chord, _, styles = at(bar)
        style, base, root = styles["808"], bar * BAR, ROOT_808[chord]
        if style == "drop":
            if index < 3:
                continue
            if index == 3:  # one low hit that slides up into the slam
                events.append((base + 8 * STEP, 8 * STEP + STEP // 2, root - 12, 110))
                events.append((base + 15 * STEP, STEP + STEP // 2, root, 112))
                continue
            style = "hook"
        rhythms = BASS_RHYTHMS[style]
        for start, length, interval, glide in rhythms[index % len(rhythms)]:
            if glide and events:
                # Stretch the previous note to overlap this one so Porta slides between them.
                tick, _, pitch, velocity = events[-1]
                events[-1] = (tick, base + start * STEP + STEP // 2 - tick, pitch, velocity)
            velocity = 122 if start == 0 else 112 if glide else 104
            events.append((base + start * STEP, length * STEP, root + interval, velocity - (20 if style == "long" else 0)))
        if index == 7 and style != "long":
            # Into the next section: a glide down from the octave onto its first root.
            next_root = ROOT_808[at(bar + 1)[2]]
            tick, _, pitch, velocity = events[-1]
            events[-1] = (tick, base + 15 * STEP + STEP // 2 - tick, pitch, velocity)
            events.append((base + 15 * STEP, STEP + STEP // 2, next_root + 12, 110))
    return events


def drums():
    events = []
    bass = bass808()
    for bar in range(BARS):
        _, index, _, _, styles = at(bar)
        style, base = styles["drums"], bar * BAR

        def hit(tick, pitch, velocity, length=STEP):
            events.append((base + tick, length, pitch, velocity))

        if style == "drop":
            if index < 3:
                continue
            if index == 3:  # a hat roll that speeds up, and a snare roll into the slam
                for i in range(8):
                    hit(8 * STEP + i * STEP // 2, kit.HAT, 50 + i * 6, STEP // 2)
                for i in range(4):
                    hit(12 * STEP + i * STEP, kit.SNARE, 70 + i * 12)
                continue
            if index == 4:
                hit(0, kit.CRASH, 118, BAR)
            style = "full"
        if style == "break":
            # Hats and rims only, then a roll back in on the last bar.
            for step in (0, 3, 6, 8, 11, 14):
                hit(step * STEP, kit.HAT, 62 if step in (0, 8) else 48)
            for step in (3, 11):
                hit(step * STEP, kit.RIM, 50)
            if index == 7:
                for i in range(6):
                    hit(12 * STEP + i * TRIPLET_16, kit.SNARE, 60 + i * 10)
            continue

        if index == 0:
            hit(0, kit.CRASH, 104, BAR)
        for tick, length, _, _ in bass:
            if base <= tick < base + BAR and length >= 3 * STEP and (tick - base) % (2 * STEP) != STEP:
                hit(tick - base, kit.KICK, 118)
        hit(8 * STEP, kit.SNARE, 120)
        hit(8 * STEP, kit.CLAP, 92)
        if bar % 2 == 1:
            hit(15 * STEP, kit.SNARE, 94)
        if bar % 4 == 3:
            hit(13 * STEP, kit.SNARE, 64)
        for step in (3, 11) if bar % 2 == 0 else (6, 11, 13):
            hit(step * STEP, kit.RIM, 66 if step == 11 else 54)
        for step in (0, 3, 6, 8, 11, 14):
            if bar % 2 == 1 and step == 14:
                continue
            hit(step * STEP, kit.HAT, 86 if step in (0, 8) else 68)
        if bar % 2 == 1:
            for i in range(6):
                hit(12 * STEP + i * TRIPLET_16, kit.HAT, 56 + i * 8, STEP // 2)
        if style == "lift":
            for step in (2, 10):
                hit(step * STEP + STEP // 2, kit.HAT, 50, STEP // 2)
        if bar % 4 == 3:
            hit(14 * STEP, kit.OPEN_HAT, 76, 2 * STEP)
    return events


def hat_rolls():
    random.seed(11)
    events = []
    for bar in range(BARS):
        _, index, _, _, styles = at(bar)
        style, base = styles["rolls"], bar * BAR
        if style == "drop" and index < 4:
            continue
        if style != "light":
            # A roll into beat three: triplets, then 32nds for the last few.
            for i in range(4):
                events.append((base + 6 * STEP + i * TRIPLET_16, STEP // 2, kit.HAT, 50 + i * 8))
            for i in range(4):
                events.append((base + 7 * STEP + i * STEP // 4, STEP // 4, kit.HAT, 66 + i * 8))
            for step in random.sample([1, 4, 9, 12], 2):
                for i in range(3):
                    events.append((base + step * STEP + i * STEP // 2, STEP // 2, kit.HAT, 54 + i * 10))
        if index == 7:
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
    markers = [(0, "Loop start"), (BARS * BAR, "Loop end")]
    bar = 0
    for name, chords, *_ in SECTIONS:
        markers.append((bar * BAR, name))
        bar += len(chords)
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
