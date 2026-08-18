# One host window with internal panels

Date: 2026-08-17  
Revised: 2026-08-18 (main drag translates the host; child drag tight-union + local compensation)

Five frameless OS windows fought Wayland/KWin (recenter, transients, skip-taskbar, leftover system-move). Tramp now uses one OS **host window** containing five **panels**; gaps punch through to the desktop; drag is app-owned. Stability and usability beat the old host shape.

## Status

Accepted

Supersedes [ADR 0006](0006-multi-window-docking.md) **only** for five OS windows and compositor drag / Dialog-transient taskbar pins. Docking, snap, shade, and zoom sizing in 0006 remain.

## Decision

One OS host window (frameless, title `Tramp` — the taskbar/pager entry). Five internal panels (main, equalizer, playlist, settings, about). Punched input so the desktop is clickable between panels. App-owned drag — not compositor `startSystemMove`.

The host window’s geometry is the **tight union** of visible panels (origin and size). The punched mask is the union of those panel rects.

Dragging the **main** panel title bar **translates the host**. Every panel keeps its position inside the host; the cluster moves on screen as a unit; host size is unchanged. The coordinator translates **every** frame (including hidden settings/about) by the same logical delta. Main never snaps and never creates dock edges. Moving the frameless toplevel on main-drag is **on purpose** — that is how the cluster translates.

Dragging **EQ, playlist, settings, or about** moves **only that panel** in screen space. Siblings stay put on screen. The host bounding box grows and shrinks with the union; the host origin moves when the union’s top-left changes. After an origin change, re-place panels so non-dragged siblings keep their screen positions (do not shift everyone). Child-drag origin changes must compensate locals so siblings do not travel.

A screen-sized “virtual desktop” overlay remains rejected: KWin does not map that as punch-click chrome, so the player never appears.

Panel screen size comes from zoomed docking logical size, not from an unmapped widget’s `size()` (which can be 0×0 and drop the main player from the layout). Title-bar drag emits `nativeMoved`; a parented panel’s `moveEvent` does not.

## Considered options

- Keep five frameless toplevels and pin against compositor recenter / skip-taskbar transients — rejected; that host shape is the fight.
- One host window with an opaque bounding-box hit target — rejected; gaps must click through to the desktop.
