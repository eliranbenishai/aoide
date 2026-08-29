# Aoide

Multi-platform desktop music player. See [`docs/aoide-v1-spec.md`](docs/aoide-v1-spec.md)
for product scope.

The app is **Qt 6** (QWidget + QPainter) in [`src/`](src/).

## Development

Qt is pinned in [`QT_VERSION`](QT_VERSION). `./build.sh` fetches that official
kit (once) into `.local/qt/` and will not link Homebrew or distro Qt.

```bash
./build.sh
./build/aoide
```

CMake, after the same kit is present (`./tool/fetch_qt.sh`):

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/aoide
```

Linux still needs the usual build tools and libmpv headers:

```bash
# Fedora
sudo dnf install libX11-devel cmake ninja-build gcc-c++ mpv-libs-devel

# Arch
sudo pacman -S cmake ninja gcc mpv
```

The chrome is rasterised on the CPU every frame, so an unoptimised build does not
just benchmark badly — panels drag at a few frames per second. `build.sh`
compiles `-O2` and CMake defaults to `RelWithDebInfo`; `--bench-chrome` fails the
gate if optimisation is off. See [`docs/architecture.md`](docs/architecture.md#paint-budget).

CMake prefers `pkg-config mpv`. If that is missing and
`third_party/libmpv/linux/x86_64/libmpv.so` exists, it links the bundle plus
SONAME stubs under `src/mpv_stubs/`. Without either, the binary still runs with
a silent `NullEngine`.

X11:

```bash
QT_QPA_PLATFORM=xcb ./build/aoide
```

Dump 1× logical chrome (no windows):

```bash
QT_QPA_PLATFORM=offscreen ./build/aoide --dump-chrome /tmp/aoide-chrome
```

Let the binary see `WAYLAND_DISPLAY` unless you opt into xcb. Drag the title
strip (not the window buttons) to move a window. Closing **Aoide** (main) quits.

## Packaging

Listener installers are built by GitHub Actions. See [`docs/distribution.md`](docs/distribution.md).

A version tag `v*` (matching [`VERSION`](VERSION)) runs the Release workflow and
attaches artifacts to a GitHub Release (a mirror; the product page is aoide.music).

## Known v1 limits

| Limit | Notes |
|-------|--------|
| Linux MPRIS | OS media keys / Now Playing via D-Bus not implemented. In-app media keys work when Aoide is focused. |
| Second-instance “Open with” | Cold-start argv and file associations work; a second running instance does not forward paths to the first. |
| Spectrum | Real 20-bar analyser (offline PCM + STFT). Honest silence until the spectrogram for the current track is ready. |
| macOS host | 1.1. CI builds, tests and smoke-starts the bundle on every run and uploads a notarized DMG. One has been installed on a MacBook and played audio; that is one machine, and nothing automated launches the *installed* app. |

Windows Store and Flathub are 1.0 channels; the macOS DMG is 1.1. Mac App Store and Snap are not.

## v1 success criteria

From [`docs/aoide-v1-spec.md`](docs/aoide-v1-spec.md).

| Criterion | Status |
|-----------|--------|
| Install/run on Win/Linux (build artifacts) | Linux binary is `build/aoide`; Windows CI compiles the host; macOS is 1.1 |
| Open local audio + playlists | Implemented (file picker, DnD, argv, folder expand) |
| Manage playlist (add/remove/reorder/save/restore) | Implemented (`PlaylistController`, M3U/M3U8) |
| Transport chrome controls + tags when present | Implemented (Qt chrome + libmpv) |
| Frameless mockup chrome (zoom-sized main/EQ, resizable playlist) | Implemented (`src/` QPainter chrome) |
| No library/WSZ skins/store dependency | Confirmed (non-goals excluded) |

Automated gate: `ctest` in `build/`.

## File associations

Aoide accepts file paths on the command line. Double-click / “Open with” must
register the OS handler to pass those paths to the executable (`Exec=aoide %F`).

### Linux

`packaging/linux/com.proximamagnifica.aoide.desktop` lists `MimeType=` entries for common
audio types and M3U playlists.

```bash
xdg-mime default com.proximamagnifica.aoide.desktop audio/mpeg
update-desktop-database ~/.local/share/applications
```

### Windows / macOS

Windows file-type registration lives in the Inno script. macOS document types are
declared in `packaging/macos/Info.plist.in` (audio plus M3U/M3U8), unverified in Finder.

## Related docs

- [`CONTEXT.md`](CONTEXT.md) — domain vocabulary
- [`docs/architecture.md`](docs/architecture.md) — structure map
- [`docs/distribution.md`](docs/distribution.md) — CI, artifacts, secrets

## License

Copyright (C) 2026 Proxima Magnifica

Aoide is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

Aoide is distributed in the hope that it is useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

The full license is in [`LICENSE`](LICENSE). Other works shipped with
Aoide are listed in [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

The name Aoide, the maker’s plate, Proxima Magnifica, and aoide.music
are trademarks; the GPL does not grant trademark rights.
