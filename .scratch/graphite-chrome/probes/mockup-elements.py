"""Measure per-element bounds inside each mockup panel.

Raised chrome buttons are found by their bright top bevel; inset wells by their
near-black interior. Coordinates are reported in mockup pixels and in the
logical canvas (mockup / 2).
"""
import sys
from PIL import Image

im = Image.open(sys.argv[1]).convert("RGB")
px = im.load()
W, H = im.size

MAIN = (19, 18, 1643, 501)
EQ = (19, 513, 1643, 923)


def lum(c):
    return (c[0] * 299 + c[1] * 587 + c[2] * 114) / 1000


def log(v):
    return round(v / 2, 1)


def runs(flags, min_len):
    out, start = [], None
    for i, f in enumerate(flags):
        if f and start is None:
            start = i
        elif not f and start is not None:
            if i - start >= min_len:
                out.append((start, i - 1))
            start = None
    if start is not None and len(flags) - start >= min_len:
        out.append((start, len(flags) - 1))
    return out


def wells(box, label):
    """Find inset wells: horizontal runs of very dark interior."""
    l, t, r, b = box
    print(f"\n=== {label}: dark (inset) column runs, sampled mid-height ===")
    ymid = (t + b) // 2
    for y in range(t + 8, b - 8, max(1, (b - t) // 12)):
        flags = [lum(px[x, y]) < 12 for x in range(l, r)]
        rr = [(a + l, z + l) for a, z in runs(flags, 30)]
        if rr:
            desc = "  ".join(f"x={a}..{z}(w={z-a+1}/log {log(z-a+1)})" for a, z in rr)
            print(f"  y={y:>4} (log {log(y - t)}): {desc}")


def bevels(box, label):
    """Find raised chrome buttons via bright top-bevel horizontal runs."""
    l, t, r, b = box
    print(f"\n=== {label}: bright bevel rows (candidate button tops) ===")
    for y in range(t, b):
        flags = [lum(px[x, y]) > 62 for x in range(l, r)]
        rr = [(a + l, z + l) for a, z in runs(flags, 22)]
        if rr:
            desc = "  ".join(f"x={a}..{z}(w={z-a+1}/log {log(z-a+1)})" for a, z in rr)
            print(f"  y={y:>4} (log {log(y - t)}): {desc}")


for box, label in ((MAIN, "MAIN"), (EQ, "EQ")):
    l, t, r, b = box
    print(f"\n#### {label} panel: x={l}..{r} y={t}..{b} "
          f"({r-l+1}x{b-t+1}) -> logical {log(r-l+1)}x{log(b-t+1)}")
    wells(box, label)
    bevels(box, label)
