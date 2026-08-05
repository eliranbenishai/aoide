"""Author placeholder graphite chrome for the polish pass.

Run from the repo root:

    python .scratch/graphite-skin/build_polish_chrome.py

See docs/design/ASSETS_NEEDED.md — these are replaceable placeholders.
"""
from __future__ import annotations

import os

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FACE = os.path.join(ROOT, "assets", "skin", "graphite", "main_face.png")
OUT = os.path.join(ROOT, "assets", "skin", "graphite", "controls")
FONT = os.path.join(ROOT, "assets", "fonts", "BarlowSemiCondensed-Bold.ttf")

MUTE_BOX = (1472, 388, 1598, 434)  # tighter to the empty well, ~126×46
# Clean metal (no glyphs): left of transport, mid-face grain.
CLEAN_METAL = (40, 200, 280, 280)


def _font(size: int) -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype(FONT, size=size)
    except OSError:
        return ImageFont.load_default()


def _recess(img: Image.Image, shift: int = 1) -> Image.Image:
    dark = ImageEnhance.Brightness(img).enhance(0.78)
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.paste(dark, (shift, shift))
    shade = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    sd.rectangle([0, 0, img.width, max(3, img.height // 8)], fill=(0, 0, 0, 55))
    return Image.alpha_composite(out, shade)


def _synth_metal(size: tuple[int, int], donor: Image.Image) -> Image.Image:
    """Uniform graphite grain: average donor colour + fine noise (no tile seams)."""
    import random

    w, h = size
    # Sample mean RGB from donor.
    small = donor.resize((16, 16), Image.Resampling.BOX).convert("RGB")
    pixels = list(small.getdata())
    r = sum(p[0] for p in pixels) // len(pixels)
    g = sum(p[1] for p in pixels) // len(pixels)
    b = sum(p[2] for p in pixels) // len(pixels)
    rng = random.Random(42)
    out = Image.new("RGBA", (w, h))
    px = out.load()
    for y in range(h):
        for x in range(w):
            # Bias slightly lighter so engraved wells don't collapse to black.
            n = rng.randint(-8, 14)
            px[x, y] = (
                max(0, min(255, r + 8 + n)),
                max(0, min(255, g + 8 + n)),
                max(0, min(255, b + 10 + n)),
                255,
            )
    return out.filter(ImageFilter.GaussianBlur(radius=0.35))


def _draw_speaker_hires(size: int, *, muted: bool, colour: tuple[int, int, int, int]) -> Image.Image:
    scale = 4
    canvas = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
    d = ImageDraw.Draw(canvas)
    w = h = size * scale
    cx, cy = w // 2 - int(w * 0.04), h // 2  # bias left so waves fit
    body = [
        (cx - int(w * 0.30), cy - int(h * 0.16)),
        (cx - int(w * 0.08), cy - int(h * 0.16)),
        (cx + int(w * 0.16), cy - int(h * 0.38)),
        (cx + int(w * 0.16), cy + int(h * 0.38)),
        (cx - int(w * 0.08), cy + int(h * 0.16)),
        (cx - int(w * 0.30), cy + int(h * 0.16)),
    ]
    d.polygon(body, fill=colour)
    if muted:
        d.line(
            (int(w * 0.12), int(h * 0.82), int(w * 0.88), int(h * 0.18)),
            fill=colour,
            width=max(4, scale * 2),
        )
    else:
        for i, rad in enumerate((0.16, 0.26)):
            bbox = [
                cx + int(w * 0.08),
                cy - int(h * rad),
                cx + int(w * (0.08 + rad)),
                cy + int(h * rad),
            ]
            d.arc(bbox, start=-50, end=50, fill=colour, width=max(3, scale + 1 - i))
    return canvas.resize((size, size), Image.Resampling.LANCZOS)


def build_mute(face: Image.Image) -> None:
    # Normalize to 130×50 logical×2 contract.
    raw = face.crop(MUTE_BOX).convert("RGBA")
    bezel = raw.resize((130, 50), Image.Resampling.LANCZOS)
    glyph_size = 20
    label = (210, 212, 218, 255)
    dim = (140, 142, 148, 255)

    def stamp(base: Image.Image, muted: bool, colour: tuple[int, int, int, int], dx: int = 0, dy: int = 0) -> Image.Image:
        out = base.copy()
        sp = _draw_speaker_hires(glyph_size, muted=muted, colour=colour)
        out.alpha_composite(
            sp,
            ((out.width - glyph_size) // 2 + dx, (out.height - glyph_size) // 2 + dy),
        )
        return out

    idle = stamp(bezel, False, label)
    muted_img = stamp(ImageEnhance.Brightness(bezel).enhance(0.95), True, dim)
    pressed = stamp(_recess(bezel), False, (180, 182, 188, 255), dx=1, dy=1)

    idle.save(os.path.join(OUT, "mute_idle.png"))
    muted_img.save(os.path.join(OUT, "mute_muted.png"))
    pressed.save(os.path.join(OUT, "mute_pressed.png"))
    print("mute sprites", idle.size)


def _make_label_button(
    metal: Image.Image,
    text: str,
    size: tuple[int, int],
    *,
    pressed: bool,
) -> Image.Image:
    """OPEN-style: metal face with a shallow recessed label well."""
    w, h = size
    base = _synth_metal((w, h), metal)
    # Outer raised rim.
    rim = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    rd = ImageDraw.Draw(rim)
    rd.rounded_rectangle([0, 0, w - 1, h - 1], radius=4, outline=(70, 76, 86, 220), width=1)
    rd.line([(1, 1), (w - 3, 1)], fill=(110, 116, 126, 100))
    rd.line([(1, 1), (1, h - 3)], fill=(110, 116, 126, 70))
    out = Image.alpha_composite(base, rim)

    # Shallow engraved well — keep grain visible (OPEN language, not a black pill).
    well = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    wd = ImageDraw.Draw(well)
    pad = 3
    box = [pad, pad, w - pad - 1, h - pad - 1]
    wd.rounded_rectangle(box, radius=3, fill=(0, 0, 0, 45))
    wd.rounded_rectangle(box, radius=3, outline=(40, 44, 52, 180), width=1)
    wd.line([(pad + 1, pad + 1), (w - pad - 2, pad + 1)], fill=(0, 0, 0, 70))
    wd.line([(pad + 1, h - pad - 2), (w - pad - 2, h - pad - 2)], fill=(90, 96, 106, 50))
    out = Image.alpha_composite(out, well)

    if pressed:
        out = _recess(out, shift=1)

    td = ImageDraw.Draw(out)
    font = _font(17)
    colour = (215, 217, 222, 255) if not pressed else (165, 167, 172, 255)
    bbox = td.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (w - tw) // 2
    y = (h - th) // 2 - 1 + (1 if pressed else 0)
    # Soft shadow under the glyph for engraved depth.
    td.text((x + 1, y + 1), text, fill=(0, 0, 0, 90), font=font)
    td.text((x, y), text, fill=colour, font=font)
    return out.filter(ImageFilter.UnsharpMask(radius=0.5, percent=70, threshold=2))


def build_playlist_toolbar(face: Image.Image) -> None:
    metal = face.crop(CLEAN_METAL).convert("RGBA")
    specs = [
        ("pl_load", "LOAD", (108, 44)),
        ("pl_save", "SAVE", (108, 44)),
        ("pl_add", "ADD", (96, 44)),
    ]
    for name, label, size in specs:
        idle = _make_label_button(metal, label, size, pressed=False)
        pressed = _make_label_button(metal, label, size, pressed=True)
        idle.save(os.path.join(OUT, f"{name}_idle.png"))
        pressed.save(os.path.join(OUT, f"{name}_pressed.png"))
        print(f"{name}", size)


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    face = Image.open(FACE).convert("RGBA")
    build_mute(face)
    build_playlist_toolbar(face)
    print("done")


if __name__ == "__main__":
    main()
