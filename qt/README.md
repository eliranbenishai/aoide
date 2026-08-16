# Qt window host

Five frameless windows in one process. QWidget + QPainter.

**Now:** mockup chassis (`.win` shell, `.tbar`, `.wbtn`, phosphor grips, `.screen` wells) at the 75% native seed. Title-strip drag uses `startSystemMove()`. Extras skip the taskbar. Closing **Tramp** (main) quits.

**Not yet:** docking, libmpv, interactive player/EQ/playlist chrome, zoom stepping.

Windows: CI compiles this target. Drag feel on Windows comes later.

## Build (system Qt)

Fedora:

```sh
sudo dnf install qt6-qtbase-devel qt6-qtwayland libX11-devel cmake ninja-build gcc-c++
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

Drag the title strip (not the window buttons) to move a window. Closing **Tramp** (main) quits the process.
