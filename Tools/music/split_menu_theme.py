"""
Turns a full-song export of the menu theme from FL Studio into the two files the game plays:

  MenuTheme_Intro.wav   bars 1-4 (plays once)
  MenuTheme_Loop.wav    bars 5 to the end, looping seamlessly

The cuts land exactly on the bar lines (from the tempo and bar counts in compose_menu_theme.py). Anything after the
last bar (reverb and delay tails) is mixed back into the start of the loop, so the loop point never cuts a tail off.
The loudness is matched to what the game's mix was balanced for, keeping the peak at least 1 dB under full scale.

  C:\\ComfyUI\\.venv\\Scripts\\python.exe -I Tools/music/split_menu_theme.py [path to the full export]

Default input: SourceArt/Music/MenuTheme/Export/theme.wav, which is kept afterwards as MenuTheme_Full.wav beside the
MIDI. Writes into SourceArt/Music/MenuTheme/Export/; then run Tools/import_audio.py.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
import compose_menu_theme as menu  # noqa: E402
from wav_io import db, read, write  # noqa: E402

THEME_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "SourceArt", "Music", "MenuTheme"))
EXPORT_DIR = os.path.join(THEME_DIR, "Export")
TARGET_RMS_DB = -18.0  # the loop's average loudness the game was balanced around
CEILING_DB = -1.0


def main():
    source = sys.argv[1] if len(sys.argv) > 1 else os.path.join(EXPORT_DIR, "theme.wav")
    audio, rate = read(source)
    seconds_per_bar = 4 * 60 / menu.TEMPO_BPM
    intro_end = int(round(len(menu.INTRO) * seconds_per_bar * rate))
    song_end = int(round(len(menu.SONG) * seconds_per_bar * rate))
    if len(audio) < song_end:
        raise SystemExit(f"The export is {len(audio) / rate:.2f} s, shorter than the song's {song_end / rate:.2f} s")

    intro = audio[:intro_end].copy()
    loop = audio[intro_end:song_end].copy()
    tail = audio[song_end:]
    tail = tail[: len(loop)]
    loop[: len(tail)] += tail  # the tails ring on into the next time round

    rms = np.sqrt(np.mean(loop ** 2))
    gain_db = TARGET_RMS_DB - db(rms)
    peak = max(np.abs(loop).max(), np.abs(intro).max())
    gain_db = min(gain_db, CEILING_DB - db(peak))
    gain = 10 ** (gain_db / 20)

    write(os.path.join(EXPORT_DIR, "MenuTheme_Intro.wav"), intro * gain, rate)
    write(os.path.join(EXPORT_DIR, "MenuTheme_Loop.wav"), loop * gain, rate)
    print(f"Intro {len(intro) / rate:.3f} s, loop {len(loop) / rate:.3f} s (tail of {len(tail) / rate:.2f} s folded in), "
          f"gain {gain_db:+.1f} dB, loop RMS {db(rms) + gain_db:.1f} dB, peak {db(peak) + gain_db:.1f} dB")

    if os.path.normcase(os.path.abspath(source)) == os.path.normcase(os.path.join(EXPORT_DIR, "theme.wav")):
        kept = os.path.join(THEME_DIR, "MenuTheme_Full.wav")
        os.replace(source, kept)
        print(f"Kept the full export as {kept}")


main()
