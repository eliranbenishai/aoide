# Tramp

Multi-platform desktop music player. See [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md)
for product scope.

The app is **Qt 6** (QWidget + QPainter) in [`src/`](src/).

## Development

Homebrew Qt / no CMake on `PATH`:

```bash
./build.sh
./build/tramp
```

System Qt + CMake:

```bash
# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtwayland libX11-devel cmake ninja-build gcc-c++ mpv-libs-devel

# Arch
sudo pacman -S qt6-base qt6-wayland cmake ninja gcc mpv
```

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/tramp
```

CMake prefers `pkg-config mpv`. If that is missing and
`third_party/libmpv/linux/x86_64/libmpv.so` exists, it links the bundle plus
SONAME stubs under `src/mpv_stubs/`. Without either, the binary still runs with
a silent `NullEngine`.

X11:

```bash
QT_QPA_PLATFORM=xcb ./build/tramp
```

Dump 1× logical chrome (no windows):

```bash
QT_QPA_PLATFORM=offscreen ./build/tramp --dump-chrome /tmp/tramp-chrome
```

Let the binary see `WAYLAND_DISPLAY` unless you opt into xcb. Drag the title
strip (not the window buttons) to move a window. Closing **Tramp** (main) quits.

## Packaging

Listener installers are built by GitHub Actions. See [`docs/distribution.md`](docs/distribution.md).

A version tag `v*` (matching [`VERSION`](VERSION)) runs the Release workflow and
attaches artifacts to a GitHub Release (a mirror; the product page is tramp.music).

## Known v1 limits

| Limit | Notes |
|-------|--------|
| Linux MPRIS | OS media keys / Now Playing via D-Bus not implemented. In-app media keys work when Tramp is focused. |
| Second-instance “Open with” | Cold-start argv and file associations work; a second running instance does not forward paths to the first. |
| Spectrum | Real 20-bar analyser (offline PCM + STFT). Honest silence until the spectrogram for the current track is ready. |
| macOS host | Qt pairing comes after Linux + Windows. |

Windows Store and Flathub are v1 channels; Mac App Store and Snap are not.

## v1 success criteria

From [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md).

| Criterion | Status |
|-----------|--------|
| Install/run on Win/Linux/macOS (build artifacts) | Linux binary is `build/tramp`; Windows CI compiles the host; macOS later |
| Open local audio + playlists | Implemented (file picker, DnD, argv, folder expand) |
| Manage playlist (add/remove/reorder/save/restore) | Implemented (`PlaylistController`, M3U/M3U8) |
| Transport chrome controls + tags when present | Implemented (Qt chrome + libmpv) |
| Frameless mockup chrome (zoom-sized main/EQ, resizable playlist) | Implemented (`src/` QPainter chrome) |
| No library/WSZ skins/store dependency | Confirmed (non-goals excluded) |

Automated gate: `ctest` in `build/`.

## File associations

Tramp accepts file paths on the command line. Double-click / “Open with” must
register the OS handler to pass those paths to the executable (`Exec=tramp %F`).

### Linux

`packaging/linux/com.proximamagnifica.tramp.desktop` lists `MimeType=` entries for common
audio types and M3U playlists.

```bash
xdg-mime default com.proximamagnifica.tramp.desktop audio/mpeg
update-desktop-database ~/.local/share/applications
```

### Windows / macOS

Windows file-type registration lives in the Inno script. macOS document types
wait on the Qt Mac host.

## Related docs

- [`CONTEXT.md`](CONTEXT.md) — domain vocabulary
- [`docs/architecture.md`](docs/architecture.md) — structure map
- [`docs/distribution.md`](docs/distribution.md) — CI, artifacts, secrets

## License

Copyright (C) 2026 Proxima Magnifica

Tramp is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

Tramp is distributed in the hope that it is useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

The full license is in [`LICENSE`](LICENSE). Other works shipped with
Tramp are listed in [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

The name Tramp, the maker’s plate, Proxima Magnifica, and tramp.music
are trademarks; the GPL does not grant trademark rights.
