# Tramp

Multi-platform desktop music player (Flutter). See [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md) for product scope.

## Development

Flutter **3.47** / Dart **3.13** (same pin as CI).

```bash
flutter pub get
flutter run -d windows   # or macos / linux
flutter test
```

**Linux file dialogs:** OPEN / ADD / playlist load-save use `file_picker`, which
needs `zenity` or `kdialog` on `PATH`. On a minimal Distrobox/Fedora image:

```bash
sudo dnf install -y zenity
```

## Packaging

Listener installers are built by GitHub Actions. See [`docs/distribution.md`](docs/distribution.md).

```bash
flutter build windows   # → build/windows/x64/runner/Release/tramp.exe
flutter build macos     # → build/macos/Build/Products/Release/tramp.app
flutter build linux     # → build/linux/x64/release/bundle/
```

A version tag `v*` (matching `pubspec.yaml`) runs the Release workflow and attaches artifacts to a GitHub Release (a mirror; the product page is tramp.music).

## Known v1 limits

| Limit | Notes |
|-------|--------|
| Linux MPRIS | OS media keys / Now Playing via D-Bus not implemented (`LinuxOsMediaControls` stub). In-app media keys work when Tramp is focused. |
| Second-instance “Open with” | Cold-start argv and file associations work; a second running instance does not forward paths to the first (starts a new process). |
| macOS/Linux packaging smoke | Release builds not verified on this host; Windows release build verified. Use the Release workflow for installer artifacts. |

Windows Store and Flathub are v1 channels; Mac App Store and Snap are not.

## v1 success criteria

From [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md). Manual smoke on each target OS; automated checks below.

| Criterion | Status |
|-----------|--------|
| Install/run on Win/Linux/macOS (build artifacts) | Windows release build verified; macOS/Linux need host build |
| Open local audio + playlists | Implemented (file picker, DnD, argv, folder expand) |
| Manage playlist (add/remove/reorder/save/restore) | Implemented (`PlaylistController`, M3U/M3U8) |
| Transport chrome controls + tags when present | Implemented (`MainPlayerPanel`, media_kit tags) |
| Frameless graphite-skin chrome UI (zoom-sized main/EQ, resizable playlist) | Implemented (`TrampShell`, `MainPlayerPanel`, PNG skin per ADR 0004, `window_manager`) |
| No library/WSZ skins/store dependency | Confirmed (non-goals excluded) |

Automated gate: `flutter test` and `flutter build windows` must pass.

## File associations

Tramp accepts file paths on the command line (`main` → `parseLaunchArgs`). Double-click / “Open with” must register the OS handler to pass those paths to the executable.

### Windows (dev)

Run from an elevated prompt, adjusting `TRAMP_EXE` to your built binary:

```powershell
$exe = "D:\path\to\build\windows\x64\runner\Debug\tramp.exe"
$exts = @(".mp3",".m4a",".aac",".flac",".wav",".ogg",".opus",".m3u",".m3u8")
foreach ($ext in $exts) {
  $type = "Tramp.File$($ext.TrimStart('.'))"
  cmd /c "assoc $ext=$type"
  cmd /c "ftype $type=`"$exe`" `"%1`""
}
```

The Flutter Windows runner forwards argv to Dart automatically. A second running instance starts a new process with the file path; **single-instance forwarding is not implemented in v1** (see [Known v1 limits](#known-v1-limits)).

### macOS

`macos/Runner/Info.plist` declares `CFBundleDocumentTypes` for v1 audio extensions and M3U/M3U8. Rebuild the app; use **Get Info → Open with** to set Tramp as default.

### Linux

`linux/com.tramp.tramp.desktop` lists `MimeType=` entries for common audio types and M3U playlists. After `flutter build linux`, install the bundle and desktop file:

```bash
xdg-mime default com.tramp.tramp.desktop audio/mpeg
update-desktop-database ~/.local/share/applications
```

File paths are passed via GTK `GApplication` argv (`linux/runner/my_application.cc`).

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

Tramp is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

The full license is in [`LICENSE`](LICENSE). Other works shipped with
Tramp are listed in [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

The name Tramp, the maker’s plate, Proxima Magnifica, and tramp.music
are trademarks; the GPL does not grant trademark rights.
