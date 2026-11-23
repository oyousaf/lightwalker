"""
Turns FL Studio's "Split mixer tracks" export of the combat theme into the six stems the game layers by wave:

  Combat_1_Strings.wav ... Combat_6_HatRolls.wav

FL names each file after its mixer track, e.g. "combat_Collected Thoughts.wav" or "combat_Insert 2.wav". Mixer tracks
1-6 are the stems in order; a file is matched by its track number ("Insert 4") or, failing that, by its channel's name
(STEM_NAMES below). Any other WAV there (a Master track, a full mixdown) is left out of the stems.

Every stem is cut to exactly the song's 112 bars (from the tempo in compose_combat_theme.py), and anything after the
last bar (reverb and delay tails) is mixed back into the start, so each loops seamlessly and all six stay in sync. One
gain is applied to all six, so the mix balance from FL is kept: the full mix is matched to the loudness the game was
balanced around, with its peak at least 1 dB under full scale.

  C:\\ComfyUI\\.venv\\Scripts\\python.exe -I Tools/music/split_combat_stems.py

Reads FL's files from SourceArt/Music/CombatTheme/Export/, writes the stems there, then moves FL's files to
SourceArt/Music/CombatTheme/FL_Stems/ (git-ignored) so Tools/import_audio.py only sees the game's files. Then run
Tools/import_audio.py.
"""

import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
import compose_combat_theme as combat  # noqa: E402
from wav_io import db, read, write  # noqa: E402

THEME_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "SourceArt", "Music", "CombatTheme"))
EXPORT_DIR = os.path.join(THEME_DIR, "Export")
KEEP_DIR = os.path.join(THEME_DIR, "FL_Stems")
STEMS = ["Combat_1_Strings", "Combat_2_Melody", "Combat_3_808", "Combat_4_Drums", "Combat_5_Counter", "Combat_6_HatRolls"]
# Words in a mixer track's name that identify its stem, when FL names the track after its channel.
STEM_NAMES = {
    1: ["strings", "collected"],
    2: ["melody", "fairy"],
    3: ["808"],
    4: ["drums"],
    5: ["counter", "synpluc", "organic"],
    6: ["hatroll", "hat roll"],
}
TARGET_RMS_DB = -19.0  # the full mix's loudness the game was balanced around (the placeholders' sum)
CEILING_DB = -1.0
GAME_FILES = set(STEMS) | {"Sting_GameOver", "Sting_WaveCleared"}


def stem_of(name):
    lower = name.lower()
    if "master" in lower:
        return "master"
    match = re.search(r"insert\s*0*(\d+)", lower) or re.search(r"(?:^|[ _-])0*([1-6])(?:[ _-]|$)", lower)
    if match and 1 <= int(match.group(1)) <= 6:
        return int(match.group(1))
    for number, words in STEM_NAMES.items():
        if any(word in lower for word in words):
            return number
    return None


def main():
    found, others = {}, []
    for path in sorted(glob.glob(os.path.join(EXPORT_DIR, "*.wav"))):
        name = os.path.splitext(os.path.basename(path))[0]
        if name in GAME_FILES:
            continue
        stem = stem_of(name)
        if stem is None or stem == "master":
            others.append(path)
            print(f"Not a stem: {name}.wav")
            continue
        if stem in found:
            raise SystemExit(f"Both {os.path.basename(found[stem])} and {name}.wav look like stem {stem}")
        found[stem] = path
    missing = [number for number in range(1, 7) if number not in found]
    if missing:
        raise SystemExit(f"No export found for stem(s) {missing}")

    song_seconds = combat.BARS * 4 * 60 / combat.BPM
    loops, rate = [], None
    for number in range(1, 7):
        audio, file_rate = read(found[number])
        if rate not in (None, file_rate):
            raise SystemExit("The stems have different sample rates")
        rate = file_rate
        end = int(round(song_seconds * rate))
        if len(audio) < end:
            raise SystemExit(f"{os.path.basename(found[number])} is {len(audio) / rate:.2f} s, shorter than the song's {song_seconds:.2f} s")
        loop = audio[:end].copy()
        tail = audio[end:][:end]
        loop[: len(tail)] += tail  # the tails ring on into the next time round
        loops.append(loop)
        print(f"  {STEMS[number - 1]:<18} <- {os.path.basename(found[number])} (tail {len(tail) / rate:.2f} s, RMS {db(np.sqrt(np.mean(loop ** 2))):.1f} dB)")

    mix = np.sum(loops, axis=0)
    gain_db = TARGET_RMS_DB - db(np.sqrt(np.mean(mix ** 2)))
    peak = max(np.abs(mix).max(), max(np.abs(loop).max() for loop in loops))
    gain_db = min(gain_db, CEILING_DB - db(peak))
    gain = 10 ** (gain_db / 20)
    for name, loop in zip(STEMS, loops):
        write(os.path.join(EXPORT_DIR, name + ".wav"), loop * gain, rate)
    print(f"Wrote six {song_seconds:.3f} s stems, gain {gain_db:+.1f} dB: mix RMS "
          f"{db(np.sqrt(np.mean(mix ** 2))) + gain_db:.1f} dB, peak {db(np.abs(mix).max()) + gain_db:.1f} dB")

    os.makedirs(KEEP_DIR, exist_ok=True)
    for path in list(found.values()) + others:
        os.replace(path, os.path.join(KEEP_DIR, os.path.basename(path)))
    print(f"Moved FL's files to {KEEP_DIR}")


main()
