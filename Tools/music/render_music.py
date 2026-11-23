"""
Renders the combat theme's two stings to the WAV files the game plays, from the compose script's MIDI, using
FluidSynth with the GeneralUser GS SoundFont (free for commercial music) and a built-in sliding 808.

  SourceArt/Music/CombatTheme/Export/Sting_WaveCleared.wav, Sting_GameOver.wav

The themes themselves are Omar's FL Studio mixes (see split_menu_theme.py and split_combat_stems.py). An FL export of
a sting under the same name replaces it with no code changes.

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
from midi_writer import BAR, PPQ, track_chunk, write  # noqa: E402

FLUIDSYNTH = os.environ.get("LW_FLUIDSYNTH", r"D:\AI\Tools\audio\fluidsynth\fluidsynth-v2.6.2-win10-x64-cpp11\bin\fluidsynth.exe")
SOUNDFONT = os.environ.get("LW_SOUNDFONT", r"D:\AI\Tools\audio\soundfonts\GeneralUser-GS.sf2")
RATE = 48000
PEAK = 0.89  # about -1 dB

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

def render_stings():
    bpm = combat.BPM
    folder = os.path.join(combat.OUT_DIR, "Export")
    sting_length = ticks_to_samples(3 * BAR, bpm)
    cleared = fluid_render([("WaveCleared", combat.sting_wave_cleared(), 1, 10)], bpm, sting_length)
    game_over_events = combat.sting_game_over()
    low = [e for e in game_over_events if e[2] < 40]
    game_over = fluid_render([("GameOver", [e for e in game_over_events if e[2] >= 40], 1, 0)], bpm, sting_length)
    bass = render_808(low, bpm, sting_length)
    game_over = [a + b * 0.6 for a, b in zip(game_over, bass)]
    for filename, audio in (("Sting_WaveCleared.wav", cleared), ("Sting_GameOver.wav", game_over)):
        write_wav(os.path.join(folder, filename), audio, PEAK / peak(audio) * 0.8)
    print("Rendered the two stings")


if __name__ == "__main__":
    for tool in (FLUIDSYNTH, SOUNDFONT):
        if not os.path.isfile(tool):
            sys.exit(f"Missing {tool} (set LW_FLUIDSYNTH / LW_SOUNDFONT)")
    render_stings()
