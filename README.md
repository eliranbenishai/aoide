# Aoide

Multi-platform desktop music player. See [`docs/aoide-v1-spec.md`](docs/aoide-v1-spec.md)
for product scope.

The app is **Qt 6** (QWidget + QPainter) in [`src/`](src/).

## Development

Qt is pinned to **6.10.3** in [`QT_VERSION`](QT_VERSION). A different kit is a
hard error in CMake and in the Linux scripts.

CMake is the common path. [PR CI](.github/workflows/ci.yml) runs

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

on every pull request and every push to `main`. Ubuntu and Windows share the
`os: [ubuntu-24.04, windows-latest]` matrix. macOS is a separate required job
(`Qt (macos-latest)`, `timeout-minutes: 90`), not a third matrix leg: a
packaging job would repeat the universal `x86_64;arm64` build, and a DMG
produced in the same job as the tests cannot be uploaded for a commit whose
tests failed. A compile, `ctest` or `--bench-chrome` failure on any host
blocks a merge. Only the macOS packaging steps (stage, smoke, sign/notarize,
upload) are `continue-on-error`, so Apple's notary cannot. The `main` ruleset
requires `Qt (ubuntu-24.04)`, `Qt (windows-latest)`, `Qt (macos-latest)` and
`CI passed`. CMake ≥ 3.16, C++17. Unset `CMAKE_BUILD_TYPE` defaults to
`RelWithDebInfo`. `-G Ninja` is optional; CI does not pass it. CI depth:
[`docs/distribution.md`](docs/distribution.md).

`./build.sh` is a Linux-only convenience: it is not CMake, and it hand-invokes
`moc` and `clang++`. [`./tool/fetch_qt.sh`](tool/fetch_qt.sh) exits 1 on any
non-Linux host. [`tool/qt-env.sh`](tool/qt-env.sh) only looks for
`libQt6Core.so` under `.local/qt/<pin>/gcc_64`, so a macOS kit is invisible
to it even when `QT=` is set.

The chrome is rasterised on the CPU every frame, so an unoptimised build does not
just benchmark badly — panels drag at a few frames per second. `build.sh`
compiles `-O2`; `--bench-chrome` fails the gate if optimisation is off. See
[`docs/architecture.md`](docs/architecture.md#paint-budget).

CMake prefers `pkg-config mpv`. If that is missing it links a staged full
libmpv: `third_party/libmpv/linux/x86_64/libmpv.so` plus SONAME stubs under
`src/mpv_stubs/` on Linux; `libmpv-2.dll` and `libmpv.dll.a` from
[`tool/fetch_full_libmpv.ps1`](tool/fetch_full_libmpv.ps1) on Windows; or
`Mpv.xcframework` from [`tool/fetch_full_libmpv.sh`](tool/fetch_full_libmpv.sh)
on macOS. Those binaries are not committed. Without an engine, configure is a
hard error unless `-DAOIDE_ALLOW_NO_AUDIO=ON` (a deliberately silent
`NullEngine`). CI and packaging: [`docs/distribution.md`](docs/distribution.md).

### Linux

Build tools and libmpv headers:

```bash
# Fedora
sudo dnf install libX11-devel cmake gcc-c++ mpv-libs-devel

# Arch
sudo pacman -S cmake gcc mpv

# Ubuntu
sudo apt-get install cmake g++ pkg-config libgl1-mesa-dev libx11-dev libmpv-dev
```

The official kit's xcb/Wayland plugins also need the client libs listed in
[`.github/workflows/ci.yml`](.github/workflows/ci.yml). A kit that is not
6.10.3 fails configure.

```bash
./tool/fetch_qt.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/aoide
```

`fetch_qt.sh` writes `.local/qt/6.10.3/gcc_64`, which CMake auto-detects.

`./build.sh` fetches that same kit if missing and writes `build/aoide`. It
defaults `CXX`/`CC` to Linuxbrew LLVM
(`/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++`) and links
`third_party/libmpv/linux/x86_64` — override `CXX`/`CC` if that toolchain is
absent, and stage a full `.so` there first (`./tool/stage_linux_libmpv.sh`).

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

### macOS

Required [PR CI](.github/workflows/ci.yml) job, not a third matrix leg (see
above). `fetch_qt.sh` exits 1 here. The product channel is **1.1**: CI builds,
tests and smoke-starts the bundle on every run and uploads a notarized DMG;
one has been installed on a MacBook and played audio; that is one machine,
and nothing automated launches the *installed* app. See
[Known v1 limits](#known-v1-limits).

Needs Xcode 15+ / the macOS 13 SDK, CMake, `python3`, and `curl`. The Qt kit
must be the official desktop `clang_64` build of 6.10.3 (universal for Qt 6.5+).
Homebrew Qt will not work: it is the wrong version and a thin arch, and CMake
defaults to `CMAKE_OSX_DEPLOYMENT_TARGET=13.0` and
`CMAKE_OSX_ARCHITECTURES=x86_64;arm64`. A universal link against a thin kit or
libmpv fails. `qttools` is required for `macdeployqt`.

```bash
aqt install-qt mac desktop 6.10.3 clang_64 --outputdir .local/qt --archives qtbase qttools
./tool/fetch_full_libmpv.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
open build/Aoide.app
```

That `aqt` line unpacks to `.local/qt/6.10.3/macos`, which CMake auto-detects
(`lib/cmake/Qt6/Qt6Config.cmake`). Otherwise pass `-DCMAKE_PREFIX_PATH`.
The product is `build/Aoide.app` (`OUTPUT_NAME Aoide`), not `build/aoide`.
If the kit or libmpv is single-arch, set `CMAKE_OSX_ARCHITECTURES` to match.

CMakeLists also accepts `pkg-config mpv` (its error text mentions
`brew install mpv`); that will not satisfy a default universal configure
against a thin Homebrew libmpv. `./tool/fetch_full_libmpv.sh` is what CI runs.

CI: `export-qt-pin.sh` → `jurplel/install-qt-action@v4` (`host: mac`,
`target: desktop`, `arch: clang_64`, `archives: qtbase qttools`) →
`./tool/fetch_full_libmpv.sh` → the CMake/`ctest` lines above →
`--bench-chrome` on `build/Aoide.app/Contents/MacOS/Aoide` (falling back to
`build/Release/Aoide.app/...`) → `packaging/macos/stage_app.sh` → smoke →
sign/notarize → upload-artifact `macos-dmg`. The smoke unsets
`QT_PLUGIN_PATH`, `DYLD_FRAMEWORK_PATH` and `DYLD_LIBRARY_PATH` (which
`install-qt-action` exports) and sets `QT_QPA_PLATFORM=offscreen`, then runs
the staged app `--bench-chrome` and with `AOIDE_AUTO_QUIT=1`, proving the
bundle carries its own Qt and libmpv. Sign/notarize and upload are gated on
`steps.smoke.outcome == 'success'`.

### Windows

PR CI installs official Qt 6.10.3 (`archives: qtbase`; `windeployqt` lives
there) and runs [`tool/fetch_full_libmpv.ps1`](tool/fetch_full_libmpv.ps1).
`fetch_qt.sh` exits 1 here. CMakeLists does not search `.local/qt/` on
Windows — pass `-DCMAKE_PREFIX_PATH` at the official 6.10.3 desktop kit
unless the environment already exports it (`install-qt-action` does).

Needs Visual Studio C++ tools and CMake. Put the kit's `bin` on `PATH` so
the build-tree exe can load Qt.

```powershell
powershell -ExecutionPolicy Bypass -File tool/fetch_full_libmpv.ps1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The exe is `build/Release/aoide.exe` on a multi-config generator (Visual
Studio) or `build/aoide.exe` on Ninja/Makefiles — same split
[PR CI](.github/workflows/ci.yml) uses.

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
| macOS host | 1.1. CI builds, tests and smoke-starts the staged bundle offscreen on every run and uploads a notarized DMG. One has been installed on a MacBook and played audio; that is one machine, and nothing automated launches the *installed* app or checks Gatekeeper. |

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
