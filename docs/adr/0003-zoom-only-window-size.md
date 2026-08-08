# 3. Window size: zoom-only for main/EQ; free resize only for playlist

Date: 2026-08-02

## Status

Superseded by [ADR 0006](0006-multi-window-docking.md) (2026-08-08)

## Context

Frameless desktop players often keep edge-resize. Tramp’s main player and
equalizer are fixed logical canvases meant to match a mockup; stretching them
fights that layout. The playlist, however, must show thousands of rows and needs
a growing well.

This ADR framed those rules for a **single window** that swapped EQ and playlist
in a lower region.

## Decision

- **Main player and equalizer never freely resize or stretch** — permanent. Their
  on-screen size changes only via discrete zoom steps.
- **Equalizer mode** (lower region = EQ or windowshade): the window is not
  drag-resized; size is the panel stack at the current zoom step (plus frame).
- **Playlist mode** (lower region = playlist): the window is freely resizable in
  width and height. The main canvas stays fixed (× zoom), top-aligned; extra
  space is gutter plus the playlist well. Persist the last playlist window size.
- Title-bar zoom-in / zoom-out replace maximize as the size control for zoom.
  Free-resize “Scalable UI” for the whole chrome remains out of scope for v1.

## Consequences

Shell enables resize edges only in playlist mode. EQ ↔ PL switches snap or
restore sizes as above. Playlist chrome must be 9-sliced (or equivalent), not a
single fixed-height face. See [ADR 0004](0004-png-graphite-skin.md) and
[`graphite-skin-delivery-design`](../superpowers/specs/2026-08-02-graphite-skin-delivery-design.md).

**Supersession note:** The product model is now three detachable dockable windows
with global zoom ([ADR 0006](0006-multi-window-docking.md),
[ADR 0002](0002-fixed-canvas-zoom.md)). Zoom-only main/EQ and free-resize playlist
*sizing rules* continue in spirit; the single-window EQ/PL mutual-exclusion framing
and PNG nine-slice consequences do not.
