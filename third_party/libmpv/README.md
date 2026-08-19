# Full libmpv bundle

Tramp ships **full** libmpv (+ FFmpeg filter graph support), not a compressed
“audio-default” / slim build. See [`docs/architecture.md`](../../docs/architecture.md).

Slim builds often embed `--disable-filters`. That removes `aresample` from
libavfilter, so EQ filter graphs silently no-op. Packaging must load the
binaries under this tree (or a distro full libmpv) instead.

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

- **macOS:** downloads `audio-full` xcframeworks into `macos/universal/` (see
  `pins.json`). Used when the Qt Mac host exists.
- **Linux:** documents system package expectation; optionally copies a local
  full `libmpv.so*` if you place it under `linux/x86_64/`. Prefer
  `./tool/stage_linux_libmpv.sh` to copy the distro full libmpv into that dir.

## Packaging hooks

| Platform | Hook |
|----------|------|
| Windows | `packaging/windows/stage.ps1` copies `libmpv-2.dll` next to `tramp.exe` when present. |
| Linux | Root `CMakeLists.txt` install stages `third_party/libmpv/linux/x86_64/libmpv.so*` into the bundle `lib/` when present (else system libmpv). |
| macOS | After `fetch_full_libmpv.sh`, the Qt Mac host will load the staged frameworks. Do not ship slim `audio-default`. |

## Do not commit slim libs

Never vendor a slim ~15 MB media_kit-style audio DLL. Prefer the fetch script + pins.
Git LFS is optional if you later choose to commit the full DLL (~114 MB).
