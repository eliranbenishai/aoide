# 5. Full libmpv bundling (features first)

Date: 2026-08-08

## Status

Accepted

## Context

Tramp needs audible equalizer graphs, real spectrum analysis support paths, and
force-mono downmix. Prior research showed the **compressed / slim** libmpv
binaries shipped with media_kit’s default `media_kit_libs_*` packages cannot
construct filter graphs reliably (operations may report success while doing
nothing). That constraint was real for those binaries; it must not freeze the
product as chrome-only EQ forever.

## Decision

- Bundle **full libmpv** (+ required FFmpeg) for Windows, macOS, and Linux.
- Talk to libmpv through the in-process `PlayerEngine` seam (`MpvEngine` in the Qt host).
- Packaging and load paths must use **our** full binaries so the process cannot
  silently fall back to stock slim libs.
- Prioritize **features** (EQ, formats, filters) over binary size for v1.
- Ship audible EQ in the UI only after a **measurement gate**: prove the loaded
  binary is our full build on each OS, and prove measured band response.

## Consequences

Equalizer and Mono become real audio-path features rather than chrome theatre.
Spectrum prefers a PCM/analyser path to 20 bars; synthetic levels are a
hard-fail/dev signal, not the product end-state. Installer/app size grows;
accepted for v1. Historical “EQ impossible on slim libmpv” notes remain valid
as history for those builds and must not block full-libmpv work.

See product spec [`tramp-v1-spec.md`](../tramp-v1-spec.md) and design
[`2026-08-08-mockup-multiwindow-redesign-design.md`](../superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md).
