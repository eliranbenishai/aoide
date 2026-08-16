# Qt window-host tracer

Five frameless windows in one process. QWidget + QPainter. Not the product chrome.

**Prove:** stay up, per-pixel alpha, extras skip the taskbar, title strip uses `startSystemMove()`. Feel it on **this host’s Wayland session**, then once with `QT_QPA_PLATFORM=xcb`. No software raster. No docking, mpv, or mockup port.

Windows: CI compiles this target. Drag feel on Windows comes later.

## Build (system Qt)

Fedora:

```sh
sudo dnf install qt6-qtbase-devel qt6-qtwayland cmake ninja-build gcc-c++
```

Arch:

```sh
sudo pacman -S qt6-base qt6-wayland cmake ninja gcc
```

```sh
cmake -S qt -B qt/build -G Ninja
cmake --build qt/build
ctest --test-dir qt/build --output-on-failure
```

Run (Wayland — default on Fedora):

```sh
./qt/build/tramp-qt-tracer
```

X11:

```sh
QT_QPA_PLATFORM=xcb ./qt/build/tramp-qt-tracer
```

Do not use `~/.config/tramp-flutter-env.sh` (it forces X11 and Mesa for Flutter). This binary is Qt; let it see `WAYLAND_DISPLAY`.

Drag the title text to move a window. Closing **Tramp** (main) quits the process.
