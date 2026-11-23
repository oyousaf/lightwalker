"""Reading and writing the WAVs FL Studio exports (16, 24 or 32-bit PCM), shared by the split scripts."""

import wave

import numpy as np


def read(path):
    """Returns (stereo float samples, sample rate)."""
    with wave.open(path, "rb") as file:
        channels, width, rate, frames = file.getnchannels(), file.getsampwidth(), file.getframerate(), file.getnframes()
        raw = file.readframes(frames)
    if width == 2:
        data = np.frombuffer(raw, "<i2").astype(np.float64) / 32768
    elif width == 3:
        bytes3 = np.frombuffer(raw, np.uint8).reshape(-1, 3)
        ints = (bytes3[:, 0].astype(np.int32) | (bytes3[:, 1].astype(np.int32) << 8) | (bytes3[:, 2].astype(np.int32) << 16))
        ints = np.where(ints >= 1 << 23, ints - (1 << 24), ints)
        data = ints.astype(np.float64) / (1 << 23)
    elif width == 4:
        data = np.frombuffer(raw, "<i4").astype(np.float64) / (1 << 31)
    else:
        raise SystemExit(f"Unsupported sample width in {path}: {width} bytes")
    data = data.reshape(-1, channels)
    if channels == 1:
        data = np.repeat(data, 2, axis=1)
    return data[:, :2], rate


def write(path, data, rate):
    """Writes 24-bit stereo PCM, so gain changes cost nothing."""
    ints = np.clip(np.round(data * (1 << 23)), -(1 << 23), (1 << 23) - 1).astype(np.int32)
    raw = np.stack([ints & 0xFF, (ints >> 8) & 0xFF, (ints >> 16) & 0xFF], axis=-1).astype(np.uint8).tobytes()
    with wave.open(path, "wb") as file:
        file.setnchannels(2)
        file.setsampwidth(3)
        file.setframerate(rate)
        file.writeframes(raw)


def db(value):
    return 20 * np.log10(max(value, 1e-12))
