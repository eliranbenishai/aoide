# Development

Aoide is Qt 6 (QWidget + QPainter) in [`src/`](../src/). Product: [`README.md`](../README.md). Scope: [`aoide-v1-spec.md`](aoide-v1-spec.md).

## Prerequisites

Qt is pinned to **6.11.1** in [`QT_VERSION`](../QT_VERSION). A different kit is a hard error in CMake (`VERSION_EQUAL`) and in the Linux scripts ([`tool/qt-env.sh`](../tool/qt-env.sh), [`tool/fetch_qt.sh`](../tool/fetch_qt.sh)).

CMake ≥ 3.16, C++17. Unset `CMAKE_BUILD_TYPE` on a single-config generator defaults to `RelWithDebInfo`. `-G Ninja` is optional; CI does not pass it.

## Build

CMake is the common path. [PR CI](../.github/workflows/ci.yml) runs:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## libmpv

CMake prefers `pkg-config mpv`. If that target is missing it links a staged full libmpv. Those binaries are not committed.

| Host | Staged tree | Populate with |
|------|-------------|----------------|
| Linux | `third_party/libmpv/linux/x86_64/libmpv.so` plus SONAME stubs under `src/mpv_stubs/` | Distro devel package, or [`tool/stage_linux_libmpv.sh`](../tool/stage_linux_libmpv.sh) |
| Windows | `libmpv-2.dll` and `libmpv.dll.a` under `third_party/libmpv/windows/x86_64/` | [`tool/fetch_full_libmpv.ps1`](../tool/fetch_full_libmpv.ps1) |
| macOS | `Mpv.xcframework` under `third_party/libmpv/macos/universal/` | [`tool/fetch_full_libmpv.sh`](../tool/fetch_full_libmpv.sh) |

[`tool/fetch_full_libmpv.sh`](../tool/fetch_full_libmpv.sh) on Linux only prints where to put a `.so`; it does not populate the tree.

Without an engine, configure is a hard error unless `-DAOIDE_ALLOW_NO_AUDIO=ON`. That build uses `MissingAudioEngine` (refuses `open`; playback stays stopped). `NullEngine` is a test stand-in only.

## Paint budget

Chrome is CPU-rasterised every frame. An unoptimised build drags at a few frames per second. `build.sh` compiles `-O2`. `--bench-chrome` exits 1 if optimisation is off (`__OPTIMIZE__` on GCC/Clang; `NDEBUG` on MSVC) and if the millisecond budgets fail. Details: [`architecture.md`](architecture.md#paint-budget).

## CI

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs on every pull request and every push to `main`. Matrix: `os: [ubuntu-24.04, windows-latest, macos-latest]`. Each `Qt (…)` job runs the CMake/`ctest` commands above, `--bench-chrome`, stages the bundle, and smoke-starts that stage after stripping the runner's Qt. Ubuntu also runs [`tool/check-metainfo.sh`](../tool/check-metainfo.sh). A red compile, `ctest`, paint budget, metainfo check, stage or smoke fails that host.

The `main` ruleset requires `Qt (ubuntu-24.04)`, `Qt (windows-latest)`, `Qt (macos-latest)` and `CI passed`. Pull-request CI does not sign, notarize, wrap a DMG or upload an artifact. Depth: [`distribution.md`](distribution.md).

## Linux

Build tools and libmpv headers:

```bash
# Fedora
sudo dnf install cmake gcc-c++ pkgconf-pkg-config mesa-libGL-devel libX11-devel mpv-devel

# Arch
sudo pacman -S cmake gcc mpv

# Ubuntu
sudo apt-get install cmake g++ pkg-config libgl1-mesa-dev libx11-dev libmpv-dev
```

The official kit's xcb/Wayland plugins also need the client libs listed in [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) (`libxcb-*`, `libxkbcommon-x11-0`, `libwayland-*`). A kit that is not 6.11.1 fails configure.

```bash
./tool/fetch_qt.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/aoide
```

`fetch_qt.sh` writes `.local/qt/6.11.1/gcc_64` (`qtbase qtwayland icu`), which CMake auto-detects. Binary: `build/aoide`.

### `build.sh`

[`./build.sh`](../build.sh) is a Linux-only convenience. It is not CMake: it hand-invokes `moc` and `clang++`, compiles `-O2`, writes `build/aoide`, then runs `--bench-chrome` and its own test binaries. CI does not use it.

[`./tool/fetch_qt.sh`](../tool/fetch_qt.sh) exits 1 on any non-Linux host. [`tool/qt-env.sh`](../tool/qt-env.sh) only looks for `libQt6Core.so` under `.local/qt/<pin>/gcc_64`, so a macOS kit is invisible to it even when `QT=` is set.

It defaults `CXX`/`CC` to Linuxbrew LLVM (`/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++`) and links `third_party/libmpv/linux/x86_64`. Override `CXX`/`CC` if that toolchain is absent, and stage a full `.so` first (`./tool/stage_linux_libmpv.sh`).

## macOS

[`fetch_qt.sh`](../tool/fetch_qt.sh) exits 1 here. Needs Xcode 15+ (macOS 14 SDK or higher), CMake, `python3`, and `curl`. Deployment target is **13.0**; CMake refuses anything lower. Default architectures are `x86_64;arm64`.

The Qt kit must be the official desktop `clang_64` build of 6.11.1 (universal for Qt 6.5+). Homebrew Qt will not work: the pin is exact, and a universal link against a thin kit or libmpv fails. `qttools` is required for `macdeployqt`.

```bash
aqt install-qt mac desktop 6.11.1 clang_64 --outputdir .local/qt --archives qtbase qttools
./tool/fetch_full_libmpv.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
open build/Aoide.app
```

That `aqt` line unpacks to `.local/qt/6.11.1/macos`, which CMake auto-detects (`lib/cmake/Qt6/Qt6Config.cmake`). Otherwise pass `-DCMAKE_PREFIX_PATH`. The product is `build/Aoide.app` (`OUTPUT_NAME Aoide`), or `build/Release/Aoide.app` on a multi-config generator — not `build/aoide`. If the kit or libmpv is single-arch, set `CMAKE_OSX_ARCHITECTURES` to match.

CMakeLists also accepts `pkg-config mpv` (its error text mentions `brew install mpv`); that will not satisfy a default universal configure against a thin Homebrew libmpv. `./tool/fetch_full_libmpv.sh` is what CI runs.

## Windows

[`fetch_qt.sh`](../tool/fetch_qt.sh) exits 1 here. CMakeLists does not search `.local/qt/` on Windows — pass `-DCMAKE_PREFIX_PATH` at the official 6.11.1 desktop kit unless the environment already exports it (`install-qt-action` does).

Needs Visual Studio C++ tools and CMake. Put the kit's `bin` on `PATH` so the build-tree exe can load Qt. `windeployqt` lives in `qtbase`. Official aqtinstall 3.3.0 cannot locate Qt 6.11.1 on Windows (repository layout change); CI pins a newer aqtinstall commit — see [`.github/workflows/ci.yml`](../.github/workflows/ci.yml).

```powershell
powershell -ExecutionPolicy Bypass -File tool/fetch_full_libmpv.ps1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The exe is `build/Release/aoide.exe` on a multi-config generator (Visual Studio) or `build/aoide.exe` on Ninja/Makefiles.

## Useful commands

Force xcb (Linux). Let the binary see `WAYLAND_DISPLAY` unless you opt into this:

```bash
QT_QPA_PLATFORM=xcb ./build/aoide
```

Dump 1× logical chrome to PNGs (no windows). Works on any host with the offscreen QPA plugin:

```bash
QT_QPA_PLATFORM=offscreen ./build/aoide --dump-chrome /tmp/aoide-chrome
```

Drag the title strip (not the window buttons) to move a panel. Closing **Aoide** (main) quits.

## Packaging

Listener installers are built by GitHub Actions. See [`distribution.md`](distribution.md).

A version tag `v*` (matching [`VERSION`](../VERSION), currently **1.2**) runs the Release workflow and attaches artifacts to a GitHub Release (a mirror; the product page is aoide.music).

## Known v1 limits

| Limit | Notes |
|-------|--------|
| Linux MPRIS | OS media keys / Now Playing via D-Bus not implemented. In-app media keys work when Aoide is focused. |
| Second-instance “Open with” | Cold-start argv and file associations work; a second running instance does not forward paths to the first. |
| Spectrum | Real 20-bar analyser (offline PCM + STFT). Honest silence until the spectrogram for the current track is ready. |
| macOS host | Channel opened in 1.1. Pull-request CI builds, tests and smoke-starts the staged bundle offscreen on every run. A notarized DMG is produced by release CI, not uploaded from pull-request CI. One has been installed on a MacBook and played audio; that is one machine, and nothing automated launches the *installed* app or checks Gatekeeper. |

Windows Store and Flathub are 1.0 channels; the macOS DMG is a 1.1 channel. Mac App Store and Snap are not.

## v1 success criteria

From [`aoide-v1-spec.md`](aoide-v1-spec.md).

| Criterion | Status |
|-----------|--------|
| Install/run on Win/Linux (build artifacts) | Linux binary is `build/aoide`; Windows `aoide.exe`; macOS `Aoide.app` since 1.1 |
| Open local audio + playlists | Implemented (file picker, DnD, argv, folder expand) |
| Manage playlist (add/remove/reorder/save/restore) | Implemented (`PlaylistController`, M3U/M3U8) |
| Transport chrome controls + tags when present | Implemented (Qt chrome + libmpv) |
| Frameless mockup chrome (zoom-sized main/EQ, resizable playlist) | Implemented (`src/` QPainter chrome) |
| No library/WSZ skins/store dependency | Confirmed (non-goals excluded) |

Automated gate: `ctest` in `build/`.

## File associations

Aoide accepts file paths on the command line. Double-click / “Open with” must register the OS handler to pass those paths to the executable (`Exec=aoide %F`).

### Linux

[`packaging/linux/com.proximamagnifica.aoide.desktop`](../packaging/linux/com.proximamagnifica.aoide.desktop) lists `MimeType=` entries for common audio types and M3U playlists.

```bash
xdg-mime default com.proximamagnifica.aoide.desktop audio/mpeg
update-desktop-database ~/.local/share/applications
```

### Windows

File-type registration lives in [`packaging/windows/aoide.iss`](../packaging/windows/aoide.iss) (`.mp3` `.m4a` `.aac` `.flac` `.wav` `.ogg` `.opus` `.m3u` `.m3u8`).

### macOS

Document types are declared in [`packaging/macos/Info.plist.in`](../packaging/macos/Info.plist.in) (audio plus M3U/M3U8), unverified in Finder. Same-process Finder / Dock opens arrive as `QFileOpenEvent`; a second running instance still does not forward paths.

## Related docs

- [`CONTEXT.md`](../CONTEXT.md) — domain vocabulary
- [`architecture.md`](architecture.md) — structure map
- [`distribution.md`](distribution.md) — CI, artifacts, secrets
- [`aoide-v1-spec.md`](aoide-v1-spec.md) — product scope

Agent-facing notes live under `docs/agents/` and are gitignored, so they are
local to a checkout rather than published.
