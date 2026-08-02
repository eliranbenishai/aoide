"""Author the playlist 9-slice skin in-family from the main/EQ face grain.

There is no playlist mockup, so the chrome is *invented* from the same brushed
graphite the main and equalizer faces are made of: a real low-variance metal
patch is sampled from `main_face.png`, made seamless by mirror-tiling, and used
as the base texture for every region. Frame bevels (highlight/shadow) are drawn
on top from the skin palette so the panel reads as a raised bezel around a
recessed, grained well -- never a flat `#1D2128` rectangle.

Run from the worktree root:

    python .scratch/graphite-skin/build_playlist_slices.py

Outputs (2x logical; logical = px / 2), RGBA, into
`assets/skin/graphite/playlist/`:

    nw n ne   corners 24x24            (logical 12x12)
    w  .  e   side edges  w/e 24x64, n/s 64x24
    sw s se   (edges tile along their long axis)
    well      96x96 dark grained well tile (tiles both axes)

The Dart `PlaylistSlices.graphite` border is EdgeInsets.all(12), matching the
24px corners at 2x.
"""
import os

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "assets", "skin", "graphite", "main_face.png")
OUT_DIR = os.path.join(ROOT, "assets", "skin", "graphite", "playlist")

# Sizes in 2x px.
B = 24            # border / corner thickness
EDGE_TILE = 64    # long-axis length of the tiling edge strips
WELL = 96         # well tile size

# Skin palette (from TrampColors).
BEVEL_HI = np.array([0x55, 0x5B, 0x65], np.float32)
BEVEL_LO = np.array([0x0B, 0x0E, 0x12], np.float32)
WELL_DEEP = np.array([0x01, 0x03, 0x06], np.float32)
LCD_GLASS = np.array([0x03, 0x06, 0x0A], np.float32)

# Clean brushed-metal patch in main_face (probe_playlist_grain.py: mean~36,
# std~2 -- real graphite grain, no glyph, fully opaque).
SAMPLE_BOX = (884, 360, 1140, 440)  # l, t, r, b -> 256x80 of pure metal


def _load_grain():
    """Sample the real metal patch: its mean colour and its grain amplitude.

    The mockup grain is fine, structureless speckle (probe: luminance std ~2),
    so the playlist chrome is rebuilt as neutral high-frequency noise matched to
    that amplitude over the sampled base colour -- in-family with the main/EQ
    faces, and tileable with no seam because pure per-pixel noise has no
    low/mid-frequency structure to break at a tile boundary.
    """
    im = Image.open(SRC).convert("RGB")
    l, t, r, b = SAMPLE_BOX
    arr = np.asarray(im.crop((l, t, r, b)), np.float32)
    base = arr.mean(axis=(0, 1))
    lum = arr @ np.array([0.3, 0.6, 0.1], np.float32)
    grain_std = float(lum.std())
    return base, grain_std


BASE, GRAIN_STD = _load_grain()
_RNG = np.random.default_rng(0x7A3B)  # fixed seed -> reproducible art


def metal(h, w, base=None, grain_scale=1.0):
    """An h x w RGBA metal tile: sampled base colour + matched neutral grain.

    Grain is single-channel (equal r/g/b) so it reads as brushed metal, and is
    pure per-pixel noise so the tile repeats seamlessly along any axis.
    """
    if base is None:
        base = BASE
    noise = _RNG.normal(0.0, GRAIN_STD * grain_scale, (h, w, 1))
    rgb = np.clip(base + noise, 0, 255)
    out = np.empty((h, w, 4), np.float32)
    out[:, :, :3] = rgb
    out[:, :, 3] = 255
    return out


def _blend_line(img, ys, xs, colour, alpha):
    """Alpha-blend a solid colour into img[ys, xs]."""
    region = img[ys, xs, :3]
    img[ys, xs, :3] = region * (1 - alpha) + colour * alpha


def bevel(img, top=0.0, bottom=0.0, left=0.0, right=0.0, width=3):
    """Draw soft highlight (+) / shadow (-) bevel lines on the four sides.

    A positive value lightens (highlight, BEVEL_HI), negative darkens (shadow,
    BEVEL_LO). Alpha fades across `width` px so the bevel reads as rounded metal.
    """
    h, w, _ = img.shape
    for i in range(width):
        a = (width - i) / width  # strongest at the outer pixel
        if top:
            c = BEVEL_HI if top > 0 else BEVEL_LO
            _blend_line(img, i, slice(0, w), c, a * abs(top))
        if bottom:
            c = BEVEL_HI if bottom > 0 else BEVEL_LO
            _blend_line(img, h - 1 - i, slice(0, w), c, a * abs(bottom))
        if left:
            c = BEVEL_HI if left > 0 else BEVEL_LO
            _blend_line(img, slice(0, h), i, c, a * abs(left))
        if right:
            c = BEVEL_HI if right > 0 else BEVEL_LO
            _blend_line(img, slice(0, h), w - 1 - i, c, a * abs(right))


def save(arr, name):
    Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGBA").save(
        os.path.join(OUT_DIR, f"{name}.png"))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    HI, LO = 0.85, 0.9  # bevel strengths

    # --- Edges: metal strips that tile along their long axis ----------------
    n = metal(B, EDGE_TILE)
    bevel(n, top=HI, bottom=-LO)        # outer highlight, inner (well) shadow
    save(n, "n")

    s = metal(B, EDGE_TILE)
    bevel(s, top=-LO, bottom=HI * 0.6)  # inner shadow on top, soft outer base
    save(s, "s")

    w = metal(EDGE_TILE, B)
    bevel(w, left=HI, right=-LO)
    save(w, "w")

    e = metal(EDGE_TILE, B)
    bevel(e, left=-LO, right=HI * 0.6)
    save(e, "e")

    # --- Corners: fixed metal squares carrying both adjacent bevels ---------
    nw = metal(B, B)
    bevel(nw, top=HI, left=HI, bottom=-LO, right=-LO)
    save(nw, "nw")

    ne = metal(B, B)
    bevel(ne, top=HI, right=HI * 0.6, bottom=-LO, left=-LO)
    save(ne, "ne")

    sw = metal(B, B)
    bevel(sw, bottom=HI * 0.6, left=HI, top=-LO, right=-LO)
    save(sw, "sw")

    se = metal(B, B)
    bevel(se, bottom=HI * 0.6, right=HI * 0.6, top=-LO, left=-LO)
    save(se, "se")

    # --- Well: recessed dark grained glass, tiles both axes -----------------
    # Dark base between the deep well and the LCD glass, real grain kept (at
    # reduced amplitude) so it never reads as a flat fill.
    well_base = (WELL_DEEP + LCD_GLASS) / 2 + np.array([8, 11, 16], np.float32)
    well = metal(WELL, WELL, base=well_base, grain_scale=1.6)
    save(well, "well")

    print("wrote playlist slices to", OUT_DIR)


if __name__ == "__main__":
    main()
