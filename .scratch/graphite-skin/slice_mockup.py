"""Slice the graphite-chrome mockup into the TRAMP skin's panel-face PNGs.

Run from the repo root (the worktree root):

    python .scratch/graphite-skin/slice_mockup.py

Outputs (2x logical sizes, RGBA):
    assets/skin/graphite/main_face.png              1624 x 484   (logical 812 x 242)
    assets/skin/graphite/equalizer_face.png         1624 x 412   (logical 812 x 206)
    assets/skin/graphite/equalizer_shade_face.png   1624 x  70   (logical 812 x  35)

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

  4. Cleans the equalizer face for Task 7: the baked mockup fader thumbs, green
     LED fills and gain-value numbers are removed so live code thumbs / fills /
     labels do not double-draw over static art. Each groove is emptied by a
     vertical *period-aligned* clone (copy every row from `pitch` rows above,
     top-down) seeded from the clean channel above the highest thumb -- this
     removes thumb + fill while faithfully reproducing the groove's own dark
     slot and its side tick marks (grain preserved, nothing redrawn as a flat
     fill). The gain numbers are covered by cloning clean panel metal from the
     inter-groove gap. The baked ON button (lit green in the mockup) is
     recoloured to neutral grey so the code toggle owns the lit state.
     A title-bar strip is also emitted as `equalizer_shade_face.png` for the
     collapsed windowshade.

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

# --- Equalizer face cleaning (all coords are eq_face-local px, 1624 x 412) ---
# Fader groove centres, measured from the baked green fills.
EQ_PREAMP_CX = 127
EQ_BAND_CX = [373, 471, 569, 667, 765, 862, 960, 1058, 1156, 1254]
# Groove side tick pitch (~15.3 px bands, ~14 px preamp); integer keeps the
# period-aligned clone reproducing the dashes.
EQ_BAND_PITCH = 15
EQ_PREAMP_PITCH = 14
# TrampColors.label grey (0xC9CED3), for neutralising the baked green ON glyph.
LABEL_GREY = (201, 206, 213)


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


def clean_groove(img, cx, start_y, bottom, pitch, half=34):
    """Empty a fader groove by a period-aligned vertical clone.

    Copies every row from `pitch` rows above, top-down, over the thumb + fill
    region. `start_y` sits just below the groove's clean channel top (above the
    highest thumb) so the seed rows carry the real dark slot and side ticks; the
    copy then propagates that pattern down, removing the baked thumb and green
    fill while keeping the groove's own grain and dashes.
    """
    px = img.load()
    for y in range(start_y, bottom):
        for x in range(cx - half, cx + half):
            px[x, y] = px[x, y - pitch]


def blank_gain_numbers(img, centres, top, bottom, half=32, shift=50):
    """Cover each baked gain-value number with clean inter-groove panel metal."""
    px = img.load()
    for cx in centres:
        for y in range(top, bottom):
            for x in range(cx - half, cx + half):
                px[x, y] = px[x + shift, y]


def recolour_region(img, box, target, lum_min=90, lum_ref=205):
    """Recolour bright pixels in `box` to `target`, keeping their luminance."""
    px = img.load()
    tr, tg, tb = target
    l, t, r, b = box
    for y in range(t, b):
        for x in range(l, r):
            cr, cg, cb, ca = px[x, y]
            if ca == 0:
                continue
            lum = 0.3 * cr + 0.6 * cg + 0.1 * cb
            if lum <= lum_min:
                continue
            f = min(1.0, lum / lum_ref)
            px[x, y] = (int(tr * f), int(tg * f), int(tb * f), ca)


def clean_eq_face(eq_face):
    """Remove baked thumbs / fills / gain numbers; neutralise the ON glyph."""
    for cx in EQ_BAND_CX:
        clean_groove(eq_face, cx, 160, 367, EQ_BAND_PITCH)
    clean_groove(eq_face, EQ_PREAMP_CX, 190, 323, EQ_PREAMP_PITCH)
    blank_gain_numbers(eq_face, [EQ_PREAMP_CX] + EQ_BAND_CX, 338, 382)
    recolour_region(eq_face, (72, 90, 140, 114), LABEL_GREY)


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

    # --- 2b. Task 6 main-panel retouch -------------------------------------
    # The title bar changes to minimize / zoom- / zoom+ / close (four controls,
    # code-drawn glyphs). Erase the mockup's three painted window buttons
    # (minimize / maximize / close, x=1356..1644) by cloning clean title metal
    # from x=1075 at the SAME rows, so the bar's vertical bevel is preserved.
    clone_strip(im, (1356, 24, 1644, 78), 1075, 24)

    # The bottom-right "SHUFFLE" button is redundant with the shuffle icon in
    # the top-right toggle pair, so it becomes OPEN. Clone the button's own
    # clean interior column across the glyph+label, then stamp "OPEN" in the
    # same embossed face as the wordmark. The bezel, grain and bevel are kept.
    tile_fill(im, (1214, 405, 1472, 451), (1200, 405, 1212, 451))
    open_font = ImageFont.truetype(FONT_PATH, 30)
    stamp_text(draw, 1341, 440, "OPEN", open_font, tracking=5)

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

    # --- 4b. Clean the equalizer face + emit the windowshade title strip ----
    clean_eq_face(eq_face)
    eq_shade = eq_face.crop((0, 0, 1624, 70))

    # --- 5. Save ------------------------------------------------------------
    os.makedirs(OUT_DIR, exist_ok=True)
    main_face.save(os.path.join(OUT_DIR, "main_face.png"))
    eq_face.save(os.path.join(OUT_DIR, "equalizer_face.png"))
    eq_shade.save(os.path.join(OUT_DIR, "equalizer_shade_face.png"))
    print("main_face:", main_face.size, "eq_face:", eq_face.size,
          "eq_shade:", eq_shade.size)


if __name__ == "__main__":
    main()
