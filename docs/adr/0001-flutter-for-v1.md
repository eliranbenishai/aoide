# Flutter for Tramp v1

Date: 2026-08-02

## Status

Superseded by [ADR 0016](0016-qt-for-v1.md).

Tramp v1 is a UX-first desktop player with custom app chrome and a dense playlist UI. We locked **Flutter** (Windows, Linux, macOS) so the shell owns its pixels and packaging stays one-codebase. Tauri 2 + Rust was the researched runner-up but loses on hybrid WebView performance for this product shape; Electron is out of v1.

The host is now Qt 6 C++. This ADR remains as the original stack lock.

