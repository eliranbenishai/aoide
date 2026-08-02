"""Fine probe of the title-bar window buttons (min / max / close).

Run from the worktree root:
    python .scratch/graphite-skin/probe_window_buttons.py

Emits a magnified 5x grid (5px step) of the full title-bar right cluster so the
three raised button bezels can be measured in MOCKUP pixel space (1663 x 946).
"""
import os
from PIL import Image, ImageDraw

SRC = "docs/mockups/graphite-chrome.png"
OUT = ".scratch/graphite-skin/_probe"
os.makedirs(OUT, exist_ok=True)

im = Image.open(SRC).convert("RGB")
region = (1350, 18, 1655, 82)
scale, step = 5, 5
l, t, r, b = region
crop = im.crop(region).resize(((r - l) * scale, (b - t) * scale), Image.NEAREST)
d = ImageDraw.Draw(crop)
for xm in range(l - (l % step), r + 1, step):
    xc = (xm - l) * scale
    if xc < 0:
        continue
    w = 2 if xm % 20 == 0 else 1
    d.line([(xc, 0), (xc, crop.height)], fill=(255, 60, 60), width=w)
    if xm % 20 == 0:
        d.text((xc + 1, 1), str(xm), fill=(255, 170, 170))
for ym in range(t - (t % step), b + 1, step):
    yc = (ym - t) * scale
    if yc < 0:
        continue
    w = 2 if ym % 20 == 0 else 1
    d.line([(0, yc), (crop.width, yc)], fill=(60, 160, 255), width=w)
    d.text((1, yc + 1), str(ym), fill=(150, 190, 255))
crop.save(os.path.join(OUT, "window_buttons_fine.png"))
print("wrote window_buttons_fine.png", region)
