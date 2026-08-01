# Recommend the v1 implementation stack

Type: research
Status: resolved

## Question

Which implementation stack should the Tramp v1 product spec assume for a UX-first, multi-platform desktop player (Windows, Linux, macOS)?

Evaluate **Flutter** as the primary candidate and **Tauri 2 + Rust** as the runner-up. Electron is out of the first cut.

The recommendation must be evidence-backed on:

1. **UX flexibility** — custom app chrome (no OS window frame), edge resize, drag-to-move, dense playlist-centric UI, high-frequency UI updates (seek, playlist)
2. **Performance** — especially if recommending any web/native hybrid; must clear an “extremely performant” bar for a music player shell
3. **Local audio** — decode/playback path for MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus on all three OSes
4. **Packaging** — realistic desktop distribution on Windows, Linux, and macOS from one codebase

Resolve with a single recommended stack for v1 (and when the runner-up would still win), citing primary sources. Capture findings in-repo and link them from this ticket.

## Answer

**Recommend Flutter for Tramp v1** (custom chrome via `window_manager`, playback via media_kit/libmpv covering all v1 formats, `flutter build` for Win/Linux/macOS).

**Tauri 2 + Rust** remains the runner-up when the team is web+Rust-native, binary size is a hard constraint, and they accept WebView shell performance plus a separate Opus decode path (Symphonia/rodio do not cover Opus yet).

Full evidence and citations: [research/v1-stack.md](../research/v1-stack.md).
