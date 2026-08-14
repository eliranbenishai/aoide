# 13. Linux is AppImage plus Flathub

Date: 2026-08-14

## Status

Accepted

## Context

Linux needed the same split Windows already has ([ADR 0011](0011-windows-store-and-exe.md)): a store-shaped install that does MIME/desktop integration well, and a file you can run when you do not want a store. The two common website Linux *installers* (.deb/.rpm) were considered; the chosen pair is the portable file plus the Flatpak store.

## Decision

Linux has **two** obtain paths:

- **Flathub** — Flatpak. This is the Linux store exception (Mac App Store and Snap stay out).
- **Official download** — an **AppImage** on `https://tramp.music`.

The in-app new-version prompt follows **install channel**: Flathub opens Flathub (or `flatpak update`); AppImage opens tramp.music.

## Consequences

The Flatpak must bundle full libmpv (not a slim host library) and grant filesystem access suitable for a local-files player. AppImage desktop integration is weaker than Flatpak; document that. Two Linux artifacts, one version number.
