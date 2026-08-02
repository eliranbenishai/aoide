"""Measure panel and element bounds in the mockup by detecting the near-black gutter."""
import sys
from PIL import Image

im = Image.open(sys.argv[1]).convert("RGB")
px = im.load()
W, H = im.size
print(f"image {W}x{H}")


def lum(c):
    return (c[0] * 299 + c[1] * 587 + c[2] * 114) / 1000


def row_lum(y, x0=0, x1=None):
    x1 = x1 or W
    return sum(lum(px[x, y]) for x in range(x0, x1, 4)) / len(range(x0, x1, 4))


def col_lum(x, y0, y1):
    return sum(lum(px[x, y]) for y in range(y0, y1, 4)) / len(range(y0, y1, 4))


print("\n=== panel bands (mean row luminance; <6 == outer black gutter) ===")
dark = [y for y in range(H) if row_lum(y) < 6]
bands, start = [], None
for y in range(H):
    d = y in set(dark)
    if not d and start is None:
        start = y
    elif d and start is not None:
        if y - start > 20:
            bands.append((start, y - 1))
        start = None
if start is not None:
    bands.append((start, H - 1))
for t, b in bands:
    print(f"  panel y={t}..{b}  height={b - t + 1}")

for idx, (t, b) in enumerate(bands):
    dc = [x for x in range(W) if col_lum(x, t, b) < 6]
    xs = [x for x in range(W) if x not in set(dc)]
    l, r = min(xs), max(xs)
    print(f"\n  panel #{idx}: x={l}..{r} width={r - l + 1} height={b - t + 1} "
          f"aspect={(r - l + 1) / (b - t + 1):.4f}")

print("\n=== chartreuse element map (bright yellow-green pixels) ===")


def acid(c):
    r, g, b = c
    return g > 120 and g > b + 55 and r < g + 30


rows = {}
for y in range(H):
    n = sum(1 for x in range(0, W, 2) if acid(px[x, y]))
    if n > 8:
        rows[y] = n
runs, prev, start = [], None, None
for y in sorted(rows):
    if prev is None or y - prev > 3:
        if start is not None:
            runs.append((start, prev))
        start = y
    prev = y
if start is not None:
    runs.append((start, prev))
for a, b in runs:
    xs = [x for x in range(W) if any(acid(px[x, y]) for y in range(a, b + 1))]
    print(f"  y={a}..{b} (h={b - a + 1})  x={min(xs)}..{max(xs)}")
