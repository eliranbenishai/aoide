# 2. Fixed logical canvas plus a single transform for zoom

Date: 2026-08-02

## Status

Accepted (revised 2026-08-08: three canvases, global zoom, playlist free resize)

## Context

Tramp must support several zoom levels for high-DPI displays while looking
identical at every step. Two approaches were considered: scaling a fixed logical
canvas with one transform, or threading a scale factor through every metric so
widgets lay out natively at the scaled size.

The mockup multi-window redesign keeps that model but applies it across **three**
windows (main, equalizer, playlist) with one **global** zoom step, and allows
the playlist window free resize in logical coordinates.

## Decision

- Each fixed panel is authored once against a fixed logical canvas — main player
  **825×348**, equalizer **825×348** — and zoom is applied by scaling that
  logical size (root transform / equivalent per window).
- **One global** discrete zoom step (six steps 100–300% unless the product spec
  re-pins the ladder). Main title-bar −/+ (and shortcuts) change the step for
  **all three** windows. Pixel size = logical size × zoom.
- The **playlist** window is freely resizable in width and height. User size is
  stored in **logical** coordinates and scaled by the current zoom step. Default
  logical size **825×696**.

## Consequences

There is exactly one source of geometry per canvas, so no zoom step can drift
from the mockup reference, and adding a step costs nothing. Hairlines may land
on fractional device pixels at odd steps; snap helpers remain available where
needed.

Per-window independent zoom is rejected — it would desync docked groups and
fight the Winamp-style multi-window model ([ADR 0006](0006-multi-window-docking.md)).

The rejected alternative — scaled metric tokens without a fixed canvas — keeps
hairlines exact without snapping, but requires every widget to consume metrics
correctly. A single widget forgetting one silently breaks visual fidelity.

Chrome construction is code-from-mockup ([ADR 0007](0007-code-constructed-mockup-chrome.md)),
not bitmap faces scaled under the transform. Prior canvas sizes (812×242 /
812×206) and the single-window stack framing in [ADR 0003](0003-zoom-only-window-size.md)
are superseded for product sizing.
