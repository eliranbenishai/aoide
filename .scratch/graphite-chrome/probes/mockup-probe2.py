"""Sample named regions of the mockup and report dominant/brightest colors."""
import sys
from collections import Counter
from PIL import Image

im = Image.open(sys.argv[1]).convert("RGB")
W, H = im.size

# Region boxes in mockup pixel space (l, t, r, b), eyeballed from the 1663x946 render.
REGIONS = {
    "outer background (top-left corner)": (0, 0, 16, 16),
    "main panel face (right of transport)": (1120, 400, 1180, 440),
    "titlebar rail (left run)": (140, 55, 700, 62),
    "titlebar wordmark WINAMP": (740, 45, 900, 70),
    "LCD panel background": (620, 300, 700, 330),
    "LCD track title text": (600, 155, 1090, 180),
    "LCD big time 0:05": (580, 205, 680, 245),
    "LCD total time 0:22": (700, 215, 760, 240),
    "LCD kbps/khz text": (580, 265, 780, 285),
    "spectrum bars area": (100, 200, 540, 320),
    "play button triangle": (300, 405, 350, 440),
    "transport button face": (60, 390, 110, 410),
    "VU meter bar area": (1220, 205, 1560, 225),
    "shuffle/repeat button face": (1340, 125, 1400, 165),
    "logo bolt button": (30, 30, 100, 100),
    "EQ panel face": (250, 620, 320, 660),
    "EQ slider fill green": (388, 760, 400, 830),
    "EQ slider groove": (388, 660, 400, 700),
    "EQ thumb": (378, 725, 412, 745),
    "EQ value label -1.2": (370, 860, 420, 885),
    "EQ ON button": (95, 600, 160, 630),
    "PRESETS button face": (1380, 610, 1520, 645),
}


def hexc(c):
    return "#%02X%02X%02X" % c


for name, box in REGIONS.items():
    crop = im.crop(box)
    data = list(crop.getdata())
    top = Counter(data).most_common(3)
    brightest = max(data, key=lambda c: sum(c))
    print(f"\n{name}  {box}")
    print("  dominant:", "  ".join(f"{hexc(c)}({100*n/len(data):.0f}%)" for c, n in top))
    print("  brightest:", hexc(brightest))
