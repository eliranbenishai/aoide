# Flutter for Tramp v1

Tramp v1 is a UX-first desktop player with custom app chrome and a dense playlist UI. We lock **Flutter** (Windows, Linux, macOS) so the shell owns its pixels and packaging stays one-codebase. Tauri 2 + Rust was the researched runner-up but loses on hybrid WebView performance for this product shape; Electron is out of v1.

Preferred defaults (not frozen): `window_manager` for app chrome, media_kit/libmpv for playback. State management, routing, SDK pins, and design-system packages stay unspecified until implementation needs them.
