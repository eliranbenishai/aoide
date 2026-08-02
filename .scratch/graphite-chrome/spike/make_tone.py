"""Generate a test tone with equal energy in three bands the EQ should hit."""
import math
import struct
import sys
import wave

OUT = sys.argv[1] if len(sys.argv) > 1 else "tone.wav"
RATE = 44100
SECONDS = 3.0
TONES = [60.0, 1000.0, 8000.0]
AMP = 0.28  # per tone, so the sum stays clear of clipping

frames = bytearray()
for n in range(int(RATE * SECONDS)):
    t = n / RATE
    s = sum(AMP * math.sin(2 * math.pi * f * t) for f in TONES)
    v = max(-1.0, min(1.0, s))
    frames += struct.pack("<hh", int(v * 32767), int(v * 32767))

with wave.open(OUT, "wb") as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(RATE)
    w.writeframes(bytes(frames))

print(f"wrote {OUT}: {SECONDS}s stereo {RATE}Hz, tones {TONES}")
