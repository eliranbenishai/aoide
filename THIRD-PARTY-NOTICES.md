# Third-party notices

Tramp is **GPL-3.0-or-later**. This file lists other works shipped in the source tree or typical binaries. It is not legal advice.

## Fonts (SIL Open Font License 1.1)

`TrampCondensed` and `TrampMono` are modified versions of **Barlow** (The Barlow Project Authors) and **IBM Plex** (IBM Corp., reserved name “Plex”). **Anton** (The Anton Project Authors) is the skin-immune TRAMP wordmark face. The font software remains under the OFL; the full text is [`assets/fonts/OFL.txt`](assets/fonts/OFL.txt). Modified versions must not use the reserved names Barlow or Plex.

## Qt

The UI is built with [Qt](https://www.qt.io/) 6 (LGPL-3.0 / GPL-3.0 / commercial, Copyright The Qt Company and Qt contributors). Tramp links Qt Widgets / Gui / Core / DBus.

**Every download bundles Qt except the Flatpak.** Windows installers stage it next to `tramp.exe` via `windeployqt`; the Linux AppImage and tarball stage it into `lib/` and `plugins/` via [`packaging/linux/stage_bundle.sh`](packaging/linux/stage_bundle.sh). The Flatpak carries no Qt at all — it uses the `org.kde.Platform` runtime's, which the user can replace independently of Tramp.

Tramp uses Qt under the **LGPL-3.0**, and the bundles are built to keep that workable:

- Qt is **unmodified** upstream or distro Qt, taken as built. Tramp patches nothing in it.
- Qt ships as **separate shared libraries**, dynamically linked — never statically linked and never relinked into the executable. Replacing `lib/libQt6*.so.6` in an extracted bundle with your own build of the same Qt version is enough to run Tramp against a modified Qt, which is the LGPL right the bundling has to preserve.
- The bundle also carries the transitive dependency closure of Qt and libmpv, minus the loader, the C/C++ runtimes and the graphics stack, which stay the host's. Each of those libraries keeps its own upstream licence.

## Playback engine (binaries)

Release builds bundle **libmpv** and **FFmpeg** (full, filters enabled). Those libraries are copyleft (GPL family, including the pinned Windows shinchiro `mpv-dev` build). Their source is upstream mpv/FFmpeg; pins live in [`third_party/libmpv/`](third_party/libmpv/).

On Linux those come from the build machine's distro packages, so a bundle also carries their dependencies — codec, network and system-integration libraries under their own terms (LGPL, BSD, MIT and others). `lib/` in the AppImage or tarball is the authoritative list for a given build.

## Trademarks

The GPL and these notices do **not** grant rights in the name **Tramp**, the maker’s plate, **Proxima Magnifica**, or `tramp.music`.
