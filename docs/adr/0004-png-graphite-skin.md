# 4. PNG-first graphite skin for chrome look

Date: 2026-08-02

## Status

Accepted

## Context

Coded vector chrome (`MetalPanel`, painted bevels, etc.) can approximate the mockup but burns time on materials instead of fidelity. With fixed canvases and discrete zoom only, a Winamp-style asset skin becomes practical: the window no longer stretches arbitrary bitmaps under free resize.

## Decision

The built-in **graphite skin** delivers the look of the main player, equalizer, and playlist primarily as PNG assets (one high master density, scaled with the zoom step). Code owns hit targets, which asset state is shown, slider thumb position, spectrum visualization, and all LCD/playlist text. SVG remains for brand marks where already useful. This is not classic Winamp WSZ loading — that stays out of v1.

## Consequences

Visual fidelity tracks art direction and slice quality, not painter math. Zoom stays [ADR 0002](0002-fixed-canvas-zoom.md)’s single root scale; odd steps may be slightly softer than integer densities — accepted for v1 over per-step exports. Much of `lib/ui/chrome/`’s painted surfaces become hit-target and overlay machinery over skin art rather than the source of the look.
