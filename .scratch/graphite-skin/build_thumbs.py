"""Author the horizontal volume + seek slider grips for the graphite skin.

Run from the worktree root:

    python .scratch/graphite-skin/build_thumbs.py

Outputs (RGBA, 2x like the rest of the skin) under
`assets/skin/graphite/controls/`:

    volume_thumb.png   20 x 24   (logical 10 x 12) — rides the L/R meter gap
    seek_thumb.png     18 x 12   (logical  9 x  6) — rides the transport seek bar

PNG-first: the brushed-metal grain comes straight from the mockup's fader-grip
pixels (the same light metal as `slider_thumb` / `eq_thumb`); we only mask it to
a rounded grip and stamp a 1px bevel (light top edge, dark bottom edge). Nothing
is redrawn as a flat gradient, and the EQ vertical grip is never squashed — this
is fresh metal cropped for a horizontal handle.
"""
import os

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "docs", "mockups", "graphite-chrome.png")
OUT_DIR = os.path.join(ROOT, "assets", "skin", "graphite", "controls")

# A flat patch of the EQ fader grip's brushed metal (above its highlight bar).
METAL_PATCH = (379, 731, 407, 745)  # 28 x 14 of clean light grip metal

HI = (95, 102, 112, 255)   # bevel highlight (top/left)
LO = (10, 12, 16, 255)      # bevel shadow (bottom/right)


def tiled_metal(w, h, patch):
    """Fill a w x h image by tiling the brushed-metal `patch` (grain kept)."""
    out = Image.new("RGBA", (w, h))
    pw, ph = patch.size
    for y in range(0, h, ph):
        for x in range(0, w, pw):
            out.paste(patch, (x, y))
    return out


def rounded_grip(w, h, radius):
    """A rounded-rect grip of real metal grain with a stamped bevel edge."""
    metal = Image.open(SRC).convert("RGBA").crop(METAL_PATCH)
    body = tiled_metal(w, h, metal)

    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, w - 1, h - 1], radius=radius,
                                           fill=255)
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    out.paste(body, (0, 0), mask)

    # Bevel: light along the top edge, dark along the bottom edge.
    px = out.load()
    for x in range(w):
        for y in range(h):
            if mask.getpixel((x, y)) == 0:
                continue
            if y <= 1:
                px[x, y] = HI
            elif y >= h - 2:
                px[x, y] = LO
            break
    return out


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    volume = rounded_grip(20, 24, radius=6)
    volume.save(os.path.join(OUT_DIR, "volume_thumb.png"))
    seek = rounded_grip(18, 12, radius=4)
    seek.save(os.path.join(OUT_DIR, "seek_thumb.png"))
    print("volume_thumb:", volume.size, "seek_thumb:", seek.size)


if __name__ == "__main__":
    main()
