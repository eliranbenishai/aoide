# Third-party notices

Tramp is **GPL-3.0-or-later**. This file lists other works shipped in the source tree or typical binaries. It is not legal advice.

## Fonts (SIL Open Font License 1.1)

`TrampCondensed` and `TrampMono` are modified versions of **Barlow** (The Barlow Project Authors) and **IBM Plex** (IBM Corp., reserved name “Plex”). The font software remains under the OFL; the full text is [`assets/fonts/OFL.txt`](assets/fonts/OFL.txt). Modified versions must not use the reserved names Barlow or Plex.

## Qt

The UI is built with [Qt](https://www.qt.io/) 6 (LGPL-3.0 / GPL-3.0 / commercial, Copyright The Qt Company and Qt contributors). Tramp links Qt Widgets / Gui / Core. Typical Linux builds use the distro Qt; Windows installers stage Qt libraries next to `tramp.exe` via `windeployqt`.

## Playback engine (binaries)

Release builds bundle **libmpv** and **FFmpeg** (full, filters enabled). Those libraries are copyleft (GPL family, including the pinned Windows shinchiro `mpv-dev` build). Their source is upstream mpv/FFmpeg; pins live in [`third_party/libmpv/`](third_party/libmpv/).

## Trademarks

The GPL and these notices do **not** grant rights in the name **Tramp**, the maker’s plate, **Proxima Magnifica**, or `tramp.music`.
