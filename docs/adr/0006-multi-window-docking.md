# 6. Multi-window host with Winamp-style docking

Date: 2026-08-08

## Status

Accepted

Supersedes [ADR 0003](0003-zoom-only-window-size.md) for product window model
(single-window EQ/PL mutual exclusion).

## Context

Classic Winamp separates main, equalizer, and playlist into detachable windows
that snap and drag as a group. Tramp’s prior v1 model used one frameless window
and swapped EQ vs playlist in a lower region — simpler to ship, but not the
mockup direction and not the Winamp-shaped product users expect.

## Decision

- One Flutter process hosts **three** frameless OS windows: Main Player
  (825×348), Equalizer (825×348), Playlist Editor (default 825×696).
- EQ and playlist may **both** be open. Main EQ/PL toggles show/hide those
  windows.
- **Docking:** snap when edges/corners approach (thresholds in the
  implementation plan); docked windows form a group — dragging one moves the
  group; undock via break-threshold drag and/or modifier.
- **Zoom-only** sizing for main and EQ; **free resize** for playlist (logical
  size persisted, scaled by global zoom) — see [ADR 0002](0002-fixed-canvas-zoom.md).
- Main close quits the app; EQ/PL close hides. Main minimize and clutterbar
  always-on-top apply to the **visible docked group**. Windowshade on EQ/PL
  collapses to title bar; docking uses shaded height.
- Persist dock graph, positions, visibility, shade, and playlist logical size.

## Consequences

`DockingCoordinator` (or equivalent) becomes a core session seam. The single
`TrampShell` lower-region EQ/PL swap is removed as the product model. Platform
multi-window APIs and persistence grow in complexity; accepted to match
`player-mockup-2.html` and the redesign design doc. ADR 0003’s zoom-only main/EQ
and free-resize playlist *sizing intent* continues under this ADR and ADR 0002;
its single-window framing does not.
