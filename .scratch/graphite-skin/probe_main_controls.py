"""Annotate the graphite-chrome mockup with a coordinate grid for Task 6.

Run from the worktree root:
    python .scratch/graphite-skin/probe_main_controls.py

Outputs magnified, grid-labelled crops of each control cluster so control
bounding boxes can be read off in MOCKUP pixel space (source 1663 x 946).
Mockup px -> main-face px: subtract (19, 18).  main-face px -> logical: / 2.
So logical = (mockup_px - 19) / 2 (x), (mockup_py - 18) / 2 (y).
"""
import os
from PIL import Image, ImageDraw

SRC = "docs/mockups/graphite-chrome.png"
OUT = ".scratch/graphite-skin/_probe"
os.makedirs(OUT, exist_ok=True)

im = Image.open(SRC).convert("RGB")
print("source size:", im.size)


def grid(name, region, scale, step=10):
    l, t, r, b = region
    crop = im.crop(region).resize(((r - l) * scale, (b - t) * scale), Image.NEAREST)
    d = ImageDraw.Draw(crop)
    for xm in range(l - (l % step), r + 1, step):
        xc = (xm - l) * scale
        if xc < 0:
            continue
        d.line([(xc, 0), (xc, crop.height)], fill=(255, 60, 60), width=1)
        d.text((xc + 1, 1), str(xm), fill=(255, 150, 150))
    for ym in range(t - (t % step), b + 1, step):
        yc = (ym - t) * scale
        if yc < 0:
            continue
        d.line([(0, yc), (crop.width, yc)], fill=(60, 160, 255), width=1)
        d.text((1, yc + 1), str(ym), fill=(150, 190, 255))
    crop.save(os.path.join(OUT, name))
    print("wrote", name, region)


# Title-bar window buttons (min / max / close), mockup top-right.
grid("titlebar_right.png", (1350, 10, 1650, 80), 4)
# Shuffle / repeat icon buttons, top-right of display well.
grid("shuffle_repeat.png", (1280, 100, 1580, 185), 3)
# EQ / PL buttons on the right, just below the meters.
grid("eq_pl.png", (1150, 285, 1470, 360), 3)
# Transport row (prev/play/pause/stop/next) + right SHUFFLE / bolt buttons.
grid("transport_band.png", (60, 370, 1650, 470), 1, step=20)
# Right cluster of the transport band (SHUFFLE + bolt) magnified.
grid("transport_right.png", (1180, 370, 1560, 470), 2)
