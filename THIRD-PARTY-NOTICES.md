# Third-party notices

Tramp is **GPL-3.0-or-later**. This file lists other works shipped in the source tree or typical binaries. It is not legal advice.

## Fonts (SIL Open Font License 1.1)

`TrampCondensed` and `TrampMono` are modified versions of **Barlow** (The Barlow Project Authors) and **IBM Plex** (IBM Corp., reserved name “Plex”). The font software remains under the OFL; the full text is [`assets/fonts/OFL.txt`](assets/fonts/OFL.txt). Modified versions must not use the reserved names Barlow or Plex.

## Flutter and Dart packages

The UI is built with [Flutter](https://flutter.dev/) (BSD-style license, Copyright Flutter Authors / Google). Direct Dart dependencies and their typical licenses:

| Package | License (upstream) |
|---------|-------------------|
| `window_manager` (git pin) | MIT |
| `desktop_multi_window` (vendored fork) | Apache-2.0 — [`third_party/desktop_multi_window/LICENSE`](third_party/desktop_multi_window/LICENSE) |
| `media_kit` / `media_kit_libs_audio` | MIT |
| `file_picker` | MIT |
| `desktop_drop` | MIT |
| `path_provider` | BSD-3-Clause |
| `path` | BSD-3-Clause |
| `archive` | Apache-2.0 |
| `smtc_windows` | MIT |
| `flutter_svg` | MIT |
| `ffi` | BSD-3-Clause |
| `flutter_rust_bridge` (override) | MIT |

Each package’s own `LICENSE` in the pub cache / GitHub repo is authoritative. Transitive dependencies are not listed here.

## Playback engine (binaries)

Release builds bundle **libmpv** and **FFmpeg** (full, filters enabled — [ADR 0005](docs/adr/0005-full-libmpv.md)). Those libraries are copyleft (GPL family, including the pinned Windows shinchiro `mpv-dev` build). Their source is upstream mpv/FFmpeg; pins live in [`third_party/libmpv/`](third_party/libmpv/).

## Trademarks

The GPL and these notices do **not** grant rights in the name **Tramp**, the maker’s plate, **Proxima Magnifica**, or `tramp.music`.
