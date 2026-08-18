# One host window with internal panels

Date: 2026-08-17

Five frameless OS windows fought Wayland/KWin (recenter, transients, skip-taskbar, leftover system-move). Tramp now uses one OS **host window** containing five **panels**; gaps punch through to the desktop; drag is app-owned. Stability and usability beat the old host shape.

## Status

Accepted

Supersedes [ADR 0006](0006-multi-window-docking.md) **only** for five OS windows and compositor drag / Dialog-transient taskbar pins. Docking, snap, shade, and zoom sizing in 0006 remain.

## Decision

One OS host window (frameless, title `Tramp` — the taskbar/pager entry). Five internal panels (main, equalizer, playlist, settings, about). Punched input so the desktop is clickable between panels. App-owned drag — not compositor `startSystemMove`.

The host window covers the **virtual desktop** (union of screens). Panels are children placed in that space; the punched mask is the union of visible panel rects. The host does **not** move when a panel is dragged — Wayland/KWin will not reliably move a frameless toplevel, and a bounding-box host that `setGeometry`s on every drag translates every panel together.

Panel screen size comes from zoomed docking logical size, not from an unmapped widget’s `size()` (which can be 0×0 and drop the main player from the layout). Title-bar drag emits `nativeMoved`; a parented panel’s `moveEvent` does not.

## Considered options

- Keep five frameless toplevels and pin against compositor recenter / skip-taskbar transients — rejected; that host shape is the fight.
- One host window with an opaque bounding-box hit target — rejected; gaps must click through to the desktop.
