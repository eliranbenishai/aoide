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
- No software raster, no docking, no mpv, no mockup port

## Build

System Qt via CMake `find_package(Qt6 COMPONENTS Widgets)`. Ship later (Flatpak/AppImage), not this tracer.

## Layout

- `qt/src/window_spec.*` — five roles, skip-taskbar, sizes (unit-tested)
- `qt/src/skip_taskbar.*` — X11 `_NET_WM_STATE_SKIP_TASKBAR` / Windows `WS_EX_TOOLWINDOW` (not `Qt::Window|Qt::Tool`)
- `qt/src/main.cpp` — one `QApplication`, five windows; closing main quits
- `qt/README.md` — build and Wayland / `xcb` runs
