# Tramp logo design

> **Superseded 2026-08-02.** The mark is now externally authored artwork at
> `lib/ui/chrome/logo.svg`, rendered by `TrampLogo` (`lib/ui/chrome/logo.dart`)
> via `flutter_svg`. The `CustomPainter` this spec describes has been deleted.
> Kept as a record of the original design intent.

**Date:** 2026-08-01
**Component:** `lib/ui/chrome/tramp_logo.dart` (`TrampLogo`, `TrampLogoPainter`)
**Parent direction:** [`2026-08-01-classic-main-player-design.md`](2026-08-01-classic-main-player-design.md)

## Goal

An original pin-up mark for Tramp: a woman's head only, eyes closed, listening through over-ear headphones. Glamour is deliberately exaggerated to play against the app's name. Drawn entirely in vector paths — no raster assets, no SVG dependency.

## Decisions locked

| Aspect | Decision |
|--------|----------|
| Framing | Head and hair only; no body |
| Pose | Head tilted back ~18°, turned ~20°; chin lifted, lost in the music |
| Style | Flat cel shading — base fill + hard-edged shadow block + hard-edged highlight block |
| Hair | Platinum blonde, big rolled crown, long waves falling back |
| Headphones | Retro over-ear cans; chrome band, brushed cup, dark leather pad |
| Rendering | `CustomPainter` in unit space `[0,1]²`; no gradients, no blur |
| Size | Undetermined — placement and scale decided later with app design |

## Composition

She faces the viewer's **left** with her chin lifted, so the jawline is the mark's strongest diagonal and the throat opens beneath it. The back of her head and the full hair mass therefore fall to the **right**, sweeping down and back as if gravity is pulling them away from the tilt.

Because the head is turned, the near ear sits right of centre. That cup is drawn in full; the far cup is buried in hair. Two symmetrical cups would flatten the pose.

Approximate anchors in unit space:

| Feature | Position |
|---------|----------|
| Crown | `(0.50, 0.18)` |
| Brow | `(0.29, 0.35)` |
| Nose tip | `(0.25, 0.44)` |
| Lips | `(0.29, 0.52)` |
| Chin | `(0.33, 0.61)` |
| Jaw angle | `(0.57, 0.58)` |
| Ear / near cup | `(0.60, 0.45)` |

## Palette

| Surface | Base | Shadow | Highlight |
|---------|------|--------|-----------|
| Skin | `0xFFF6C9A4` | `0xFFD9976E` | `0xFFFFE3C8` |
| Platinum hair | `0xFFF0E0B8` | `0xFFCBA96A` | `0xFFFFF6DC` |
| Lips | `0xFFD8384F` | `0xFFA02338` | `0xFFF2697A` |
| Chrome | `0xFFB8BCC0` | `0xFF6E7378` | `0xFFF0F0F0` |

Liner, lashes, and brows use `0xFF1A1214`. Ear pad uses `0xFF2A2E33`. A warm rim light `0x66FFE9D0` traces the jaw and throat so the mark separates from dark chrome.

## Shading rules

- Light comes from the **upper left**.
- Each surface is a flat base fill, then at most one shadow block and one highlight block, both hard-edged.
- Tone blocks are drawn oversized and clipped to their parent silhouette (`canvas.clipPath`), so a shadow can be shaped loosely without spilling past the edge.
- No `MaskFilter.blur` and no gradients anywhere. Soft falloff is what makes hand-coded paths read as approximate; hard edges read as deliberate.

## Where the exaggeration lives

Heavy winged lashes, strongly arched brows, full parted lips with a wet highlight, an elongated neck against a sharp jaw, oversized hair volume, and a beauty mark.

The register stays glamour-poster rather than explicit. Beyond being the classic pin-up idiom, this mark is a likely future taskbar and dock icon and has to sit alongside ordinary desktop apps.

## Painter structure

`TrampLogoPainter.paint` scales the canvas by `size.shortestSide` and draws back to front:

1. `_paintHairBack` — mass behind the head and neck
2. `_paintNeck` — neck and throat with jaw shadow
3. `_paintFace` — silhouette, cheek and jaw shadow, forehead and cheekbone highlight
4. `_paintFeatures` — brows, closed lashes, nose, lips, beauty mark
5. `_paintHairFront` — rolled crown, face-framing locks, highlight bands
6. `_paintHeadphones` — band over the crown, near cup, pad, specular wedge
7. `_paintRimLight` — jaw and throat edge

`TrampLogo`'s public API is unchanged (`const TrampLogo({super.key, this.size = 28})`), so `ClassicMainPlayer` and the existing tests in `test/ui/chrome/tramp_logo_test.dart` are untouched.

## Verification

- Render the painter to PNG at 512px through `PictureRecorder` and review it visually, iterating until the pose and shading hold up. Preview harness is throwaway and removed once the mark is locked.
- `flutter test` must stay green; the two existing logo tests assert widget presence and `CustomPaint` usage, both preserved.
- No golden test — the parent spec explicitly does not require pixel-identical goldens for chrome.

## Doc impact

The parent spec's Logo section describes a "soft illustrative pin-up" with "warm shading via gradients"; it is corrected to cel shading and platinum blonde. `docs/architecture.md` needs no change — `TrampLogoPainter` is already mapped and no module boundary moves.
