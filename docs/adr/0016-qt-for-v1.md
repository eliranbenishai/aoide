# Qt 6 C++ is the Tramp v1 host

Date: 2026-08-17

## Status

Accepted

Supersedes [ADR 0001](0001-flutter-for-v1.md) and [ADR 0015](0015-one-engine-windows.md).

## Context

Flutter was locked so one desktop codebase could own custom chrome ([ADR 0001](0001-flutter-for-v1.md)). Five product windows then meant five engines; collapsing them onto one isolate ([ADR 0015](0015-one-engine-windows.md)) still left drag, compositor teardown, and experimental windowing as product risks. A Qt tracer already painted the mockup chassis and hosted live session state; that tree is now the product (`src/` at the repo root).

## Decision

The product is **Qt 6 C++** (QWidget + QPainter) in `src/`. One process, five frameless windows. Playback talks to **libmpv directly** (`MpvEngine`), not media_kit. The Flutter/Dart tree is retired; there is no second build.

Linux and Windows are the pairing hosts. macOS packaging waits on a Qt Mac host.

## Considered options

- Stay on Flutter 3.47 windowing / extra views — rejected; native drag and extra-view cost were the reason for the Qt tracer.
- Keep a frozen Dart tree for goldens — rejected; two hosts drift, and Qt `--dump-chrome` is the chrome bar.
- Tauri / Electron — still out of v1.

## Consequences

CI and release build with CMake at the repo root. Version lives in `VERSION`. Docking, zoom, and mockup-chrome *product* rules in ADRs 0002 / 0006 / 0007 still apply; their Flutter implementation pins do not. ADR 0005’s full-libmpv requirement still applies; the control seam is `PlayerEngine`, not media_kit.
