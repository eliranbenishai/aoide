"""Compare the left-edge vertical gradient of the main player vs the EQ panel."""
import sys
from PIL import Image

im = Image.open(sys.argv[1]).convert("RGB")
px = im.load()


def hexc(c):
    return "#%02X%02X%02X" % c


def column(label, x, y0, y1, step):
    print(f"\n--- {label}: x={x}, y={y0}..{y1} ---")
    for y in range(y0, y1, step):
        print(f"  y={y:>4} {hexc(px[x, y])}")


# Main player panel: outer frame ~x=13..1648, y=13..500
column("MAIN left edge inner face", 30, 20, 500, 24)
column("MAIN top-left corner detail", 40, 14, 60, 2)

# EQ panel: y=512..926
column("EQ left edge inner face", 30, 516, 924, 24)
column("EQ top-left corner detail", 40, 512, 558, 2)

print("\n--- horizontal gradient across MAIN panel face at y=460 ---")
for x in range(16, 1650, 80):
    print(f"  x={x:>4} {hexc(px[x, 460])}")

print("\n--- horizontal gradient across EQ panel face at y=900 ---")
for x in range(16, 1650, 80):
    print(f"  x={x:>4} {hexc(px[x, 900])}")
