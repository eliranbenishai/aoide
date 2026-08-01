# Tramp

Multi-platform desktop music player (Flutter). See [`docs/tramp-v1-spec.md`](docs/tramp-v1-spec.md) for product scope.

## Development

```bash
flutter pub get
flutter run -d windows   # or macos / linux
flutter test
```

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

The Flutter Windows runner forwards argv to Dart automatically. A second running instance starts a new process with the file path; single-instance forwarding is not implemented in v1.

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
