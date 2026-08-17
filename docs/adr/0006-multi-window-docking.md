# 6. Multi-window host with Winamp-style docking

Date: 2026-08-08  
Revised: 2026-08-09 (move/snap ownership, playlist snap sides, taskbar);  
2026-08-17 (each title-bar drag moves only that window)

## Status

Accepted

Supersedes [ADR 0003](0003-zoom-only-window-size.md) for product window model
(single-window EQ/PL mutual exclusion).

## Context

Classic Winamp separates main, equalizer, and playlist into detachable windows
that snap to each other’s edges. Tramp’s prior v1 model used one frameless window
and swapped EQ vs playlist in a lower region — simpler to ship, but not the
mockup direction and not the Winamp-shaped product users expect.

The 2026-08-09 polish pass tightened move/snap ownership and Windows taskbar
behavior; see
[`2026-08-09-ui-polish-docking-taskbar-design.md`](../superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md).

## Decision

- One Qt process hosts **five** frameless OS windows: Main Player
  (825×348), Equalizer (825×348), Playlist Editor (default 1073×696),
  plus freestanding settings and about.
- EQ and playlist may **both** be open. Main EQ/PL toggles show/hide those
  windows.
- **Move ownership:** each window’s title-bar drag moves **only** that window.
  Dragging an EQ or playlist title bar peels its dock edges. Hidden windows
  are unchanged.
- **Snap:** only when finishing an EQ or playlist drag. EQ may snap to any
  side of any other visible window. Playlist may snap only **top/bottom**; on
  that snap, also flush left or right if that edge is already within the snap
  threshold. Main never initiates snap. Thresholds live in the coordinator /
  polish design. Snap runs on EQ/PL drag end (not during `startSystemMove`).
- Undock via peel-on-EQ/PL-drag, break-threshold separation, and/or Shift.
- **Zoom-only** sizing for main and EQ; **free resize** for playlist (logical
  size persisted, scaled by global zoom) — see [ADR 0002](0002-fixed-canvas-zoom.md).
- Main close quits the app; EQ/PL close hides. Main minimize hides/restores
  visible secondaries; clutterbar always-on-top applies to visible tramp
  windows. Windowshade on EQ/PL collapses to title bar; docking uses shaded
  height.
- **Taskbar (Windows):** only the main window appears in the taskbar
  (extras are `Qt::Dialog` transients / skip-taskbar). Target platform remains
  **windows-x64**.
- Persist dock graph, positions, visibility, shade, and playlist logical size.

## Consequences

`DockingCoordinator` remains the pure layout seam; the session host applies
frames to each OS window. Sticky dock edges record snap contact for persistence
and peel. The single `TrampShell` lower-region EQ/PL swap stays removed. ADR
0003’s zoom-only main/EQ and free-resize playlist *sizing intent* continues
under this ADR and ADR 0002; its single-window framing does not.

## Implementation pins

- One Qt process; extra OS windows are `HostWindow` views on `TrampSession`.
  See [ADR 0016](0016-qt-for-v1.md).
- Title-bar drag is `QWindow::startSystemMove()`. Extras skip the taskbar as
  `Qt::Dialog` transients of main.
