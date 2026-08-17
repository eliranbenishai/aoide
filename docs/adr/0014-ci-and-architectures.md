# 14. Release artifacts come from GitHub Actions; v1 is x64 plus universal Mac

Date: 2026-08-14

## Status

Accepted

## Context

v1 ships five listener artifacts (Windows MSIX, Windows EXE, Linux AppImage, Flathub Flatpak, macOS DMG) plus public source. Building those by hand on Fedora, a Windows VM, and an occasional Mac will drift. Notarization needs a Mac toolchain; GitHub’s `macos-latest` runner can hold the Apple Developer cert in secrets.

## Decision

**GitHub Actions** produces every release artifact: `windows-latest`, `macos-latest`, `ubuntu-latest`. Partner Center and Flathub submission stay human clicks on CI-built bits.

v1 CPU matrix:

- **Windows:** x64 Store MSIX and x64 website EXE (Inno Setup). ARM later.
- **macOS:** one universal notarized DMG (Apple Silicon + Intel).
- **Linux:** x86_64 AppImage and x86_64 Flathub. No ARM AppImage in v1; aarch64 Flathub only if it stays a cheap extra job.

## Consequences

Apple notarization secrets and any Store identity variables live in GitHub, not on a laptop. Workflows: `.github/workflows/ci.yml` (PR/main tests) and `release.yml` (tag `v*` packages). How to cut a release and which secrets to set: [`distribution.md`](../distribution.md). Windows ARM and Linux ARM AppImage are explicit non-goals until a real need. The website EXE remains unsigned; SmartScreen is documented, not “fixed” by CI. The macOS DMG job waits on the Qt Mac host ([ADR 0016](0016-qt-for-v1.md)).
