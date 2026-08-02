"""Slice the graphite-chrome mockup into the TRAMP skin's panel-face PNGs.

Run from the repo root (the worktree root):

    python .scratch/graphite-skin/slice_mockup.py

Outputs (2x logical sizes, RGBA):
    assets/skin/graphite/main_face.png       1624 x 484   (logical 812 x 242)
    assets/skin/graphite/equalizer_face.png  1624 x 412   (logical 812 x 206)

What it does (PNG-first: the brushed metal / grain / bevels / emboss all come
straight from the mockup pixels; nothing is redrawn as a flat gradient):

  1. Rebrands the title bars WINAMP -> TRAMP and WINAMP EQUALIZER ->
     TRAMP EQUALIZER. The old word is erased by cloning a clean strip of the
     same title-bar metal (same rows, so the vertical bevel is preserved), then
     the new word is stamped in a light-grey embossed Barlow Bold that matches
     the mockup's face colour.
  2. Removes both gold lightning bolts (top-left corner button + bottom-right
     button on the main panel). Each bolt is covered by tiling a clean patch of
     that button's own dark face texture -- grainy metal, never a flat hole.
     The equalizer panel carries no bolt.
  3. Punches full alpha (transparent) in the main display-well interior that
     code will own -- the spectrum analyser + the LCD text column (track title,
     time, bitrate, EQ/PL toggles). The well's bevelled frame stays opaque.
     The equalizer face is NOT punched: its slider grooves are face art that
     code-drawn thumbs overlay.

All coordinates below are in MOCKUP pixel space (source is 1663 x 946). Edits
are applied to the full mockup first, then the panels are cropped, so probe
coordinates map 1:1. See .scratch/graphite-skin/probe_*.py for how they were
measured.

------------------------------------------------------------------------------
PUNCHED LOGICAL RECT (for Task 2 / graphite_skin.dart)
------------------------------------------------------------------------------
Asset pixels are authored at 2x, so logical = asset_pixel / 2.

  main display well (spectrum + LCD text column), in the 812 x 242 main canvas:
      logical  Rect.fromLTRB(35.5, 37.0, 559.0, 167.0)
      (main_face pixels: left=71 top=74 right=1118 bottom=334)

  Sub-regions inside that well, for reference when Task 2 lays out painters
  (also logical, 812 x 242 canvas):
      spectrum analyser : Rect.fromLTRB(35.5, 37.0, 250.0, 167.0)
      LCD text column   : Rect.fromLTRB(255.0, 37.0, 559.0, 167.0)
  These sub-splits are advisory; only the single well rect above is punched.

  equalizer_face: NO alpha punch (slider grooves are face art).
------------------------------------------------------------------------------
"""
import os

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "docs", "mockups", "graphite-chrome.png")
OUT_DIR = os.path.join(ROOT, "assets", "skin", "graphite")
FONT_PATH = os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-Bold.ttf")

# Panel crop boxes (left, top, right, bottom); right/bottom exclusive.
#   main: 1643-19 = 1624 wide, 502-18 = 484 tall
#   eq  : 1643-19 = 1624 wide, 925-513 = 412 tall
MAIN_CROP = (19, 18, 1643, 502)
EQ_CROP = (19, 513, 1643, 925)

# Title face colour sampled from the mockup (light grey, faintly cool).
TEXT_FACE = (203, 208, 213, 255)
TEXT_SHADOW = (14, 16, 20, 200)


def tile_fill(img, box, patch_box):
    """Cover `box` by tiling the texture inside `patch_box` (grainy metal)."""
    l, t, r, b = box
    patch = img.crop(patch_box)
    pw, ph = patch.size
    for y in range(t, b, ph):
        for x in range(l, r, pw):
            img.paste(patch, (x, y))


def clone_strip(img, dst_box, src_left, src_top):
    """Paste a region the size of `dst_box`, sourced from (src_left, src_top)."""
    l, t, r, b = dst_box
    w, h = r - l, b - t
    src = img.crop((src_left, src_top, src_left + w, src_top + h))
    img.paste(src, (l, t))


def stamp_text(draw, center_x, baseline_y, text, font, tracking):
    """Draw `text` centred on center_x at `baseline_y`, embossed, with tracking."""
    advances = [draw.textlength(ch, font=font) for ch in text]
    total = sum(advances) + tracking * (len(text) - 1)
    x = center_x - total / 2
    for ch, adv in zip(text, advances):
        draw.text((x + 1, baseline_y + 1), ch, font=font, fill=TEXT_SHADOW,
                  anchor="ls")
        draw.text((x, baseline_y), ch, font=font, fill=TEXT_FACE, anchor="ls")
        x += adv + tracking


def main():
    im = Image.open(SRC).convert("RGBA")
    draw = ImageDraw.Draw(im)

    # --- 1. Remove gold lightning bolts (main panel only) -------------------
    # Cover each bolt by tiling a flat, gradient-free patch of that button's own
    # dark face (measured lum ~34-44, no bevel/speck) so no seam or stripe shows.
    # top-left corner button: bolt gold bbox x=77..99 y=42..73
    tile_fill(im, (70, 38, 106, 78), (62, 46, 74, 66))
    # bottom-right button: bolt gold bbox x=1495..1518 y=417..447
    tile_fill(im, (1488, 412, 1526, 452), (1455, 418, 1475, 446))

    # --- 2. Rebrand title bars ---------------------------------------------
    # MAIN: WINAMP (x=750..913) sits in the pinstripe gap x=720..943.
    # Erase by cloning clean title metal from x>=1075 (right of right pinstripe),
    # same rows so the bar's vertical bevel is preserved.
    clone_strip(im, (725, 40, 940, 72), 1075, 40)
    main_font = ImageFont.truetype(FONT_PATH, 30)
    stamp_text(draw, 831, 65, "TRAMP", main_font, tracking=4)

    # EQ: WINAMP EQUALIZER (x=668..986) in the pinstripe gap x=646..1010.
    clone_strip(im, (650, 524, 1006, 561), 1170, 524)
    eq_font = ImageFont.truetype(FONT_PATH, 26)
    stamp_text(draw, 827, 553, "TRAMP EQUALIZER", eq_font, tracking=3)

    # --- 3. Crop panel faces ------------------------------------------------
    main_face = im.crop(MAIN_CROP)
    eq_face = im.crop(EQ_CROP)

    # --- 4. Punch the main display well to full alpha -----------------------
    # Mockup well interior x=90..1136 y=92..352 -> main-crop local (offset 19,18)
    punch = main_face.load()
    px_l, px_t, px_r, px_b = 71, 74, 1118, 334
    for y in range(px_t, px_b):
        for x in range(px_l, px_r):
            r, g, b, _ = punch[x, y]
            punch[x, y] = (r, g, b, 0)

    # --- 5. Save ------------------------------------------------------------
    os.makedirs(OUT_DIR, exist_ok=True)
    main_face.save(os.path.join(OUT_DIR, "main_face.png"))
    eq_face.save(os.path.join(OUT_DIR, "equalizer_face.png"))
    print("main_face:", main_face.size, "eq_face:", eq_face.size)


if __name__ == "__main__":
    main()
