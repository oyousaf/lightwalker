"""
Renders both themes to the WAV files the game plays, from the compose scripts' MIDI, using FluidSynth with the
GeneralUser GS SoundFont (free for commercial music) and a built-in sliding 808 for the drill bass.

These are placeholders in the final layout: replacing any file in Export/ with an FL Studio export of the same
name swaps it in with no code changes.

  SourceArt/Music/MenuTheme/Export/MenuTheme_Intro.wav, MenuTheme_Loop.wav
  SourceArt/Music/CombatTheme/Export/Combat_1_Strings.wav ... Combat_6_HatRolls.wav, Sting_*.wav

Needs (outside the repo): FluidSynth and the SoundFont. Paths can be overridden with LW_FLUIDSYNTH and LW_SOUNDFONT.
  python Tools/music/render_music.py
"""

import array
import math
import os
import subprocess
import sys
import tempfile
import wave

sys.path.insert(0, os.path.dirname(__file__))
import compose_combat_theme as combat  # noqa: E402
import compose_menu_theme as menu  # noqa: E402
from midi_writer import BAR, PPQ, track_chunk, write  # noqa: E402

FLUIDSYNTH = os.environ.get("LW_FLUIDSYNTH", r"D:\AI\Tools\audio\fluidsynth\fluidsynth-v2.6.2-win10-x64-cpp11\bin\fluidsynth.exe")
SOUNDFONT = os.environ.get("LW_SOUNDFONT", r"D:\AI\Tools\audio\soundfonts\GeneralUser-GS.sf2")
RATE = 48000
PEAK = 0.89  # about -1 dB

# General MIDI programs for each part (drum channel 9 picks a kit: 0 standard, 25 TR-808).
MENU_PROGRAMS = {"Lead": 80, "Bass": 36, "Stabs": 62, "Arp": 98, "Drums": 0}
COMBAT_PROGRAMS = {"Strings": 49, "Melody": 0, "Drums": 25, "Counter": 10, "HatRolls": 25}

# The mix: each combat stem's average loudness relative to the others (dB RMS). The 808 and drums lead, the
# melody sits on top, strings and the high layers stay behind.
COMBAT_LEVELS_DB = {"Strings": -24.0, "Melody": -20.0, "808": -17.0, "Drums": -19.0, "Counter": -27.0, "HatRolls": -27.0}


# ---------------------------------------------------------------------------------------------------------------
# Audio helpers (stereo float lists of [left, right] interleaved)

def seconds_to_samples(seconds):
    return int(round(seconds * RATE))


def ticks_to_samples(ticks, bpm):
    return seconds_to_samples(ticks * 60.0 / bpm / PPQ)


def fluid_render(tracks, bpm, length_samples):
    """tracks: (name, events, channel, program). Returns interleaved stereo floats, padded or cut to length."""
    with tempfile.TemporaryDirectory() as folder:
        midi_path = os.path.join(folder, "in.mid")
        wav_path = os.path.join(folder, "out.wav")
        write(midi_path, bpm, [track_chunk(name, events, channel, program) for name, events, channel, program in tracks])
        subprocess.run([FLUIDSYNTH, "-ni", "-q", "-g", "0.6", "-r", str(RATE), "-F", wav_path, SOUNDFONT, midi_path],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return read_wav(wav_path, length_samples)


def read_wav(path, length_samples):
    with wave.open(path, "rb") as source:
        assert source.getsampwidth() == 2 and source.getnchannels() == 2, "expected 16-bit stereo"
        data = array.array("h", source.readframes(source.getnframes()))
    samples = [s / 32768.0 for s in data]
    wanted = length_samples * 2
    return samples[:wanted] + [0.0] * max(0, wanted - len(samples))


def write_wav(path, samples, gain):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    data = array.array("h", (max(-32767, min(32767, int(s * gain * 32767))) for s in samples))
    with wave.open(path, "wb") as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(data.tobytes())


def wrap_loop(samples, start, end):
    """The loop from start to end, with everything after end (reverb, release tails) folded back onto its start."""
    loop = samples[start * 2:end * 2]
    tail = samples[end * 2:]
    for index in range(min(len(tail), len(loop))):
        loop[index] += tail[index]
    return loop


def rms_db(samples):
    energy = sum(s * s for s in samples) / max(1, len(samples))
    return 10 * math.log10(max(energy, 1e-12))


def peak(*tracks):
    if not tracks:
        return 1e-6
    return max(1e-6, max(abs(sum(values)) for values in zip(*tracks)))


# ---------------------------------------------------------------------------------------------------------------
# The drill 808: a sine with saturation that glides when notes overlap, like FL's Porta.

def render_808(events, bpm, length_samples, glide_seconds=0.075):
    out = [0.0] * (length_samples * 2)
    events = sorted(events)
    notes = [(ticks_to_samples(t, bpm), ticks_to_samples(t + l, bpm), p, v) for t, l, p, v in events]
    phase = 0.0
    frequency = 0.0
    for index, (start, end, pitch, velocity) in enumerate(notes):
        target = 440.0 * 2 ** ((pitch - 69) / 12)
        previous_end = notes[index - 1][1] if index else -1
        sliding = index > 0 and previous_end > start
        if not sliding:
            frequency = target
        from_frequency = frequency
        next_start = notes[index + 1][0] if index + 1 < len(notes) else end
        stop = min(max(end, next_start if next_start < end else end), length_samples)
        amp = velocity / 127
        for sample in range(start, stop):
            t = (sample - start) / RATE
            if sliding:
                blend = min(1.0, t / glide_seconds)
                frequency = from_frequency + (target - from_frequency) * (1 - (1 - blend) ** 2)
            else:
                frequency = target * (1 + 0.5 * math.exp(-t * 45))  # punchy pitch drop on a fresh hit
            phase += 2 * math.pi * frequency / RATE
            attack = 1.0 if sliding else min(1.0, t / 0.002)
            release = min(1.0, (stop - sample) / (0.04 * RATE))
            value = math.tanh(math.sin(phase) * 2.2) * 0.8 * amp * attack * release
            out[sample * 2] += value
            out[sample * 2 + 1] += value
        frequency = target
    return out


# ---------------------------------------------------------------------------------------------------------------

def render_menu():
    bpm = menu.TEMPO_BPM
    tail = seconds_to_samples(3.0)
    loop_start = ticks_to_samples(len(menu.INTRO) * BAR, bpm)
    loop_end = ticks_to_samples(len(menu.SONG) * BAR, bpm)
    tracks = [(name, build(), channel, MENU_PROGRAMS[name]) for name, build, channel in menu.PARTS]
    song = fluid_render(tracks, bpm, loop_end + tail)
    intro = song[:loop_start * 2]
    loop = wrap_loop(song, loop_start, loop_end)
    gain = PEAK / peak(song)
    folder = os.path.join(menu.OUT_DIR, "Export")
    write_wav(os.path.join(folder, "MenuTheme_Intro.wav"), intro, gain)
    write_wav(os.path.join(folder, "MenuTheme_Loop.wav"), loop, gain)
    print(f"Menu: intro {loop_start / RATE:.2f} s, loop {(loop_end - loop_start) / RATE:.2f} s")


def render_combat():
    bpm = combat.BPM
    loop_end = ticks_to_samples(combat.BARS * BAR, bpm)
    length = loop_end + seconds_to_samples(3.0)

    stems = []
    for index, (name, build, channel) in enumerate(combat.PARTS, start=1):
        if name == "808":
            audio = render_808(build(), bpm, length)
        else:
            audio = fluid_render([(name, build(), channel, COMBAT_PROGRAMS[name])], bpm, length)
        loop = wrap_loop(audio, 0, loop_end)
        trim = 10 ** ((COMBAT_LEVELS_DB[name] - rms_db(loop)) / 20)
        stems.append((f"Combat_{index}_{name}.wav", [s * trim for s in loop]))

    # Each stem is set to its own loudness above (the mix balance), then one gain for every stem keeps that
    # balance when the game layers them.
    gain = PEAK / peak(*[audio for _, audio in stems])
    folder = os.path.join(combat.OUT_DIR, "Export")
    for filename, audio in stems:
        write_wav(os.path.join(folder, filename), audio, gain)

    sting_length = ticks_to_samples(3 * BAR, bpm)
    cleared = fluid_render([("WaveCleared", combat.sting_wave_cleared(), 1, 10)], bpm, sting_length)
    game_over_events = combat.sting_game_over()
    low = [e for e in game_over_events if e[2] < 40]
    game_over = fluid_render([("GameOver", [e for e in game_over_events if e[2] >= 40], 1, 0)], bpm, sting_length)
    bass = render_808(low, bpm, sting_length)
    game_over = [a + b * 0.6 for a, b in zip(game_over, bass)]
    for filename, audio in (("Sting_WaveCleared.wav", cleared), ("Sting_GameOver.wav", game_over)):
        write_wav(os.path.join(folder, filename), audio, PEAK / peak(audio) * 0.8)
    print(f"Combat: {len(stems)} stems of {loop_end / RATE:.2f} s, 2 stings")


if __name__ == "__main__":
    for tool in (FLUIDSYNTH, SOUNDFONT):
        if not os.path.isfile(tool):
            sys.exit(f"Missing {tool} (set LW_FLUIDSYNTH / LW_SOUNDFONT)")
    render_menu()
    render_combat()
