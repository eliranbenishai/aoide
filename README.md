# Tramp

Multi-platform desktop music player. See [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md)
for product scope.

The running app is the **Qt 6 host** in [`qt/`](qt/). The Dart/Flutter tree is a
frozen chrome and domain reference (goldens, vocabulary).

## Development

```bash
./qt/build.sh
./qt/build/tramp
```

Or with system CMake (see [`qt/README.md`](qt/README.md)):

```bash
cmake -S qt -B qt/build -G Ninja
cmake --build qt/build
ctest --test-dir qt/build --output-on-failure
./qt/build/tramp
```

Dump 1× logical chrome (no windows):

```bash
QT_QPA_PLATFORM=offscreen ./qt/build/tramp --dump-chrome /tmp/tramp-chrome
```

Flutter **3.47** / Dart **3.13** remains the pin for the frozen reference tests:

```bash
flutter pub get
flutter test
```

## Packaging

Listener installers are built by GitHub Actions. See [`docs/distribution.md`](docs/distribution.md).
Linux/Windows product packaging still needs the Qt host wired into those
workflows; the Flutter `flutter build …` bundles are leftover until that cutover.

A version tag `v*` (matching `pubspec.yaml`) runs the Release workflow and
attaches artifacts to a GitHub Release (a mirror; the product page is tramp.music).

## Known v1 limits

| Limit | Notes |
|-------|--------|
| Linux MPRIS | OS media keys / Now Playing via D-Bus not implemented. In-app media keys work when Tramp is focused. |
| Second-instance “Open with” | Cold-start argv and file associations work; a second running instance does not forward paths to the first. |
| Spectrum | Honest silence until a Qt analyser exists (bars stay at 0 in live play). |
| Skin packs | Builtin mockup tokens. Recolor packs remain in the Dart reference tree. |
| macOS host | Qt pairing comes after Linux + Windows. |

Windows Store and Flathub are v1 channels; Mac App Store and Snap are not.

## v1 success criteria

From [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md).

| Criterion | Status |
|-----------|--------|
| Install/run on Win/Linux/macOS (build artifacts) | Linux Qt host is the product binary (`qt/build/tramp`); Windows CI compiles the host; macOS later |
| Open local audio + playlists | Implemented (file picker, DnD, argv, folder expand) |
| Manage playlist (add/remove/reorder/save/restore) | Implemented (`PlaylistController`, M3U/M3U8) |
| Transport chrome controls + tags when present | Implemented (Qt chrome + libmpv) |
| Frameless mockup chrome (zoom-sized main/EQ, resizable playlist) | Implemented (`qt/src/` QPainter chrome) |
| No library/WSZ skins/store dependency | Confirmed (non-goals excluded) |

Automated gate: `ctest` in `qt/build` (and `flutter test` for the frozen reference).

## File associations

Tramp accepts file paths on the command line. Double-click / “Open with” must
register the OS handler to pass those paths to the executable (`Exec=tramp %F`).

### Linux

`linux/com.tramp.tramp.desktop` lists `MimeType=` entries for common audio types
and M3U playlists.

```bash
xdg-mime default com.tramp.tramp.desktop audio/mpeg
update-desktop-database ~/.local/share/applications
```

### Windows / macOS

Windows file-type registration and `Info.plist` document types still describe
the Flutter packaging path; they need a Qt-host pass when those installers
switch.

## Related docs

- [`CONTEXT.md`](CONTEXT.md) — domain vocabulary
- [`docs/architecture.md`](docs/architecture.md) — structure map
- [`docs/distribution.md`](docs/distribution.md) — CI, artifacts, secrets
- [`qt/README.md`](qt/README.md) — Qt host build

## License

Copyright (C) 2026 Proxima Magnifica

Tramp is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

Tramp is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

The full license is in [`LICENSE`](LICENSE). Other works shipped with
Tramp are listed in [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

The name Tramp, the maker’s plate, Proxima Magnifica, and tramp.music
are trademarks; the GPL does not grant trademark rights.
