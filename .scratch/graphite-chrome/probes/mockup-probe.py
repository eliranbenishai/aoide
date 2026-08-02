"""Sample colors and detect panel edges in the UI mockup."""
import sys
from collections import Counter
from PIL import Image

path = sys.argv[1]
im = Image.open(path).convert("RGB")
w, h = im.size
print(f"size={w}x{h} format={Image.open(path).format}")

px = im.load()


def hexc(c):
    return "#%02X%02X%02X" % c


print("\n--- top-20 colors ---")
for c, n in Counter(im.getdata()).most_common(20):
    print(f"{hexc(c):>8} {n:>7} ({100*n/(w*h):5.2f}%)")

print("\n--- vertical scan at x=512 (row transitions) ---")
prev = None
for y in range(h):
    c = px[512, y]
    if prev is None or max(abs(a - b) for a, b in zip(c, prev)) > 12:
        print(f"y={y:>4} {hexc(c)}")
    prev = c

print("\n--- horizontal scan at y=40 (title bar row) ---")
prev = None
for x in range(w):
    c = px[x, 40]
    if prev is None or max(abs(a - b) for a, b in zip(c, prev)) > 12:
        print(f"x={x:>4} {hexc(c)}")
    prev = c
