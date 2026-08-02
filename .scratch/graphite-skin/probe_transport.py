"""Pinpoint the EQ slider thumb with a fine grid.

Run from the worktree root:
    python .scratch/graphite-skin/probe_transport.py
"""
import os
from PIL import Image, ImageDraw

SRC = "docs/mockups/graphite-chrome.png"
OUT = ".scratch/graphite-skin/_probe"
os.makedirs(OUT, exist_ok=True)

im = Image.open(SRC).convert("RGB")

REG = (340, 695, 440, 785)
SCALE = 6
crop = im.crop(REG).resize(
    ((REG[2] - REG[0]) * SCALE, (REG[3] - REG[1]) * SCALE), Image.NEAREST
)
draw = ImageDraw.Draw(crop)
for xm in range(REG[0], REG[2] + 1, 10):
    xc = (xm - REG[0]) * SCALE
    draw.line([(xc, 0), (xc, crop.height)], fill=(255, 60, 60), width=1)
    draw.text((xc + 1, 1), str(xm), fill=(255, 120, 120))
for ym in range(REG[1], REG[3] + 1, 10):
    yc = (ym - REG[1]) * SCALE
    draw.line([(0, yc), (crop.width, yc)], fill=(60, 160, 255), width=1)
    draw.text((1, yc + 1), str(ym), fill=(120, 180, 255))
crop.save(os.path.join(OUT, "thumb_fine.png"))
print("region", REG)
