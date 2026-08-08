# Full libmpv bundle

Tramp ships **full** libmpv (+ FFmpeg filter graph support), not the compressed
`media_kit_libs_*` “audio-default” / slim binaries. See
[`docs/adr/0005-full-libmpv.md`](../../docs/adr/0005-full-libmpv.md).

Slim media_kit Windows builds embed `--disable-filters`. That removes
`aresample` from libavfilter, so EQ filter graphs silently no-op. Our packaging
must load the binaries under this tree instead.

## Layout

```text
third_party/libmpv/
  pins.json                 # pinned URLs + hashes (committed)
  windows/x86_64/libmpv-2.dll   # fetched, not committed by default
  linux/x86_64/libmpv.so*       # optional bundle override
  macos/universal/              # full xcframework contents (fetched)
  .cache/                       # download/extract scratch (gitignored)
```

## Fetch (Windows)

From the repo / worktree root:

```powershell
powershell -ExecutionPolicy Bypass -File tool/fetch_full_libmpv.ps1
```

This downloads the pinned shinchiro `mpv-dev` archive, verifies SHA-256, extracts
`libmpv-2.dll` into `windows/x86_64/`, and checks the DLL hash.

Requires Visual Studio’s CMake (`cmake -E tar`) on PATH, or the VS 2022
Community CMake path used by the script.

## Fetch (macOS / Linux)

```bash
./tool/fetch_full_libmpv.sh
```

- **macOS:** downloads media-kit `audio-full` xcframeworks into
  `macos/universal/` (see `pins.json`). After fetch, point the app at these
  frameworks instead of the pod’s slim `audio-default` (see below).
- **Linux:** documents system package expectation; optionally copies a local
  full `libmpv.so*` if you place it under `linux/x86_64/`.

## Packaging hooks

| Platform | Hook |
|----------|------|
| Windows | `windows/CMakeLists.txt` replaces `media_kit_libs_windows_audio`’s bundled `libmpv-2.dll` with `third_party/libmpv/windows/x86_64/libmpv-2.dll` (configure fails if missing). |
| Linux | `linux/CMakeLists.txt` installs `third_party/libmpv/linux/x86_64/libmpv.so*` into the bundle `lib/` when present (else system libmpv). |
| macOS | After `fetch_full_libmpv.sh`, copy/replace `Mpv.xcframework` into the media_kit pod Frameworks **or** add a vendored framework copy step before archive. Do not ship `audio-default`. |

`pubspec.yaml` still depends on `media_kit_libs_audio` for plugin registration;
the native load path is overridden so the process cannot silently use slim DLLs
on Windows.

## Runtime verification

`LibmpvBundle.verify()` resolves the loaded library path and scans for the slim
marker `--disable-filters`. Debug/profile startup fails if a slim binary is
detected.

## Do not commit slim libs

Never vendor the ~15 MB media_kit audio DLL. Prefer the fetch script + pins.
Git LFS is optional if you later choose to commit the full DLL (~114 MB).
