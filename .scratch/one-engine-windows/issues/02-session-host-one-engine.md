# 02 — Session host on one engine

Status: resolved
Type: task

## Goal

Replace `desktop_multi_window` secondary engines + `SessionClientApp` with extra Flutter views on the existing isolate (`OsWindow` + `ViewAnchor`). Native `begin_move_drag` stays. Do not wrap product chrome in `RegularWindow` / listen-on-configure.

## Done when

- `main.dart` force-enables windowing, then runs only `SessionHostApp`
- EQ / playlist / settings / about attach via `OsWindow.attach` (no `ListenableBuilder` on the windowing controller)
- Title-bar drag on extras calls that window's native `startDrag`, not `windowManager.startDragging` (main HWND)
- Architecture doc + ADR pins match

## Answer

Product host is one isolate. `enableTrampWindowing()` runs before the binding; extras are `OsWindow.attach` under `ViewAnchor`. Native startDrag is synchronous (not `async`). `RegularWindow` widget is not used. `SessionClientApp` remains unused leftover until macOS native is real.

Feel EQ+PL drag on this host before calling it fixed.
