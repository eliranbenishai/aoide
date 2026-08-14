# 10. Tramp is open-source; the website remains the official download

Date: 2026-08-14

## Status

Accepted

## Context

[ADR 0009](0009-website-distribution.md) kept the source private so tramp.music could be a closed product. Bundling full libmpv then required an LGPL rebuild of the Windows pin (shinchiro’s `mpv-dev` is GPL) plus matching Linux/macOS pins and relink notices. That hassle outweighed keeping the source closed.

## Decision

Tramp is **GPL-3.0-or-later** ([ADR 0012](0012-gpl-3.md)). Listeners get binaries from the **official download** at `https://tramp.music`, plus the store-shaped exceptions: Microsoft Store ([ADR 0011](0011-windows-store-and-exe.md)) and Flathub ([ADR 0013](0013-linux-flathub-and-appimage.md)). Mac App Store and Snap stay out of v1.

Existing installs learn about a new version **in-app**. The prompt follows **install channel**: Store → Store, Flathub → Flathub, everything else → tramp.music. The app does not replace itself.

Public git is GitHub; tramp.music remains the download UI (GitHub Releases may mirror, they are not the product).

## Consequences

The current full-libmpv pins can stay in the GPL family. GPL also means each published binary needs corresponding source (a public repo or a source archive next to the installer). Gatekeeper and SmartScreen still apply to website downloads; open-source does not sign the app.
