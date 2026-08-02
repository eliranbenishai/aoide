"""Crop the graphite skin's control art out of the chrome mockup.

Run from the worktree root (the repo root of the worktree):

    python .scratch/graphite-skin/crop_controls.py

Outputs (RGBA, authored at 2x like the panel faces) under
`assets/skin/graphite/controls/`:

    Task 3
      transport_play_idle.png       138 x 80    slider_thumb.png   32 x 45
      transport_play_pressed.png    138 x 80

    Task 6 (this pass)
      transport_prev/pause/stop/next  _idle / _pressed   138 x 80
      shuffle_idle / shuffle_active                       152 x 58
      repeat_idle  / repeat_active                        152 x 58
      eq_idle      / eq_active                            114 x 40
      pl_idle      / pl_active                            114 x 40

PNG-first, per the skin's hard rule: the brushed-metal grain, bevels and glyphs
all come straight from the mockup pixels. Missing states are tonal transforms of
real pixels, never flat redraws:

  * transport "pressed" — the idle crop darkened, nudged 1px down-right, with a
    soft top inner shadow (recessed metal). (Task 3's derivation, reused.)
  * toggle "active"/"idle" — the button's own glyph pixels are recoloured to the
    lit phosphor (active) or the neutral label grey (idle). The mockup bakes
    shuffle grey and repeat green; recolouring both ways gives a consistent
    off=grey / on=phosphor pair while keeping the real bezel, grain and bevel.

Coordinates are MOCKUP pixel space (source 1663 x 946); measured with
.scratch/graphite-skin/probe_main_controls.py.  Mockup px -> logical: (px-19)/2
in x, (py-18)/2 in y.
"""
import os

from PIL import Image, ImageEnhance

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "docs", "mockups", "graphite-chrome.png")
OUT_DIR = os.path.join(ROOT, "assets", "skin", "graphite", "controls")

# Transport button crop boxes (left, top, right, bottom); each 138 x 80.
# Bezels start at x = 98/238/378/518/658 with tops at y = 380 (pitch ~140).
TRANSPORT = {
    "prev": (98, 380, 236, 460),
    "play": (238, 380, 376, 460),
    "pause": (378, 380, 516, 460),
    "stop": (518, 380, 656, 460),
    "next": (658, 380, 796, 460),
}

# Play glyph inner box (relative to the play crop) for the idle grey recolour.
# The mockup bakes the play triangle green; that is the `active` sprite, and
# `idle` recolours the triangle to neutral label grey like the other transport
# icons (Task fidelity pass, decision 4).
PLAY_GLYPH = (30, 8, 112, 72)

# Toggle buttons: (crop box, glyph inner box relative to the crop).
SHUFFLE_BOX = (1316, 116, 1468, 174)   # 152 x 58
REPEAT_BOX = (1476, 116, 1628, 174)    # 152 x 58
EQ_BOX = (1206, 313, 1320, 353)        # 114 x 40
PL_BOX = (1322, 313, 1436, 353)        # 114 x 40

SHUFFLE_GLYPH = (20, 14, 118, 46)
REPEAT_GLYPH = (14, 14, 118, 48)
EQ_GLYPH = (14, 6, 100, 34)
PL_GLYPH = (18, 6, 104, 34)

# Title-bar window button bezels (mockup min / max / close). slice_mockup.py
# blanks these three painted buttons on the FACE so code controls sit on clean
# metal; here we crop the *pristine* mockup so the button's metal bezel itself
# is real PNG art (not a code-painted bevel). Tramp's title order is
# min / zoom- / zoom+ / close: minimize and close keep their mockup glyphs, and
# the two zoom buttons reuse the minimize bezel with its bar cloned out (a clean
# blank bezel) onto which the widget stamps a code +/- (the mockup has no zoom
# art, and the maximize square is not a Tramp control). 90 x 50 each.
WIN_MIN_BOX = (1360, 27, 1450, 77)
WIN_CLOSE_BOX = (1545, 27, 1635, 77)
# Minimize bar to clone out for the blank (zoom) bezel, and a clean metal patch
# from immediately below it — both relative to WIN_MIN_BOX. A single-paste copy
# from adjacent rows keeps the grain and bevel without tiling streaks.
WIN_MIN_BAR = (24, 24, 66, 34)
WIN_MIN_PATCH = (24, 35, 66, 45)

# Equalizer band metal fader grip (Task 3).
THUMB_BOX = (377, 728, 409, 773)  # 32 x 45

# --- Task 7: equalizer control crops --------------------------------------
# Mockup px = eq_face px + (19, 513). The rebrand only rewrites the title text,
# so ON / AUTO / PRESETS / collapse / close read the same in the pristine
# mockup as on the face. Boxes are (left, top, right, bottom).
EQ_ON_BOX = (81, 597, 167, 633)      # 86 x 36  -> logical 43 x 18
EQ_AUTO_BOX = (176, 597, 266, 633)   # 90 x 36  -> logical 45 x 18
EQ_PRESETS_BOX = (1352, 609, 1578, 643)  # 226 x 34 -> logical 113 x 17
EQ_COLLAPSE_BOX = (33, 523, 111, 567)    # 78 x 44  -> logical 39 x 22
EQ_CLOSE_BOX = (1559, 523, 1635, 567)    # 76 x 44  -> logical 38 x 22
# Full metal fader grip (rounded bezel + highlight bar), above the green fill.
EQ_THUMB_BOX = (553, 679, 621, 725)  # 68 x 46  -> logical 34 x 23

# Glyph inner boxes (relative to the crop) for the ON / AUTO toggle recolours.
EQ_ON_GLYPH = (10, 6, 78, 30)
EQ_AUTO_GLYPH = (15, 6, 81, 30)

PHOSPHOR = (207, 234, 69)   # TrampColors.phosphor 0xCFEA45
LABEL = (201, 206, 213)     # TrampColors.label   0xC9CED3


def press(idle):
    """Derive a pressed sprite from the idle crop: recessed, real grain kept."""
    w, h = idle.size
    pressed = ImageEnhance.Brightness(idle).enhance(0.80)
    shifted = Image.new("RGBA", (w, h))
    shifted.paste(pressed, (1, 1))
    shifted.paste(pressed.crop((0, 0, w, 1)), (0, 0))
    shifted.paste(pressed.crop((0, 0, 1, h)), (0, 0))
    pressed = shifted
    shadow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sp = shadow.load()
    depth = 14
    for y in range(depth):
        a = int(150 * (1 - y / depth))
        for x in range(w):
            sp[x, y] = (0, 0, 0, a)
    return Image.alpha_composite(pressed, shadow)


def blank_glyph(crop, glyph_box, patch_box):
    """Clone a clean metal `patch_box` over `glyph_box` (removes the glyph).

    Keeps the button's own grain and bevel — only the painted glyph is covered,
    exactly as slice_mockup.py blanks the face buttons.
    """
    out = crop.copy()
    patch = out.crop(patch_box)
    pw, ph = patch.size
    l, t, r, b = glyph_box
    for y in range(t, b, ph):
        for x in range(l, r, pw):
            out.paste(patch, (x, y))
    return out


def recolour(crop, glyph_box, target):
    """Recolour the glyph pixels inside `glyph_box` to `target`.

    Only bright pixels (the glyph, not the dark metal or bezel) are touched, and
    each keeps its own luminance so anti-aliased edges and the emboss survive.
    """
    out = crop.copy()
    px = out.load()
    tr, tg, tb = target
    l, t, r, b = glyph_box
    for y in range(t, b):
        for x in range(l, r):
            cr, cg, cb, ca = px[x, y]
            if ca == 0:
                continue
            lum = 0.3 * cr + 0.6 * cg + 0.1 * cb
            if lum <= 90:
                continue
            f = min(1.0, lum / 205.0)
            px[x, y] = (int(tr * f), int(tg * f), int(tb * f), ca)
    return out


def neutralise_green(crop, glyph_box, target, warm=6):
    """Desaturate the *warm* (green/yellow) pixels in `glyph_box` to `target`.

    The mockup's lit glyphs (play triangle, repeat arrows) sit in a phosphor
    bloom whose rim runs green->yellow. The graphite metal is cool (blue-
    leaning), so any pixel whose red/green rises above blue (`max(r,g) - b >
    warm`) is phosphor, not metal: it is remapped to the neutral grey `target`
    at its own luminance. Cool metal and neutral bevels pass through untouched,
    and luminance preservation keeps the anti-aliased edges and emboss.
    """
    out = crop.copy()
    px = out.load()
    tr, tg, tb = target
    l, t, r, b = glyph_box
    for y in range(t, b):
        for x in range(l, r):
            cr, cg, cb, ca = px[x, y]
            if ca == 0:
                continue
            if max(cr, cg) - cb <= warm:
                continue
            lum = 0.3 * cr + 0.6 * cg + 0.1 * cb
            f = min(1.0, lum / 205.0)
            px[x, y] = (int(tr * f), int(tg * f), int(tb * f), ca)
    return out


def main():
    im = Image.open(SRC).convert("RGBA")
    os.makedirs(OUT_DIR, exist_ok=True)

    def save(name, img):
        img.save(os.path.join(OUT_DIR, name + ".png"))

    for name, box in TRANSPORT.items():
        crop = im.crop(box)
        if name == "play":
            # Mockup play is lit green -> active; idle neutralises the triangle
            # and its phosphor glow to label grey (decision 4). The bloom spills
            # across the whole bezel, so scan the full crop (cool metal is safe).
            save("transport_play_active", crop)
            idle = neutralise_green(
                crop, (0, 0, crop.width, crop.height), LABEL, warm=1)
            save("transport_play_idle", idle)
            save("transport_play_pressed", press(idle))
            continue
        save("transport_%s_idle" % name, crop)
        save("transport_%s_pressed" % name, press(crop))

    shuffle = im.crop(SHUFFLE_BOX)
    save("shuffle_idle", shuffle)
    save("shuffle_active", recolour(shuffle, SHUFFLE_GLYPH, PHOSPHOR))

    repeat = im.crop(REPEAT_BOX)
    save("repeat_idle",
         neutralise_green(repeat, (0, 0, repeat.width, repeat.height), LABEL))
    save("repeat_active", repeat)

    eq = im.crop(EQ_BOX)
    save("eq_idle", eq)
    save("eq_active", recolour(eq, EQ_GLYPH, PHOSPHOR))

    pl = im.crop(PL_BOX)
    save("pl_idle", pl)
    save("pl_active", recolour(pl, PL_GLYPH, PHOSPHOR))

    win_min = im.crop(WIN_MIN_BOX)
    save("win_minimize_idle", win_min)
    save("win_minimize_pressed", press(win_min))

    win_close = im.crop(WIN_CLOSE_BOX)
    save("win_close_idle", win_close)
    save("win_close_pressed", press(win_close))

    win_blank = blank_glyph(win_min, WIN_MIN_BAR, WIN_MIN_PATCH)
    save("win_blank_idle", win_blank)
    save("win_blank_pressed", press(win_blank))

    save("slider_thumb", im.crop(THUMB_BOX))

    # --- Task 7: equalizer controls ---------------------------------------
    # ON: mockup bakes it lit green -> that is the `active` sprite; `idle` is the
    # glyph recoloured to neutral label grey. AUTO is baked grey -> `idle`; its
    # `active` recolours the glyph to lit phosphor. (Mirror of Task 6 toggles.)
    eq_on = im.crop(EQ_ON_BOX)
    save("eq_on_active", eq_on)
    save("eq_on_idle", recolour(eq_on, EQ_ON_GLYPH, LABEL))

    eq_auto = im.crop(EQ_AUTO_BOX)
    save("eq_auto_idle", eq_auto)
    save("eq_auto_active", recolour(eq_auto, EQ_AUTO_GLYPH, PHOSPHOR))

    eq_presets = im.crop(EQ_PRESETS_BOX)
    save("eq_presets_idle", eq_presets)
    save("eq_presets_pressed", press(eq_presets))

    eq_collapse = im.crop(EQ_COLLAPSE_BOX)
    save("eq_collapse_idle", eq_collapse)
    save("eq_collapse_pressed", press(eq_collapse))

    eq_close = im.crop(EQ_CLOSE_BOX)
    save("eq_close_idle", eq_close)
    save("eq_close_pressed", press(eq_close))

    save("eq_thumb", im.crop(EQ_THUMB_BOX))

    print("eq:", eq_on.size, eq_auto.size, eq_presets.size,
          eq_collapse.size, eq_close.size, im.crop(EQ_THUMB_BOX).size)
    print("window:", win_min.size, win_close.size, win_blank.size)
    print("transport:", {k: im.crop(v).size for k, v in TRANSPORT.items()})
    print("shuffle:", shuffle.size, "repeat:", repeat.size,
          "eq:", eq.size, "pl:", pl.size)


if __name__ == "__main__":
    main()
