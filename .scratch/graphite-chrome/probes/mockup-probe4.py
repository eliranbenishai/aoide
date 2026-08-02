"""Locate the thin green bar under the spectrum and measure its fill fraction."""
from PIL import Image
import sys

im = Image.open(sys.argv[1]).convert("RGB")
px = im.load()
W, H = im.size


def greenish(c):
    r, g, b = c
    return g > 90 and g > b + 40 and r < g + 40


print("--- rows in the LCD left region (x 90..560) with bright chartreuse pixels ---")
for y in range(300, 360):
    xs = [x for x in range(80, 580) if greenish(px[x, y])]
    if len(xs) > 20:
        print(f"y={y:>4} count={len(xs):>4} span x={min(xs)}..{max(xs)}")

print("\n--- LCD inset bounds probe: horizontal scan at y=330 ---")
prev = None
for x in range(0, 1200):
    c = px[x, 330]
    if prev is None or max(abs(a - b) for a, b in zip(c, prev)) > 14:
        print(f"  x={x:>4} #%02X%02X%02X" % c)
    prev = c
