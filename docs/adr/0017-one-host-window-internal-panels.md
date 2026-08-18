# One host window with internal panels

Date: 2026-08-17  
Revised: 2026-08-18 (host geometry and main-drag mechanism moved to [ADR 0019](0019-virtual-desktop-punched-host.md))

Five frameless OS windows fought Wayland/KWin (recenter, transients, skip-taskbar, leftover system-move). Tramp now uses one OS **host window** containing five **panels**; gaps punch through to the desktop. Main title-bar drag is compositor-owned; child drags are app-owned. Stability and usability beat the old host shape.

## Status

Partially superseded by [ADR 0019](0019-virtual-desktop-punched-host.md) (host geometry, main-drag mechanism, screen-sized overlay). One OS host, five internal panels, and punched gaps remain.

Supersedes [ADR 0006](0006-multi-window-docking.md) **only** for five OS windows and compositor drag / Dialog-transient taskbar pins. Docking, snap, shade, and zoom sizing in 0006 remain.

## Decision

**Current host geometry and main-drag mechanism:** [ADR 0019](0019-virtual-desktop-punched-host.md).

One OS host window (frameless, title `Tramp` — the taskbar/pager entry). Five internal panels (main, equalizer, playlist, settings, about). Punched input so the desktop is clickable between panels.

Panel screen size comes from zoomed docking logical size, not from an unmapped widget’s `size()` (which can be 0×0 and drop the main player from the layout). Title-bar drag emits `nativeMoved`; a parented panel’s `moveEvent` does not.

## Historical (superseded by 0019)

The host window’s geometry was the **tight union** of visible panels, with main title-bar drag as compositor `startSystemMove`. A screen-sized overlay was rejected after an earlier KWin mapping failure; 0019 revisits that overlay after a mapping prototype.

## Considered options

- Keep five frameless toplevels and pin against compositor recenter / skip-taskbar transients — rejected; that host shape is the fight.
- One host window with an opaque bounding-box hit target — rejected; gaps must click through to the desktop.
