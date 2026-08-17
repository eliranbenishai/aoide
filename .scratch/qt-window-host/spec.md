# Qt window host

Five frameless OS windows, one Qt process, QWidget + QPainter.
This is the product host ([ADR 0016](../../docs/adr/0016-qt-for-v1.md)).

## Prove

- Five windows: main, equalizer, playlist, settings, about
- Per-pixel alpha (rounded panel, corners punch through)
- Extras skip the taskbar; main does not
- Title strip calls `QWindow::startSystemMove()` (synchronous on press)
- Linux feel: this host, Wayland, then one `QT_QPA_PLATFORM=xcb` run
- Windows: CI compile; drag feel later

## Build

See [`README.md`](../../README.md).

## Verdict

**Pass on this host (xcb), 2026-08-16.** Five windows move with `startSystemMove` (WM snap works). After skip-taskbar via `_NET_WM_STATE`, only main is on the taskbar. Windows drag feel still later; CI compile still the Windows bar.
