# 15. One Flutter engine, several OS windows

Date: 2026-08-16

## Status

Superseded by [ADR 0016](0016-qt-for-v1.md).

The surviving product rule is **one process, several OS windows**. The Flutter engine / isolate mechanics below are historical.

## Context

Five product windows on `desktop_multi_window` meant **five Flutter engines**.
After Impeller was off, solo-main drag was smooth; EQ+PL still lagged because
each extra engine billed `getPosition` / compositor work. Flutter 3.47’s
experimental windowing API can attach extra `FlutterView`s to the **existing**
isolate, but its `RegularWindow` widget `ListenableBuilder`s on every GTK
configure — that rebuild-on-move is choppy during native drag.

Stable 3.47 rejects `--dart-define=FLUTTER_ENABLED_FEATURE_FLAGS=windowing` and
has no `flutter config --enable-windowing`. CI stays on 3.47.0 (not master).

## Decision

- Keep Flutter **3.47.0**. Force `isWindowingEnabled = true` in Dart **before**
  `WidgetsFlutterBinding.ensureInitialized()`.
- One isolate: `SessionHostApp` owns all chrome. Extra windows are `OsWindow`
  views (`View` + `ViewAnchor` / `ViewCollection`), not new engines.
- Do **not** use the `RegularWindow` widget in product chrome. Native
  `begin_move_drag` / `SC_MOVE` stays. Fill missing APIs (position, skip-taskbar,
  frameless, start-drag, resize) via GTK / Win32 on the windowing HWND.
- `desktop_multi_window` secondary engines are no longer the product path
  ([ADR 0006](0006-multi-window-docking.md) docking rules are unchanged).

## Considered options

- Stay on five engines and optimize dock-follow — rejected; the bill was the
  extra engines, not follow.
- Ship Flutter `RegularWindow` as the surface — rejected; configure rebuilds
  made drag choppy even with one extra view.
- Move CI to Flutter master for the official windowing flag — rejected.

## Consequences

macOS extra-window native (NSWindow FFI) is still stubbed. The vendored
`desktop_multi_window` plugin remains in the tree until that host is proven.
Session command types stay; the method-channel bus is unused by the host.
