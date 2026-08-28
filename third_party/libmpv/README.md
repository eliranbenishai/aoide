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
  include/mpv/*.h               # vendored client API headers (committed)
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
  build. It checks the whole staged framework set, not `Mpv` alone: here FFmpeg
  is separate xcframeworks, so `aresample` lives only in `Avfilter` and the
  configure line only in `Avutil`/`Avcodec`/`Avfilter`. Windows can interrogate
  `libmpv-2.dll` by itself only because FFmpeg is linked into that one file.
- **Linux:** documents system package expectation; optionally copies a local
  full `libmpv.so*` if you place it under `linux/x86_64/`. Prefer
  `./tool/stage_linux_libmpv.sh` to copy the distro full libmpv into that dir.

## Vendored headers are pinned to client API 2.1 — do not raise them

`include/mpv/` is committed and is the only include path the build uses on all
three platforms; the macOS xcframeworks ship no usable headers for us, and none
of the pinned binaries is a source of truth for what the *other* two can run.

Those headers are upstream mpv 0.36.0, declaring `MPV_CLIENT_API_VERSION` **2.1**.
That is deliberately the **lowest** API any pinned platform provides, not the
highest:

| Platform | Pinned binary | Client API |
|----------|---------------|-----------|
| macOS | media-kit `v0.7.2` (mpv 0.36.0) | **2.1** |
| Linux | distro / staged `libmpv.so.2.5.0` | 2.5 |
| Windows | shinchiro `mpv-dev` | 2.5 |

macOS is the floor and cannot currently be raised: `media-kit/libmpv-darwin-build`
has never published past mpv 0.36.0, so every release in that project — including
the newest — is API 2.1. Pinning the headers to that floor means calling anything
mpv 0.36 lacks is a **compile error on every platform**, caught on Linux by the
developer who wrote it. Raising them to 2.5 to match Linux moves that failure to
run time on macOS, the platform with the least CI coverage, where an unknown
option passed to `mpv_set_option_string` is a silent no-op rather than an error.

The whole 2.1→2.5 delta is one function, `mpv_get_time_ns`, which nothing calls.

Raise these headers only together with a macOS libmpv that actually provides the
higher API — which today means building libmpv from source, not changing a pin.

## Packaging hooks

| Platform | Hook |
|----------|------|
| Windows | Root `CMakeLists.txt` links `libmpv.dll.a` and copies `libmpv-2.dll` beside the binaries it builds and into the install prefix; `packaging/windows/stage.ps1` copies it next to `aoide.exe` when present. |
| Linux | Root `CMakeLists.txt` install stages `third_party/libmpv/linux/x86_64/libmpv.so*` into the bundle `lib/` when present (else system libmpv). |
| macOS | Root `CMakeLists.txt` links `Mpv.framework` from the staged xcframework and copies the whole `@rpath` graph into `Aoide.app/Contents/Frameworks`. `packaging/macos/stage_app.sh` runs `macdeployqt` and fails if libmpv is still missing. Do not ship slim `audio-default`. |

## Do not commit slim libs

Never vendor a slim ~15 MB media_kit-style audio DLL. Prefer the fetch script + pins.
Git LFS is optional if you later choose to commit the full DLL (~114 MB).
