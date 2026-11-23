"""
Writes the menu theme as MIDI, one file per part plus a combined multitrack, for building in FL Studio.

An original tune in the spirit of early-90s 16-bit platformers and 80s club synth-pop: a bouncing slap/synth bass
that walks up into each chord, off-beat stabs that turn into brass hits in the chorus, a 16th-note arpeggio, a hooky
lead and punchy drums with fills. 140 BPM, A minor, lifting to C major in the chorus and up a tone for the last one.

About 3 minutes: a 4-bar intro that plays once, then a 104-bar loop of 8-bar sections that vary so the loop doesn't
feel like one:

  Intro   4    pads and arp, drums building, the lead's pickup into the hook
  A       8    the hook
  A2      8    the hook, climbing into the chorus
  B       8    the chorus: brass hits, 16th hats, leaping arp
  Break   8    hats and pads only, the hook's notes held long, a drum build back in
  C       8    the bridge: new chords (Dm Em F G), a new melody
  C2      8    the bridge an octave up, full drums
  A3      8    the hook, its first half an octave up
  A4      8    the hook, climbing into the chorus
  B2      8    the chorus
  B3      8    the chorus up a whole tone (the big finish)
  Drop    8    kick and bass alone, then the hook creeps back in
  C3      8    the bridge again
  Tag     8    long notes over the hook's chords, a pickup back to A

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
HOOK_CHORDS = ["Am", "F", "C", "G", "Am", "F", "G", "E"]
CHORUS_CHORDS = ["F", "G", "Em", "Am", "Dm", "G", "C", "E"]
BRIDGE_CHORDS = ["Dm", "Em", "F", "G", "Dm", "Em", "F", "E"]

# ---------------------------------------------------------------------------------------------------------------
# Lead melodies: per bar, (note name, start step, length in steps)

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
INTO_CHORUS = [  # the hook's last two bars, climbing into the chorus instead of falling back
    [("G6", 0, 3), ("F6", 3, 1), ("E6", 4, 2), ("D6", 6, 2), ("E6", 8, 2), ("F6", 10, 2), ("G6", 12, 4)],
    [("G#6", 0, 4), ("B6", 4, 4), ("E7", 8, 4), ("D7", 12, 2), ("C7", 14, 2)],
]
LEAD_A2 = LEAD_A[:6] + INTO_CHORUS


def octave_up(bars):
    return [[(name[:-1] + str(int(name[-1]) + 1), start, length) for name, start, length in lines] for lines in bars]


LEAD_A3 = octave_up(LEAD_A[:4]) + LEAD_A[4:]

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

LEAD_BREAK = [  # the hook's notes held long, then its first phrase returning
    [("E6", 0, 6), ("C6", 8, 6)],
    [("A5", 0, 6), ("C6", 8, 6)],
    [("G5", 0, 4), ("C6", 4, 4), ("E6", 8, 8)],
    [("D6", 0, 8), ("B5", 8, 8)],
    [("E5", 0, 2), ("A5", 2, 2), ("C6", 4, 1), ("B5", 5, 1), ("A5", 6, 2), ("E6", 8, 8)],
    [("C6", 0, 3), ("A5", 3, 1), ("F5", 4, 2), ("A5", 6, 2), ("C6", 8, 8)],
    [("D6", 0, 4), ("E6", 4, 4), ("G6", 8, 8)],
    [("G#6", 0, 8), ("B5", 8, 8)],
]

LEAD_C = [  # the bridge: rising arpeggios that fall back, answering itself
    [("F5", 0, 2), ("A5", 2, 2), ("D6", 4, 4), ("C6", 8, 2), ("A5", 10, 2), ("F5", 12, 4)],
    [("G5", 0, 2), ("B5", 2, 2), ("E6", 4, 4), ("D6", 8, 2), ("B5", 10, 2), ("G5", 12, 4)],
    [("A5", 0, 2), ("C6", 2, 2), ("F6", 4, 4), ("E6", 8, 2), ("C6", 10, 2), ("A5", 12, 2), ("C6", 14, 2)],
    [("B5", 0, 4), ("D6", 4, 4), ("G6", 8, 6), ("F6", 14, 2)],
    [("F6", 0, 3), ("E6", 3, 1), ("D6", 4, 2), ("A5", 6, 2), ("D6", 8, 2), ("F6", 10, 2), ("A6", 12, 4)],
    [("G6", 0, 3), ("E6", 3, 1), ("B5", 4, 2), ("E6", 6, 2), ("G6", 8, 2), ("B6", 10, 2), ("G6", 12, 4)],
    [("A6", 0, 4), ("G6", 4, 2), ("F6", 6, 2), ("C6", 8, 4), ("F6", 12, 4)],
    [("G#6", 0, 4), ("E6", 4, 2), ("B5", 6, 2), ("G#5", 8, 4), ("B5", 12, 2), ("E6", 14, 2)],
]
LEAD_C2 = octave_up(LEAD_C[:4]) + [
    [("D6", 0, 2), ("F6", 2, 2), ("A6", 4, 4), ("G6", 8, 2), ("F6", 10, 2), ("D6", 12, 4)],
    [("E6", 0, 2), ("G6", 2, 2), ("B6", 4, 4), ("A6", 8, 2), ("G6", 10, 2), ("E6", 12, 4)],
    [("F6", 0, 2), ("A6", 2, 2), ("C7", 4, 4), ("B6", 8, 2), ("A6", 10, 2), ("F6", 12, 4)],
    [("E6", 0, 4), ("G#6", 4, 4), ("B6", 8, 4), ("C6", 12, 2), ("D6", 14, 2)],
]
LEAD_C3 = LEAD_C[:7] + [[("G#6", 0, 4), ("E6", 4, 4), ("B5", 8, 8)]]

LEAD_DROP = [[], [], [], []] + LEAD_A[4:]  # silence, then the hook's second half creeping back

LEAD_TAG = [
    [("A5", 0, 8), ("E6", 8, 8)],
    [("F6", 0, 8), ("C6", 8, 8)],
    [("E6", 0, 8), ("G6", 8, 8)],
    [("D6", 0, 16)],
    [("C6", 0, 4), ("B5", 4, 4), ("A5", 8, 8)],
    [("A5", 0, 4), ("C6", 4, 4), ("F6", 8, 8)],
    [("D6", 0, 4), ("B5", 4, 4), ("G5", 8, 8)],
    [("G#5", 0, 8), ("B5", 8, 4), ("C5", 12, 2), ("D5", 14, 2)],
]

# ---------------------------------------------------------------------------------------------------------------
# Song layout. Each section: name, chords, lead, transpose (semitones), and the style of each part.

SECTIONS = [
    # name     chords          lead        key  bass      stabs     arp       drums
    ("A",      HOOK_CHORDS,    LEAD_A,     0,   "bounce", "offbeat", "hook",   "groove"),
    ("A2",     HOOK_CHORDS,    LEAD_A2,    0,   "bounce", "offbeat", "hook",   "groove"),
    ("B",      CHORUS_CHORDS,  LEAD_B,     0,   "bounce", "brass",  "chorus", "chorus"),
    ("Break",  HOOK_CHORDS,    LEAD_BREAK, 0,   "long",   "pad",    "hook",   "break"),
    ("C",      BRIDGE_CHORDS,  LEAD_C,     0,   "bounce", "offbeat", "chorus", "groove"),
    ("C2",     BRIDGE_CHORDS,  LEAD_C2,    0,   "bounce", "brass",  "chorus", "chorus"),
    ("A3",     HOOK_CHORDS,    LEAD_A3,    0,   "bounce", "offbeat", "hook",   "groove"),
    ("A4",     HOOK_CHORDS,    LEAD_A2,    0,   "bounce", "offbeat", "hook",   "groove"),
    ("B2",     CHORUS_CHORDS,  LEAD_B,     0,   "bounce", "brass",  "chorus", "chorus"),
    ("B3",     CHORUS_CHORDS,  LEAD_B,     2,   "bounce", "brass",  "chorus", "chorus"),
    ("Drop",   HOOK_CHORDS,    LEAD_DROP,  0,   "drop",   "drop",   "drop",   "drop"),
    ("C3",     BRIDGE_CHORDS,  LEAD_C3,    0,   "bounce", "offbeat", "chorus", "groove"),
    ("Tag",    HOOK_CHORDS,    LEAD_TAG,   0,   "bounce", "pad",    "hook",   "groove"),
]

SONG = list(INTRO)
LAYOUT = []  # per bar: (section name, index in section, chord, transpose, styles dict, lead lines)
for _ in INTRO:
    LAYOUT.append(None)
for name, chords, lead_bars, key, bass_style, stab_style, arp_style, drum_style in SECTIONS:
    for index, chord in enumerate(chords):
        SONG.append(chord)
        LAYOUT.append((name, index, chord, key,
                       {"bass": bass_style, "stabs": stab_style, "arp": arp_style, "drums": drum_style},
                       lead_bars[index]))
LOOP_START = len(INTRO)


def wrap(bar):
    """A song bar, with the loop's end wrapping back to its start."""
    return LOOP_START + (bar - len(SONG)) if bar >= len(SONG) else bar


def bar_info(bar):
    return LAYOUT[wrap(bar)]


def bass_root(bar):
    """The bass root of a bar, in its key (the intro is untransposed)."""
    bar = wrap(bar)
    return CHORDS[SONG[bar]][1] + (LAYOUT[bar][3] if LAYOUT[bar] else 0)


# ---------------------------------------------------------------------------------------------------------------
# Parts: each returns a list of (tick, length, note, velocity)

def lead():
    events = []
    base = (len(INTRO) - 1) * BAR
    for name, start, length in PICKUP:
        events.append((base + start * STEP, length * STEP - 10, note(name), 110))
    for bar in range(LOOP_START, len(SONG)):
        _, _, _, key, _, lines = LAYOUT[bar]
        base = bar * BAR
        for name, start, length in lines:
            accent = 114 if start % 4 == 0 else 98
            events.append((base + start * STEP, length * STEP - 10, note(name) + key, accent))
    return events


def bass():
    # Bouncing slap/synth bass: root, octave and fifth on a syncopated 16th pattern, walking up into each new chord.
    pattern = [(0, 2, 0), (2, 1, 12), (3, 1, 0), (4, 2, 12), (6, 2, 0), (8, 2, 0), (10, 1, 12), (11, 1, 0), (12, 2, 7)]
    events = []
    for bar, chord in enumerate(SONG):
        base = bar * BAR
        info = LAYOUT[bar]
        if info is None:  # intro: long roots, then the bounce in its last two bars
            root = CHORDS[chord][1]
            if bar < 2:
                events.append((base, BAR - 20, root, 100))
                continue
            key, style, index = 0, "bounce", bar
        else:
            _, index, _, key, styles, _ = info
            style = styles["bass"]
        root = CHORDS[chord][1] + key
        if style == "long" or (style == "drop" and index < 4):
            events.append((base, 8 * STEP - 20, root, 104))
            events.append((base + 8 * STEP, 8 * STEP - 20, root + (12 if style == "drop" else 0), 92))
            continue
        for start, length, interval in pattern:
            events.append((base + start * STEP, length * STEP - 15, root + interval, 120 if start % 4 == 0 else 96))
        next_root = bass_root(bar + 1)
        if next_root != root:
            approach = [next_root - 2, next_root - 1] if bar % 2 else [root + 7, next_root - 1]
        else:
            approach = [root + 12, root + 7]
        for index, pitch in enumerate(approach):
            events.append((base + (14 + index) * STEP, STEP - 15, pitch, 100))
    return events


def stabs():
    # Off-beat chord stabs, brass-style hits in the choruses, long pads in the intro, the break and the tag.
    events = []
    for bar, chord in enumerate(SONG):
        base = bar * BAR
        info = LAYOUT[bar]
        style = "pad" if info is None else info[4]["stabs"]
        key = 0 if info is None else info[3]
        index = 0 if info is None else info[1]
        voicing = [pitch + key for pitch in CHORDS[chord][0]]
        if style == "pad" or (style == "drop" and index >= 4):
            for pitch in voicing:
                events.append((base, BAR - 20, pitch, 72 if info else 76))
        elif style == "brass":
            for start, length in ((0, 3), (3, 3), (6, 2), (10, 2), (12, 4)):
                for pitch in voicing:
                    events.append((base + start * STEP, length * STEP - 20, pitch + 12, 96 if start == 0 else 86))
        elif style == "offbeat":
            for start in (2, 6, 10, 14):
                for pitch in voicing:
                    events.append((base + start * STEP, STEP, pitch, 90))
    return events


def arp():
    # 16ths over two octaves of the chord: up-and-down in the hook, leaping octaves in the chorus and bridge.
    shapes = {
        "hook": [0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5],
        "chorus": [0, 3, 1, 4, 2, 5, 3, 4, 0, 3, 1, 4, 2, 5, 4, 3],
    }
    events = []
    for bar, chord in enumerate(SONG):
        if bar < 2:  # silent for the first two intro bars, then fades in
            continue
        info = LAYOUT[bar]
        style = "hook" if info is None else info[4]["arp"]
        key = 0 if info is None else info[3]
        index = 0 if info is None else info[1]
        if style == "drop" and index < 4:
            continue
        shape = shapes["hook" if style == "drop" else style]
        voicing = sorted(CHORDS[chord][0])
        tones = [p + 12 + key for p in voicing] + [p + 24 + key for p in voicing]
        base = bar * BAR
        velocity = 56 + (bar - 2) * 6 if info is None else 76
        for step, tone in enumerate(shape):
            events.append((base + step * STEP, STEP - 10, tones[tone], velocity + (10 if step % 4 == 0 else 0)))
    return events


def drums():
    events = []
    for bar in range(len(SONG)):
        base = bar * BAR
        info = LAYOUT[bar]

        def hit(step, pitch, velocity, length=STEP):
            events.append((base + step * STEP, length, pitch, velocity))

        if info is None:  # intro: hats, then a four-on-the-floor build and a snare roll into the hook
            if bar < 2:
                for step in range(0, 16, 4):
                    hit(step, HAT, 55)
                continue
            for step in (0, 4, 8, 12):
                hit(step, KICK, 110)
            for step in range(0, 16, 2):
                hit(step, HAT, 70)
            if bar == len(INTRO) - 1:
                for step in range(8, 16):
                    hit(step, SNARE, 70 + step * 3)
            continue

        _, index, _, _, styles, _ = info
        style = styles["drums"]
        nxt = bar_info(bar + 1)
        last_of_section = nxt[1] == 0
        next_quiet = nxt[4]["drums"] in ("break", "drop")
        if index == 0 and style not in ("break", "drop"):
            hit(0, CRASH, 112, BAR)

        if style == "break":
            # Hats alone, then four-on-the-floor from bar 5 and a snare roll back in.
            for step in range(0, 16, 2 if index < 4 else 1):
                hit(step, HAT, 60 if step % 4 else 74)
            if index >= 4:
                for step in (0, 4, 8, 12):
                    hit(step, KICK, 104 + index * 2)
            if index == 7:
                for step in range(0, 16):
                    hit(step, SNARE, 60 + step * 4)
            continue
        if style == "drop":
            if index < 4:  # kick and bass alone
                for step in (0, 4, 8, 12):
                    hit(step, KICK, 118)
                if index == 3:
                    for step in range(8, 16):
                        hit(step, SNARE, 70 + step * 3)
                continue
            if index == 4:
                hit(0, CRASH, 112, BAR)
            style = "groove"

        if last_of_section and not next_quiet:
            # Tom fill across the last beat and a half into the next section.
            for step in (0, 6, 8):
                hit(step, KICK, 118)
            hit(4, SNARE, 112)
            for step, tom in zip(range(10, 16), (TOM_HIGH, TOM_HIGH, TOM_MID, TOM_MID, TOM_LOW, TOM_LOW)):
                hit(step, tom, 100)
            continue

        chorus = style == "chorus"
        for step in (0, 6, 8, 11) if bar % 2 == 0 else (0, 3, 8, 10):
            hit(step, KICK, 120 if step in (0, 8) else 100)
        hit(4, SNARE, 116)
        hit(12, SNARE, 116)
        hit(12, CLAP, 90)
        if bar % 4 == 3:
            hit(15, SNARE, 70)  # ghost into the next bar
        for step in range(0, 16, 1 if chorus else 2):
            pitch = OPEN_HAT if step == 14 else HAT
            hit(step, pitch, 86 if step % 4 == 0 else 66 if step % 2 == 0 else 50)
    return events


MARKERS = [(LOOP_START * BAR, "Loop start"), (len(SONG) * BAR, "Loop end")]
start = LOOP_START
for _name, _chords, *_ in SECTIONS:
    MARKERS.append((start * BAR, _name))
    start += len(_chords)

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
