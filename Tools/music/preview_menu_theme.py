"""
Renders a rough preview of the menu theme with simple built-in synths (FM bass, square lead, noise drums),
so the tune can be judged before building it properly in FL Studio. Not the final sound.

  python Tools/music/preview_menu_theme.py   ->  SourceArt/Music/MenuTheme/MenuTheme_Preview.wav
"""

import math
import os
import random
import struct
import sys
import wave

sys.path.insert(0, os.path.dirname(__file__))
import compose_menu_theme as theme  # noqa: E402

RATE = 32000
SECONDS_PER_TICK = 60.0 / theme.TEMPO_BPM / theme.PPQ


def freq(pitch):
    return 440.0 * 2 ** ((pitch - 69) / 12)


def render(events, voice, gain, buffer):
    for tick, length, pitch, velocity in events:
        start = int(tick * SECONDS_PER_TICK * RATE)
        count = int(length * SECONDS_PER_TICK * RATE)
        amp = gain * velocity / 127
        for i, sample in enumerate(voice(pitch, count)):
            index = start + i
            if index < len(buffer):
                buffer[index] += sample * amp


def envelope(i, count, attack=0.004, release=0.03):
    a = min(1.0, i / (attack * RATE))
    r = min(1.0, (count - i) / (release * RATE)) if count > i else 0.0
    return a * r


def square(pitch, count):
    f = freq(pitch)
    for i in range(count + int(0.03 * RATE)):
        phase = (i * f / RATE) % 1.0
        vibrato = 1.0
        yield (1.0 if phase < 0.5 else -1.0) * envelope(i, count + int(0.03 * RATE)) * vibrato * 0.5


def fm_bass(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        index = 3.0 * math.exp(-t * 18)  # bright slap at the start, mellowing quickly
        yield math.sin(2 * math.pi * f * t + index * math.sin(2 * math.pi * f * 2 * t)) * envelope(i, count)


def pluck(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        yield (2 * ((t * f) % 1.0) - 1) * math.exp(-t * 14) * envelope(i, count) * 0.6


def pad(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        yield (2 * ((t * f) % 1.0) - 1 + 2 * ((t * f * 1.004) % 1.0) - 1) * 0.25 * envelope(i, count, attack=0.3, release=0.2)


def drum(pitch, count):
    length = int(0.25 * RATE)
    for i in range(length):
        t = i / RATE
        if pitch == theme.KICK:
            yield math.sin(2 * math.pi * (50 + 120 * math.exp(-t * 30)) * t) * math.exp(-t * 9)
        elif pitch in (theme.SNARE, theme.CLAP):
            yield (random.uniform(-1, 1) * 0.7 + math.sin(2 * math.pi * 190 * t) * 0.3) * math.exp(-t * 18)
        elif pitch in (theme.HAT, theme.OPEN_HAT):
            decay = 10 if pitch == theme.OPEN_HAT else 45
            yield random.uniform(-1, 1) * math.exp(-t * decay) * 0.35
        elif pitch == theme.CRASH:
            yield random.uniform(-1, 1) * math.exp(-t * 4) * 0.4
        else:  # toms
            yield math.sin(2 * math.pi * (freq(pitch) * 0.5) * t) * math.exp(-t * 12)


if __name__ == "__main__":
    random.seed(1)
    total = int((len(theme.SONG) * theme.BAR + theme.PPQ) * SECONDS_PER_TICK * RATE)
    mix = [0.0] * total
    stab_events = theme.stabs()
    intro_end = len(theme.INTRO) * theme.BAR
    render(theme.lead(), square, 0.16, mix)
    render(theme.bass(), fm_bass, 0.42, mix)
    render([e for e in stab_events if e[0] >= intro_end], pluck, 0.10, mix)
    render([e for e in stab_events if e[0] < intro_end], pad, 0.12, mix)
    render(theme.arp(), pluck, 0.08, mix)
    render(theme.drums(), drum, 0.32, mix)

    peak = max(1e-6, max(abs(s) for s in mix))
    path = os.path.join(theme.OUT_DIR, "MenuTheme_Preview.wav")
    with wave.open(path, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b"".join(struct.pack("<h", int(s / peak * 0.9 * 32767)) for s in mix))
    print("Wrote", os.path.abspath(path))
