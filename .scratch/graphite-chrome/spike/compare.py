"""Compare per-band energy of two WAV files to prove whether an EQ was applied.

Uses a plain Goertzel-style DFT at the tone frequencies, so no numpy needed.
"""
import cmath
import math
import struct
import sys
import wave

FREQS = [60.0, 1000.0, 8000.0]


def read_mono(path):
    """Minimal RIFF walker. Handles WAVE_FORMAT_EXTENSIBLE (0xFFFE), which
    mpv's ao_pcm emits and Python's `wave` module rejects."""
    blob = open(path, "rb").read()
    if blob[:4] != b"RIFF" or blob[8:12] != b"WAVE":
        raise SystemExit(f"{path}: not a RIFF/WAVE file")
    pos, fmt, data = 12, None, None
    while pos + 8 <= len(blob):
        cid = blob[pos:pos + 4]
        size = struct.unpack("<I", blob[pos + 4:pos + 8])[0]
        body = blob[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = body
        elif cid == b"data":
            data = body
        pos += 8 + size + (size & 1)
    if fmt is None or data is None:
        raise SystemExit(f"{path}: missing fmt or data chunk")

    tag, ch, rate, _, _, bits = struct.unpack("<HHIIHH", fmt[:16])
    if tag == 0xFFFE and len(fmt) >= 40:
        tag = struct.unpack("<H", fmt[24:26])[0]  # first 2 bytes of SubFormat GUID
    if tag != 1 or bits != 16:
        raise SystemExit(f"{path}: expected 16-bit PCM, got tag={tag} bits={bits}")

    total = len(data) // 2
    vals = struct.unpack("<%dh" % total, data[: total * 2])
    if ch > 1:
        vals = [sum(vals[i:i + ch]) / ch for i in range(0, len(vals) - ch + 1, ch)]
    return [v / 32768.0 for v in vals], rate


def amplitude_at(samples, rate, freq, window=32768):
    # Analyse a window from the middle, past filter settling transients.
    mid = len(samples) // 2
    seg = samples[mid: mid + window]
    if len(seg) < window:
        seg = samples[-window:]
    acc = 0j
    for i, s in enumerate(seg):
        acc += s * cmath.exp(-2j * math.pi * freq * i / rate)
    return 2 * abs(acc) / len(seg)


def db(x):
    return 20 * math.log10(x) if x > 1e-9 else -180.0


a_path, b_path = sys.argv[1], sys.argv[2]
a, rate_a = read_mono(a_path)
b, rate_b = read_mono(b_path)
print(f"{a_path}: {len(a)} frames @ {rate_a} Hz")
print(f"{b_path}: {len(b)} frames @ {rate_b} Hz")
print(f"\n{'freq':>8} {'input dB':>10} {'output dB':>11} {'delta dB':>10}")
deltas = []
for f in FREQS:
    ia = db(amplitude_at(a, rate_a, f))
    ob = db(amplitude_at(b, rate_b, f))
    d = ob - ia
    deltas.append(d)
    print(f"{f:>8.0f} {ia:>10.2f} {ob:>11.2f} {d:>+10.2f}")

spread = max(deltas) - min(deltas)
print(f"\nspread between bands: {spread:.2f} dB")
print("VERDICT:", "EQ APPLIED" if spread > 3.0 else "NO PER-BAND EFFECT DETECTED")
