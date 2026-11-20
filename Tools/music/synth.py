"""
Crude built-in synths for rough audio previews of the compose scripts (pure Python, no packages).
Only for judging a tune before it's built properly in FL Studio.
"""

import math
import random
import struct
import wave

RATE = 32000


def freq(pitch):
    return 440.0 * 2 ** ((pitch - 69) / 12)


def envelope(i, count, attack=0.004, release=0.03):
    a = min(1.0, i / max(1.0, attack * RATE))
    r = min(1.0, (count - i) / max(1.0, release * RATE)) if count > i else 0.0
    return a * r


def square(pitch, count):
    f = freq(pitch)
    total = count + int(0.03 * RATE)
    for i in range(total):
        yield (1.0 if (i * f / RATE) % 1.0 < 0.5 else -1.0) * envelope(i, total) * 0.5


def fm_bass(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        index = 3.0 * math.exp(-t * 18)
        yield math.sin(2 * math.pi * f * t + index * math.sin(2 * math.pi * f * 2 * t)) * envelope(i, count)


def pluck(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        yield (2 * ((t * f) % 1.0) - 1) * math.exp(-t * 14) * envelope(i, count) * 0.6


def bell(pitch, count):
    f = freq(pitch)
    total = count + int(0.4 * RATE)
    for i in range(total):
        t = i / RATE
        tone = math.sin(2 * math.pi * f * t + 1.5 * math.exp(-t * 6) * math.sin(2 * math.pi * f * 3.5 * t))
        yield tone * math.exp(-t * 3.5) * envelope(i, total, release=0.1) * 0.7


def pad(pitch, count):
    f = freq(pitch)
    for i in range(count):
        t = i / RATE
        saw = (2 * ((t * f) % 1.0) - 1) + (2 * ((t * f * 1.005) % 1.0) - 1)
        yield saw * 0.25 * envelope(i, count, attack=0.35, release=0.25)


def sub808(pitch, count):
    """Sine 808 with a click, a slight downward pitch drop and long sustain."""
    f = freq(pitch)
    phase = 0.0
    for i in range(count):
        t = i / RATE
        phase += 2 * math.pi * f * (1 + 0.6 * math.exp(-t * 40)) / RATE
        tone = math.sin(phase)
        tone = math.tanh(tone * 1.8)  # a little saturation, like a driven 808
        yield tone * envelope(i, count, attack=0.002, release=0.05)


def drum(pitch, count, kit):
    length = int(0.3 * RATE)
    for i in range(length):
        t = i / RATE
        if pitch == kit.KICK:
            yield math.sin(2 * math.pi * (45 + 110 * math.exp(-t * 35)) * t) * math.exp(-t * 10)
        elif pitch in (kit.SNARE, kit.CLAP):
            yield (random.uniform(-1, 1) * 0.75 + math.sin(2 * math.pi * 200 * t) * 0.25) * math.exp(-t * 16)
        elif pitch == kit.RIM:
            yield math.sin(2 * math.pi * 1700 * t) * math.exp(-t * 60) * 0.6
        elif pitch in (kit.HAT, kit.OPEN_HAT):
            decay = 10 if pitch == kit.OPEN_HAT else 55
            yield random.uniform(-1, 1) * math.exp(-t * decay) * 0.3
        elif pitch == kit.CRASH:
            yield random.uniform(-1, 1) * math.exp(-t * 4) * 0.35
        else:
            yield math.sin(2 * math.pi * freq(pitch) * 0.5 * t) * math.exp(-t * 12)


class Mix:
    def __init__(self, seconds, bpm, ppq):
        self.samples = [0.0] * int(seconds * RATE)
        self.seconds_per_tick = 60.0 / bpm / ppq

    def add(self, events, voice, gain):
        for tick, length, pitch, velocity in events:
            start = int(tick * self.seconds_per_tick * RATE)
            count = int(length * self.seconds_per_tick * RATE)
            amp = gain * velocity / 127
            for i, sample in enumerate(voice(pitch, count)):
                index = start + i
                if index >= len(self.samples):
                    break
                self.samples[index] += sample * amp

    def save(self, path):
        peak = max(1e-6, max(abs(s) for s in self.samples))
        with wave.open(path, "wb") as out:
            out.setnchannels(1)
            out.setsampwidth(2)
            out.setframerate(RATE)
            out.writeframes(b"".join(struct.pack("<h", int(s / peak * 0.9 * 32767)) for s in self.samples))
