# 6. Multi-window host with Winamp-style docking

Date: 2026-08-08  
Revised: 2026-08-09 (move/snap ownership, playlist snap sides, taskbar);  
2026-08-17 (each title-bar drag moves only that window — historical);  
2026-08-17 (five OS windows superseded by ADR 0017 — docking/snap/shade remain);  
2026-08-18 (main title-bar drag translates the host; child title-bar drag moves only that panel);  
2026-08-18 (EQ snap may flush to two neighbors, one per axis)

## Status

Accepted for docking, snap, shade, and zoom-only / free-resize sizing.

Five frameless OS windows and `startSystemMove` / Dialog-transient taskbar pins are **superseded by [ADR 0017](0017-one-host-window-internal-panels.md)**.

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
The 2026-08-17 host-shape change keeps those docking rules and puts the five
surfaces inside one OS host window ([ADR 0017](0017-one-host-window-internal-panels.md)).

## Decision

- One Qt process hosts **five panels** inside one OS **host window**: Main Player
  (825×348), Equalizer (825×348), Playlist Editor (default 1073×696),
  plus freestanding settings and about. Host shape is [ADR 0017](0017-one-host-window-internal-panels.md).
- EQ and playlist may **both** be open. Main EQ/PL toggles show/hide those
  panels.
- **Move ownership:** dragging the **main** panel title bar translates every
  **panel** inside the host so the cluster moves as a unit; host size unchanged.
  Main never snaps and never creates dock edges. Dragging **EQ, playlist,
  settings, or about** moves **only that panel** in host-local space; siblings
  stay put. After map, the host is the virtual desktop and must not resize on
  child drag. Dragging an EQ or playlist title bar peels its dock edges.
- **Snap:** only when finishing an EQ or playlist drag. EQ may snap to any
  side of any other visible panel, including one vertical and one horizontal
  neighbor in the same drop. Playlist may snap only **top/bottom**; on
  that snap, also flush left or right if that edge is already within the snap
  threshold. Main never initiates snap. Thresholds live in the coordinator /
  polish design. Snap runs on EQ/PL drag end (app-owned drag; see ADR 0019).
- Undock via peel-on-EQ/PL-drag, break-threshold separation, and/or Shift.
- **Zoom-only** sizing for main and EQ; **free resize** for playlist (logical
  size persisted, scaled by global zoom) — see [ADR 0002](0002-fixed-canvas-zoom.md).
- Main close quits the app; EQ/PL close hides. Main minimize hides/restores
  visible secondaries on the host window; clutterbar always-on-top applies to
  the host window. Windowshade on EQ/PL collapses to title bar; docking uses
  shaded height.
- **Taskbar:** the host window (Tramp) is the taskbar/pager entry
  ([ADR 0017](0017-one-host-window-internal-panels.md)). Target platform remains
  **windows-x64**.
- Persist dock graph, positions, visibility, shade, and playlist logical size.

## Consequences

`DockingCoordinator` remains the pure layout seam; the session host applies
frames to each panel, then `placePanels` on the virtual-desktop host. Sticky dock edges record snap
contact for persistence and peel. The single `TrampShell` lower-region EQ/PL
swap stays removed. ADR 0003’s zoom-only main/EQ and free-resize playlist
*sizing intent* continues under this ADR and ADR 0002; its single-window
framing does not. Five-OS-window host mechanics do not continue — see ADR 0017.

## Implementation pins

- One Qt process; panels are views on `TrampSession` inside the host window.
  See [ADR 0016](0016-qt-for-v1.md) and [ADR 0017](0017-one-host-window-internal-panels.md).
- Title-bar drag: **main** translates the cluster inside the host; **EQ/PL/settings/about** move only that panel. None of these use `startSystemMove`. See [ADR 0019](0019-virtual-desktop-punched-host.md).
