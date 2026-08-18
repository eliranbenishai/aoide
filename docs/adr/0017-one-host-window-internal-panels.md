# One host window with internal panels

Date: 2026-08-17  
Revised: 2026-08-18 (main drag = host `startSystemMove`; child drag = host-local, no origin `setGeometry`)

Five frameless OS windows fought Wayland/KWin (recenter, transients, skip-taskbar, leftover system-move). Tramp now uses one OS **host window** containing five **panels**; gaps punch through to the desktop. Main title-bar drag is compositor-owned; child drags are app-owned. Stability and usability beat the old host shape.

## Status

Accepted

Supersedes [ADR 0006](0006-multi-window-docking.md) **only** for five OS windows and compositor drag / Dialog-transient taskbar pins. Docking, snap, shade, and zoom sizing in 0006 remain.

## Decision

One OS host window (frameless, title `Tramp` — the taskbar/pager entry). Five internal panels (main, equalizer, playlist, settings, about). Punched input so the desktop is clickable between panels.

The host window’s geometry is the **tight union** of visible panels. The punched mask is the union of those panel rects.

Dragging the **main** panel title bar **translates the host**. That is a compositor interactive move (`QWindow::startSystemMove` on the host) so the cluster rides as a unit; child locals stay put. Main never snaps and never creates dock edges. Hidden settings/about frames are translated by the same delta when the drag ends (they are not on the surface). Offscreen QtTest will honor a programmatic `setGeometry` origin; Wayland/KWin often will not — do not use that as proof the OS window moved.

Dragging **EQ, playlist, settings, or about** is **app-owned**: move only that panel in host-local space. Siblings stay put on screen. After map, do not `setGeometry` a new origin (that is what shoves siblings the opposite way when the compositor ignores the move). The host may **resize** to the union (grow and shrink). If a child crosses the current top-left, locals may go negative rather than shifting everyone.

A screen-sized “virtual desktop” overlay remains rejected: KWin does not map that as punch-click chrome, so the player never appears.

Panel screen size comes from zoomed docking logical size, not from an unmapped widget’s `size()` (which can be 0×0 and drop the main player from the layout). Title-bar drag emits `nativeMoved`; a parented panel’s `moveEvent` does not.

## Considered options

- Keep five frameless toplevels and pin against compositor recenter / skip-taskbar transients — rejected; that host shape is the fight.
- One host window with an opaque bounding-box hit target — rejected; gaps must click through to the desktop.
