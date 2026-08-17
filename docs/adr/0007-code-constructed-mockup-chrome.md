# 7. Code-constructed chrome from the HTML mockup

Date: 2026-08-08  
Revised: 2026-08-09 (title compact mode, EQ band fill, button bevel)

## Status

Accepted

Supersedes [ADR 0004](0004-png-graphite-skin.md).

## Context

The PNG-first graphite skin delivered a shippable look but diverged from the
locked visual authority in [`player-mockup-2.html`](../../player-mockup-2.html)
(geometry, tokens, materials, three-window layout). Maintaining a crop/slice
pipeline and bitmap faces fights exact mockup fidelity and blocks an atomic
chrome cutover.

## Decision

- Deliver built-in chrome as **code-constructed** Qt painting (QPainter layered
  fills, paths, and type) that matches the mockup CSS/markup at 100% zoom —
  geometry, tokens, type, gradients, shadows, radii, spacing, icon paths.
- The HTML/CSS mockup is the recipe; checked-in token + geometry maps mirror
  mockup `:root` / rules. Do not invent a parallel theme.
- **Retire** PNG panel faces, nine-slice graphite pack, and the crop pipeline
  from the product path (`GraphiteSkin` / `assets/skin/graphite/`).
- Brand mark remains vector (mockup embedded logo) where needed.
- Chrome cutover is **atomic** for all three windows — no mixed
  graphite/mockup UI.
- **Product overrides** (intentional vs HTML mockup), pinned in
  [`2026-08-09-ui-polish-docking-taskbar-design.md`](../superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md):
  - EQ / playlist title bars show **role title only** (no logo disc / TRAMP
    wordmark); main keeps full brand title strip.
  - Title-bar window buttons match mockup `.wbtn` **bevel/inset chrome**
    (icons already match).
  - EQ band faders add a bottom→thumb **spectrum-gradient fill** (mockup HTML
    bands have no fill; product wants parity with main vol/seek fill language).

## Consequences

Visual fidelity is verified by side-by-side screenshot diffs against the mockup
for shared materials, plus the product overrides above. ADR 0004 remains
historical only. Agents must not follow PNG-graphite delivery docs as the
current look. Fixed-canvas zoom ([ADR 0002](0002-fixed-canvas-zoom.md)) still
applies; construction is code, not master-density PNGs scaled under the
transform.

See designs
[`2026-08-08-mockup-multiwindow-redesign-design.md`](../superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md)
and
[`2026-08-09-ui-polish-docking-taskbar-design.md`](../superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md).
