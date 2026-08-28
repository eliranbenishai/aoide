# Full libmpv bundle

Aoide ships **full** libmpv (+ FFmpeg filter graph support), not a compressed
“audio-default” / slim build. See [`docs/architecture.md`](../../docs/architecture.md).

Slim builds often embed `--disable-filters`. That removes `aresample` from
libavfilter, so EQ filter graphs silently no-op. Packaging must load the
binaries under this tree (or a distro full libmpv) instead.

## Layout

```text
third_party/libmpv/
  pins.json                 # pinned URLs + hashes (committed)
  windows/x86_64/libmpv-2.dll   # fetched, not committed by default
  windows/x86_64/libmpv.dll.a   # import library, same fetch
  linux/x86_64/libmpv.so*       # optional bundle override
  macos/universal/              # audio-full xcframeworks (fetched, flattened)
    Mpv.xcframework/macos-arm64_x86_64/Mpv.framework
    Avcodec.xcframework/ … Avfilter Avformat Avutil
    Swresample.xcframework/ Swscale.xcframework/
    Mbedcrypto.xcframework/ Mbedtls.xcframework/ Mbedx509.xcframework/
  .cache/                       # download/extract scratch (gitignored)
```

The media-kit archive wraps those xcframeworks in a versioned directory.
`tool/fetch_full_libmpv.sh` lifts that wrapper (and verifies `archiveSha256`)
so CMake can look at `macos/universal/Mpv.xcframework` without baking the pin
name into the build. Each framework's install name is
`@rpath/<Name>.framework/Versions/A/<Name>`; the app's rpath is
`@executable_path/../Frameworks`.

## Fetch (Windows)

From the repo / worktree root:

```powershell
powershell -ExecutionPolicy Bypass -File tool/fetch_full_libmpv.ps1
```

This downloads the pinned shinchiro `mpv-dev` archive, verifies SHA-256, extracts
`libmpv-2.dll` **and its import library `libmpv.dll.a`** into `windows/x86_64/`,
and checks the DLL hash.

Both files are needed and neither is optional: the import library is what the
Windows build links against, and the DLL is what the loader resolves at run time
and what ships. `CMakeLists.txt` only turns `AOIDE_HAVE_MPV` on for Windows when
it finds both.

Requires Visual Studio’s CMake (`cmake -E tar`) on PATH, or the VS 2022
Community CMake path used by the script.

## Fetch (macOS / Linux)

```bash
./tool/fetch_full_libmpv.sh
```

- **macOS:** downloads `audio-full` xcframeworks into `macos/universal/` (see
  `pins.json`), verifies the archive hash, and refuses a slim `--disable-filters`
  build the same way the Windows fetcher does.
- **Linux:** documents system package expectation; optionally copies a local
  full `libmpv.so*` if you place it under `linux/x86_64/`. Prefer
  `./tool/stage_linux_libmpv.sh` to copy the distro full libmpv into that dir.

## Packaging hooks

| Platform | Hook |
|----------|------|
| Windows | Root `CMakeLists.txt` links `libmpv.dll.a` and copies `libmpv-2.dll` beside the binaries it builds and into the install prefix; `packaging/windows/stage.ps1` copies it next to `aoide.exe` when present. |
| Linux | Root `CMakeLists.txt` install stages `third_party/libmpv/linux/x86_64/libmpv.so*` into the bundle `lib/` when present (else system libmpv). |
| macOS | Root `CMakeLists.txt` links `Mpv.framework` from the staged xcframework and copies the whole `@rpath` graph into `Aoide.app/Contents/Frameworks`. `packaging/macos/stage_app.sh` runs `macdeployqt` and fails if libmpv is still missing. Do not ship slim `audio-default`. |

## Do not commit slim libs

Never vendor a slim ~15 MB media_kit-style audio DLL. Prefer the fetch script + pins.
Git LFS is optional if you later choose to commit the full DLL (~114 MB).
