"""Crop the graphite skin's control art out of the chrome mockup.

Run from the worktree root (the repo root of the worktree):

    python .scratch/graphite-skin/crop_controls.py

Outputs (RGBA, authored at 2x like the panel faces):

    assets/skin/graphite/controls/transport_play_idle.png      138 x 80
    assets/skin/graphite/controls/transport_play_pressed.png   138 x 80
    assets/skin/graphite/controls/slider_thumb.png              32 x 45

PNG-first, per the skin's hard rule: the brushed-metal grain, bevels and the
phosphor-green play glyph all come straight from the mockup pixels. Nothing is
redrawn as a flat gradient.

  - The transport row is authored at 2x (main panel = 1624px = 812 logical), so
    a logical 69x40 button is a 138x80 mockup crop. Button pitch is ~140px; the
    five bezels (prev/play/pause/stop/next) start at x = 98/238/378/518/658 with
    tops at y=380. Only play is needed for Task 3's tests; the rest are cropped
    in Task 6-7.
  - No "pressed" button is drawn in the mockup, so the pressed sprite is derived
    from the idle crop's real pixels: darkened (recessed metal) with a top inner
    shadow and the whole face nudged 1px down-right so the glyph reads as pushed
    in. This is a tonal transform of genuine grain, not a flat redraw.
  - The slider thumb is the equalizer band's metal fader grip (x=377..409,
    y=728..773). Its ~1:1.4 grip aspect is real art; call sites pick thumbSize.

Coordinates were measured with .scratch/graphite-skin/probe_transport.py.
"""
import os

from PIL import Image, ImageEnhance

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "docs", "mockups", "graphite-chrome.png")
OUT_DIR = os.path.join(ROOT, "assets", "skin", "graphite", "controls")

# Transport button crop boxes in mockup px (left, top, right, bottom).
PLAY_BOX = (238, 380, 376, 460)  # 138 x 80
# Equalizer band metal fader grip.
THUMB_BOX = (377, 728, 409, 773)  # 32 x 45


def press(idle):
    """Derive a pressed sprite from the idle crop: recessed, real grain kept.

    Darken the whole face, nudge the content 1px down-right (pushed in), then
    lay a soft top-edge inner shadow so the bezel reads as inverted.
    """
    w, h = idle.size
    pressed = ImageEnhance.Brightness(idle).enhance(0.80)

    # Nudge down-right by 1px; the exposed top/left edges take the face's own
    # top row/column so no transparent seam appears.
    shifted = Image.new("RGBA", (w, h))
    shifted.paste(pressed, (1, 1))
    shifted.paste(pressed.crop((0, 0, w, 1)), (0, 0))
    shifted.paste(pressed.crop((0, 0, 1, h)), (0, 0))
    pressed = shifted

    # Top inner shadow: opaque black fading over the top ~14px.
    shadow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sp = shadow.load()
    depth = 14
    for y in range(depth):
        a = int(150 * (1 - y / depth))
        for x in range(w):
            sp[x, y] = (0, 0, 0, a)
    return Image.alpha_composite(pressed, shadow)


def main():
    im = Image.open(SRC).convert("RGBA")
    os.makedirs(OUT_DIR, exist_ok=True)

    play_idle = im.crop(PLAY_BOX)
    play_idle.save(os.path.join(OUT_DIR, "transport_play_idle.png"))
    press(play_idle).save(os.path.join(OUT_DIR, "transport_play_pressed.png"))

    thumb = im.crop(THUMB_BOX)
    thumb.save(os.path.join(OUT_DIR, "slider_thumb.png"))

    print("play idle/pressed:", play_idle.size, "thumb:", thumb.size)


if __name__ == "__main__":
    main()
