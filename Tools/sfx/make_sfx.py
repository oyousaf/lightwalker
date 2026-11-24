"""
Synthesises every sound effect from scratch (no samples, no packages), in a neon-arcade style to match the music.

  python Tools/sfx/make_sfx.py   ->  SourceArt/Audio/SFX/*.wav  (48 kHz mono)

Then import with Tools/import_audio.py. Each sound is a small function below; tweak and re-run to change one.
"""

import array
import math
import os
import random
import wave

RATE = 48000
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "SourceArt", "Audio", "SFX")
TAU = 2 * math.pi


# ---------------------------------------------------------------------------------------------------------------
# Building blocks: each returns a list of floats

def silence(seconds):
    return [0.0] * int(seconds * RATE)


def mix(*layers):
    length = max(len(layer) for layer in layers)
    out = [0.0] * length
    for layer in layers:
        for i, value in enumerate(layer):
            out[i] += value
    return out


def gain(samples, amount):
    return [s * amount for s in samples]


def delay(samples, seconds):
    return silence(seconds) + samples


def env_exp(count, rate):
    return [math.exp(-i / RATE * rate) for i in range(count)]


def env_adsr(count, attack, release):
    a, r = int(attack * RATE), int(release * RATE)
    return [min(1.0, i / max(1, a)) * min(1.0, (count - i) / max(1, r)) for i in range(count)]


def apply(samples, envelope):
    return [s * e for s, e in zip(samples, envelope)]


def sweep_osc(seconds, start_hz, end_hz, shape="sine", curve=2.0, duty=0.5):
    """An oscillator whose pitch glides from start to end (exponential-ish when curve > 1)."""
    count = int(seconds * RATE)
    out, phase = [], 0.0
    for i in range(count):
        t = i / max(1, count - 1)
        f = start_hz + (end_hz - start_hz) * (1 - (1 - t) ** curve)
        phase = (phase + f / RATE) % 1.0
        if shape == "sine":
            out.append(math.sin(TAU * phase))
        elif shape == "saw":
            out.append(2 * phase - 1)
        elif shape == "square":
            out.append(1.0 if phase < duty else -1.0)
        else:  # triangle
            out.append(4 * abs(phase - 0.5) - 1)
    return out


def partials(seconds, base_hz, ratios, decays, amps=None):
    """Inharmonic bell or metal: several sines, each with its own decay."""
    count = int(seconds * RATE)
    amps = amps or [1.0] * len(ratios)
    out = [0.0] * count
    for ratio, decay, amp in zip(ratios, decays, amps):
        f = base_hz * ratio
        for i in range(count):
            t = i / RATE
            out[i] += math.sin(TAU * f * t) * math.exp(-t * decay) * amp
    return out


def noise(seconds, seed=0):
    rng = random.Random(seed)
    return [rng.uniform(-1, 1) for _ in range(int(seconds * RATE))]


def lowpass(samples, cutoff_start, cutoff_end=None):
    cutoff_end = cutoff_end or cutoff_start
    out, y = [], 0.0
    for i, x in enumerate(samples):
        cutoff = cutoff_start + (cutoff_end - cutoff_start) * i / max(1, len(samples) - 1)
        a = 1 - math.exp(-TAU * cutoff / RATE)
        y += a * (x - y)
        out.append(y)
    return out


def highpass(samples, cutoff):
    low = lowpass(samples, cutoff)
    return [x - l for x, l in zip(samples, low)]


def bandpass(samples, centre_start, centre_end=None, q=4.0):
    """State-variable band-pass with a sweepable centre."""
    centre_end = centre_end or centre_start
    out, low, band = [], 0.0, 0.0
    for i, x in enumerate(samples):
        centre = centre_start + (centre_end - centre_start) * i / max(1, len(samples) - 1)
        f = 2 * math.sin(math.pi * min(centre, RATE / 6) / RATE)
        high = x - low - band / q
        band += f * high
        low += f * band
        out.append(band)
    return out


def saturate(samples, drive):
    return [math.tanh(s * drive) / math.tanh(drive) for s in samples]


def save(name, samples, level=0.85):
    os.makedirs(OUT_DIR, exist_ok=True)
    fade = int(0.004 * RATE)  # no clicks at the ends
    for i in range(min(fade, len(samples))):
        samples[-1 - i] *= i / fade
    peak = max(1e-6, max(abs(s) for s in samples))
    data = array.array("h", (int(s / peak * level * 32767) for s in samples))
    with wave.open(os.path.join(OUT_DIR, f"{name}.wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(data.tobytes())
    print(f"  {name:<16} {len(samples) / RATE:.2f} s")


# ---------------------------------------------------------------------------------------------------------------
# The sounds

def shot():
    # A soft laser zap: a quick downward sweep with a little air.
    tone = apply(sweep_osc(0.11, 1900, 520, "square", curve=3, duty=0.35), env_exp(int(0.11 * RATE), 30))
    air = apply(highpass(noise(0.05, 1), 3000), env_exp(int(0.05 * RATE), 80))
    return mix(lowpass(tone, 5000, 1800), gain(air, 0.15))


def shot_spread():
    # Three slightly detuned zaps at once, a touch lower and wider.
    layers = [apply(sweep_osc(0.13, 1500 * d, 430 * d, "saw", curve=3), env_exp(int(0.13 * RATE), 26)) for d in (0.97, 1.0, 1.04)]
    return lowpass(mix(*layers), 4200, 1500)


def shot_piercing():
    # Longer and brighter, with a ringing shimmer: a beam rather than a bolt.
    count = int(0.22 * RATE)
    beam = apply(sweep_osc(0.22, 2600, 900, "saw", curve=2.5), env_exp(count, 12))
    ring = apply(sweep_osc(0.22, 5200, 3600, "sine"), env_exp(count, 18))
    return mix(lowpass(beam, 7000, 2400), gain(ring, 0.35))


def shaytaan_death():
    # A squelchy pop: a fast pitch drop with a wet noise burst.
    count = int(0.22 * RATE)
    body = apply(sweep_osc(0.22, 700, 90, "sine", curve=4), env_exp(count, 16))
    squelch = apply(bandpass(noise(0.12, 2), 1400, 300, q=3), env_exp(int(0.12 * RATE), 30))
    return saturate(mix(body, gain(squelch, 1.6)), 1.6)


def dajjal_hit():
    # A metallic ping: inharmonic partials, like striking hollow gold.
    return mix(partials(0.35, 880, [1, 2.76, 5.4, 8.93], [14, 18, 26, 40], [1, 0.6, 0.35, 0.2]),
               gain(apply(highpass(noise(0.01, 3), 4000), env_exp(int(0.01 * RATE), 300)), 0.4))


def dajjal_death():
    # A glassy shatter over a low thump: many short high tinkles scattered over a third of a second.
    rng = random.Random(4)
    shards = [delay(partials(0.25, rng.uniform(2500, 6500), [1, 2.3], [30, 45], [1, 0.4]), rng.uniform(0, 0.3)) for _ in range(18)]
    crash = apply(highpass(noise(0.6, 5), 2500), env_exp(int(0.6 * RATE), 7))
    thump = apply(sweep_osc(0.3, 160, 45, "sine"), env_exp(int(0.3 * RATE), 10))
    return mix(gain(mix(*shards), 0.25), gain(crash, 0.5), thump)


def life_lost():
    # A dark falling tone: detuned saws dropping two octaves while the filter closes.
    count = int(1.1 * RATE)
    falls = [sweep_osc(1.1, 440 * d, 110 * d, "saw", curve=1.2) for d in (1.0, 1.012, 0.988)]
    sub = sweep_osc(1.1, 110, 41, "sine", curve=1.2)
    return apply(mix(lowpass(mix(*falls), 3500, 250), gain(sub, 0.8)), env_adsr(count, 0.01, 0.4))


def respawn():
    # A rising shimmer: a quick bright arpeggio.
    notes = [523.25, 659.25, 783.99, 1046.5, 1318.5]
    return mix(*[delay(partials(0.5, f, [1, 2, 3], [7, 10, 14], [1, 0.3, 0.12]), i * 0.05) for i, f in enumerate(notes)])


def shield_block():
    # A bright clang with a click: the shield taking the hit.
    clang = partials(0.6, 690, [1, 2.4, 3.9, 6.1], [6, 9, 14, 22], [1, 0.7, 0.5, 0.3])
    click = apply(noise(0.015, 6), env_exp(int(0.015 * RATE), 250))
    return saturate(mix(clang, gain(click, 0.8)), 1.3)


def glide():
    # A soft whoosh: noise through a band-pass sweeping up and back down.
    up = apply(bandpass(noise(0.35, 7), 300, 2200, q=2.5), env_adsr(int(0.35 * RATE), 0.2, 0.05))
    down = apply(bandpass(noise(0.4, 8), 2200, 500, q=2.5), env_adsr(int(0.4 * RATE), 0.02, 0.35))
    return up + down


def power_up():
    # A two-note bell chime, up a fifth.
    return mix(partials(0.6, 1318.5, [1, 2.01, 3.0], [6, 9, 14], [1, 0.4, 0.2]),
               delay(partials(0.7, 1975.5, [1, 2.01, 3.0], [5, 8, 12], [1, 0.4, 0.2]), 0.08))


def power_down():
    # The power-up reversed and dulled: a falling two-note fade.
    return mix(partials(0.4, 1318.5, [1, 2.01], [9, 14], [0.7, 0.2]),
               delay(partials(0.5, 880, [1, 2.01], [7, 11], [0.6, 0.15]), 0.1))


def book():
    # A page turn (paper flicks), then a soft choir-like swell on an A major chord.
    flick = mix(*[delay(apply(bandpass(noise(0.06, 10 + i), 2500 + 900 * i, 1500, q=1.5), env_exp(int(0.06 * RATE), 40)), i * 0.05) for i in range(4)])
    voices = []
    for f in (220.0, 277.18, 329.63, 440.0, 554.37):
        for detune in (0.996, 1.004):
            tone = sweep_osc(1.6, f * detune, f * detune, "saw")
            voices.append(bandpass(tone, 750, 750, q=2.0))  # an "ah"-like formant
    swell = apply(mix(*voices), env_adsr(int(1.6 * RATE), 0.45, 0.9))
    return mix(gain(flick, 1.2), delay(gain(swell, 0.6), 0.15))


def wave_start():
    # A rising alarm-like sweep that opens up.
    count = int(0.7 * RATE)
    rise = apply(sweep_osc(0.7, 180, 900, "saw", curve=0.8), env_adsr(count, 0.05, 0.2))
    return lowpass(mix(rise, gain(sweep_osc(0.7, 90, 450, "square", curve=0.8), 0.4)), 600, 6000)


def jump():
    # A soft springy rise: robes and air rather than an arcade boing.
    tone = apply(sweep_osc(0.16, 240, 620, "triangle", curve=1.4), env_exp(int(0.16 * RATE), 16))
    air = apply(bandpass(noise(0.14, 30), 500, 1800, q=1.5), env_adsr(int(0.14 * RATE), 0.03, 0.1))
    return mix(tone, gain(air, 0.7))


def double_jump():
    # The same push, with a sparkle of light under it.
    tone = apply(sweep_osc(0.2, 340, 900, "triangle", curve=1.4), env_exp(int(0.2 * RATE), 13))
    sparkle = mix(*[delay(partials(0.22, f, [1, 2.01], [16, 24], [1, 0.3]), i * 0.025) for i, f in enumerate((1568, 2093, 2637))])
    return mix(tone, gain(sparkle, 0.3))


def land():
    thump = apply(sweep_osc(0.13, 130, 50, "sine"), env_exp(int(0.13 * RATE), 28))
    scuff = apply(lowpass(noise(0.09, 31), 1600, 500), env_exp(int(0.09 * RATE), 38))
    return mix(thump, gain(scuff, 0.5))


def orb():
    # A small glassy chime.
    return mix(partials(0.35, 1760, [1, 2.0, 3.01], [11, 16, 24], [1, 0.35, 0.15]),
               delay(partials(0.4, 2637, [1, 2.0], [10, 16], [0.7, 0.2]), 0.045))


def checkpoint():
    # The lantern catching: a breath of flame, then a warm open chord.
    flame = apply(bandpass(noise(0.5, 32), 300, 1400, q=1.2), env_adsr(int(0.5 * RATE), 0.12, 0.35))
    chord = [delay(partials(1.1, f, [1, 2, 3], [3.5, 5, 8], [1, 0.3, 0.1]), 0.1 + i * 0.07) for i, f in enumerate((440, 554.37, 659.25, 880))]
    return mix(gain(flame, 0.8), gain(mix(*chord), 0.45))


def light_low():
    # A heartbeat: two low thuds.
    def thud(level):
        return gain(apply(sweep_osc(0.2, 85, 42, "sine"), env_exp(int(0.2 * RATE), 20)), level)
    return saturate(mix(thud(1.0), delay(thud(0.7), 0.17)), 1.5)


def gate():
    # Walking into the light: a slow swell that opens up over a rising chord.
    count = int(2.4 * RATE)
    voices = []
    for f in (220, 277.18, 329.63, 440, 659.25):
        for detune in (0.995, 1.005):
            voices.append(sweep_osc(2.4, f * detune, f * detune, "saw"))
    swell = apply(lowpass(mix(*voices), 400, 5000), env_adsr(count, 0.9, 1.2))
    bells = [delay(partials(1.4, f, [1, 2.01, 3.0], [3, 5, 8], [1, 0.4, 0.2]), 0.35 + i * 0.16) for i, f in enumerate((880, 1108.7, 1318.5, 1760, 2217.5))]
    return mix(gain(swell, 0.14), gain(mix(*bells), 0.35))


def deflect():
    # The beam ringing off the shield: a clear struck-glass note.
    ring = partials(0.5, 1046.5, [1, 2.0, 2.99, 4.2], [8, 11, 16, 24], [1, 0.45, 0.25, 0.12])
    strike = apply(highpass(noise(0.012, 40), 3500), env_exp(int(0.012 * RATE), 260))
    return mix(ring, gain(strike, 0.5))


def wall_hit():
    # A dull knock: stone that held.
    knock = apply(sweep_osc(0.14, 320, 120, "triangle"), env_exp(int(0.14 * RATE), 26))
    grit = apply(bandpass(noise(0.1, 41), 1800, 700, q=1.5), env_exp(int(0.1 * RATE), 40))
    return mix(knock, gain(grit, 0.6))


def wall_break():
    # Darkness giving way: a crumble with a spark of light let out of it.
    crumble = apply(lowpass(noise(0.28, 42), 2600, 500), env_exp(int(0.28 * RATE), 14))
    thump = apply(sweep_osc(0.18, 180, 60, "sine"), env_exp(int(0.18 * RATE), 18))
    spark = delay(partials(0.3, 2093, [1, 1.5, 2.0], [14, 18, 22], [0.6, 0.4, 0.3]), 0.02)
    return mix(gain(crumble, 0.8), thump, gain(spark, 0.35))


def dark_bolt():
    # A low hollow pulse from the wall.
    count = int(0.45 * RATE)
    return lowpass(apply(mix(sweep_osc(0.45, 300, 90, "saw", curve=2), sweep_osc(0.45, 304, 92, "saw", curve=2)), env_adsr(count, 0.01, 0.35)), 1500, 300)


def beam_lost():
    # The light falling away: a glassy slide down into nothing.
    count = int(0.9 * RATE)
    slide = apply(sweep_osc(0.9, 1400, 120, "triangle", curve=1.3), env_adsr(count, 0.005, 0.6))
    air = apply(bandpass(noise(0.9, 43), 2400, 250, q=2), env_adsr(count, 0.05, 0.6))
    return mix(slide, gain(air, 0.6))


def eye_break():
    # The great eye: a deep blow, a long shatter, then a rising chord.
    boom = apply(sweep_osc(1.0, 110, 32, "sine"), env_exp(int(1.0 * RATE), 4))
    shatter = apply(highpass(noise(1.1, 44), 1800), env_exp(int(1.1 * RATE), 4.5))
    chord = [delay(partials(1.6, f, [1, 2.01, 3], [3, 5, 8], [1, 0.35, 0.15]), 0.3 + i * 0.09) for i, f in enumerate((440, 554.37, 659.25, 880, 1318.5))]
    return saturate(mix(boom, gain(shatter, 0.4), gain(mix(*chord), 0.3)), 1.2)


def ui_move():
    return apply(sweep_osc(0.035, 1900, 1700, "triangle"), env_exp(int(0.035 * RATE), 90))


def ui_select():
    first = apply(sweep_osc(0.06, 1200, 1200, "square", duty=0.3), env_exp(int(0.06 * RATE), 40))
    second = apply(sweep_osc(0.09, 1800, 1800, "square", duty=0.3), env_exp(int(0.09 * RATE), 30))
    return lowpass(first + second, 6000)


def ui_play():
    # The camera sweep into play: a long rising whoosh with a tone underneath.
    whoosh = apply(bandpass(noise(1.4, 20), 200, 4000, q=2.0), env_adsr(int(1.4 * RATE), 0.6, 0.6))
    tone = apply(sweep_osc(1.4, 110, 440, "saw", curve=0.7), env_adsr(int(1.4 * RATE), 0.4, 0.7))
    return mix(whoosh, gain(lowpass(tone, 400, 3000), 0.5))


SOUNDS = {
    "Shot": shot, "ShotSpread": shot_spread, "ShotPiercing": shot_piercing,
    "ShaytaanDeath": shaytaan_death, "DajjalHit": dajjal_hit, "DajjalDeath": dajjal_death,
    "LifeLost": life_lost, "Respawn": respawn, "ShieldBlock": shield_block, "Glide": glide,
    "PowerUp": power_up, "PowerDown": power_down, "Book": book, "WaveStart": wave_start,
    "Jump": jump, "DoubleJump": double_jump, "Land": land, "Orb": orb,
    "Checkpoint": checkpoint, "LightLow": light_low, "Gate": gate,
    "Deflect": deflect, "WallHit": wall_hit, "WallBreak": wall_break,
    "DarkBolt": dark_bolt, "BeamLost": beam_lost, "EyeBreak": eye_break,
    "UIMove": ui_move, "UISelect": ui_select, "UIPlay": ui_play,
}

if __name__ == "__main__":
    print(f"Writing {len(SOUNDS)} sounds to {os.path.abspath(OUT_DIR)}")
    for name, make in SOUNDS.items():
        save(f"SFX_{name}", make())
