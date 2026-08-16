# Qt window-host tracer

Five frameless OS windows, one Qt process, QWidget + QPainter.
Not a Flutter host. Dart tree is frozen (chrome/domain reference).

## Prove

- Five windows: main, equalizer, playlist, settings, about
- Per-pixel alpha (rounded panel, corners punch through)
- Extras skip the taskbar; main does not
- Title strip calls `QWindow::startSystemMove()` (synchronous on press)
- Linux feel: this host, Wayland, then one `QT_QPA_PLATFORM=xcb` run
- Windows: CI compile; drag feel later
- No software raster, no docking, no mpv
- Mockup chassis (follow-on): `.win` / `.tbar` / `.wbtn` at 75% native seeds; drag excludes window buttons

## Build

System Qt via CMake `find_package(Qt6 COMPONENTS Widgets)`. Ship later (Flatpak/AppImage), not this tracer.

## Layout

- `qt/src/window_spec.*` — five roles, skip-taskbar, 75% native seeds (unit-tested)
- `qt/src/mockup_tokens.h` / `tramp_metrics.h` / `title_chrome.*` — CSS palette, canvases, title hit-map
- `qt/src/chrome_paint.cpp` — mockup shell / title bar / wells
- `qt/src/skip_taskbar.*` — X11 `_NET_WM_STATE_SKIP_TASKBAR` / Windows `WS_EX_TOOLWINDOW`. Wayland: extras are `Qt::Dialog` transients of main (xdg_toplevel parent), not `Qt::Tool` (Popup bit).
- `qt/src/main.cpp` — one `QApplication`, five windows; closing main quits
- `qt/README.md` — build and Wayland / `xcb` runs

## Verdict

**Pass on this host (xcb), 2026-08-16.** Five windows move with `startSystemMove` (WM snap works). After skip-taskbar via `_NET_WM_STATE`, only main is on the taskbar. Per-pixel alpha was in the tracer. Windows drag feel still later; CI compile still the Windows bar.
