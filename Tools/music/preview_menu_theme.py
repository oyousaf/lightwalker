"""
Renders a rough preview of the menu theme with the crude built-in synths in synth.py, so the tune can be
judged before building it properly in FL Studio. Not the final sound.

  python Tools/music/preview_menu_theme.py   ->  SourceArt/Music/MenuTheme/MenuTheme_Preview.wav
"""

import os
import random
import sys

sys.path.insert(0, os.path.dirname(__file__))
import compose_menu_theme as theme  # noqa: E402
import midi_writer as kit  # noqa: E402
import synth  # noqa: E402

if __name__ == "__main__":
    random.seed(1)
    mix = synth.Mix(len(theme.SONG) * 4 * 60 / theme.TEMPO_BPM + 0.5, theme.TEMPO_BPM, theme.PPQ)
    stab_events = theme.stabs()
    intro_end = len(theme.INTRO) * theme.BAR
    mix.add(theme.lead(), synth.square, 0.16)
    mix.add(theme.bass(), synth.fm_bass, 0.42)
    mix.add([e for e in stab_events if e[0] >= intro_end], synth.pluck, 0.10)
    mix.add([e for e in stab_events if e[0] < intro_end], synth.pad, 0.12)
    mix.add(theme.arp(), synth.pluck, 0.08)
    mix.add(theme.drums(), lambda pitch, count: synth.drum(pitch, count, kit), 0.32)
    path = os.path.join(theme.OUT_DIR, "MenuTheme_Preview.wav")
    mix.save(path)
    print("Wrote", os.path.abspath(path))
