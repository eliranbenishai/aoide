# 6. Multi-window host with Winamp-style docking

Date: 2026-08-08  
Revised: 2026-08-09 (move/snap ownership, playlist snap sides, taskbar);  
2026-08-12 (main drag carries dock-edge cohort only)

## Status

Accepted

Supersedes [ADR 0003](0003-zoom-only-window-size.md) for product window model
(single-window EQ/PL mutual exclusion).

## Context

Classic Winamp separates main, equalizer, and playlist into detachable windows
that snap and drag as a group. Tramp’s prior v1 model used one frameless window
and swapped EQ vs playlist in a lower region — simpler to ship, but not the
mockup direction and not the Winamp-shaped product users expect.

The 2026-08-09 polish pass tightened move/snap ownership and Windows taskbar
behavior; see
[`2026-08-09-ui-polish-docking-taskbar-design.md`](../superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md).

## Decision

- One Flutter process hosts **three** frameless OS windows: Main Player
  (825×348), Equalizer (825×348), Playlist Editor (default 825×696).
- EQ and playlist may **both** be open. Main EQ/PL toggles show/hide those
  windows.
- **Move ownership:** dragging the **main** title bar translates its
  **dock-edge cohort** (windows currently snapped to main, including
  transitive links such as PL→EQ→main) by the same delta. Free / undocked
  windows stay put. Dragging an EQ or playlist title bar moves **only** that
  window (peel dock edges on drag). Hidden windows never follow.
- **Snap:** only when finishing an EQ or playlist drag. EQ may snap to any
  side of any other visible window. Playlist may snap only **top/bottom**; on
  that snap, also flush left or right if that edge is already within the snap
  threshold. Main never initiates snap. Thresholds live in the coordinator /
  polish design. On Linux, `window_manager` never emits `onWindowMoved`, so
  quiet soft-end finalize must still snap (and apply the snapped frame) or
  dock edges never form and main cannot carry satellites.
- Undock via peel-on-EQ/PL-drag, break-threshold separation, and/or Shift.
- **Zoom-only** sizing for main and EQ; **free resize** for playlist (logical
  size persisted, scaled by global zoom) — see [ADR 0002](0002-fixed-canvas-zoom.md).
- Main close quits the app; EQ/PL close hides. Main minimize hides/restores
  visible secondaries; clutterbar always-on-top applies to visible tramp
  windows. Windowshade on EQ/PL collapses to title bar; docking uses shaded
  height.
- **Taskbar (Windows):** only the main window appears in the taskbar
  (`skipTaskbar` on secondaries; Windows-only style fallback if the plugin
  ignores it). Target platform remains **windows-x64**.
- Persist dock graph, positions, visibility, shade, and playlist logical size.

## Consequences

`DockingCoordinator` remains the pure layout seam; session host/client apply
frames. Sticky dock edges record snap contact for persistence and peel, and
**gate** whether main carries satellites (`moveCohortOf` → `groupOf`). The
single `TrampShell` lower-region EQ/PL swap stays removed. ADR 0003’s zoom-only
main/EQ and free-resize playlist *sizing intent* continues under this ADR and
ADR 0002; its single-window framing does not.

## Implementation pins

- `desktop_multi_window` `0.3.0` (MixinNetwork) — one process, three Flutter
  engines; secondary plugin registration via platform window-created callbacks.
- `window_manager` git fork required by that plugin’s README:

  ```yaml
  window_manager:
    git:
      url: https://github.com/boyan01/window_manager.git
      path: packages/window_manager
      ref: 6fae92d21b4c80ce1b8f71c1190d7970cf722bd4
  ```
