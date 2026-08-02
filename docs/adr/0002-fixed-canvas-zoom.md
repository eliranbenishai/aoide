# 2. Fixed logical canvas plus a single transform for zoom

Date: 2026-08-02

## Status

Accepted

## Context

The graphite chrome redesign must support several zoom levels for high-DPI
displays while looking identical at every one. Two approaches were considered:
scaling a fixed logical canvas with one transform, or threading a scale factor
through every metric so widgets lay out natively at the scaled size.

## Decision

Each panel is authored once against a fixed logical canvas — main player
812x242, equalizer 812x206 — and the zoom factor is applied by a single
`Transform.scale` at the root of the panel stack.

## Consequences

There is exactly one source of geometry, so no zoom step can drift from the
reference mockup, and adding a step costs nothing. Because every surface is
vector and Flutter rasterizes text at final device resolution, output is crisp
at any factor.

The cost is that 1px bevels land on fractional device pixels at 125% and 150%.
`ZoomScope.snap` rounds hairlines to whole device pixels to compensate.

The rejected alternative — scaled metric tokens — keeps hairlines exact without
snapping, but requires every widget to consume metrics correctly. A single
widget forgetting one silently breaks visual fidelity, which is the specific
failure this redesign cannot tolerate.

Still accepted under the PNG graphite skin ([ADR 0004](0004-png-graphite-skin.md)):
one master-density asset set scales with the same root transform. Hairline snap
matters less once faces are bitmaps; the fixed-canvas + zoom-step model remains.
