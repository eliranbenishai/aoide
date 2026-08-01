# Tramp v1 product spec

Product requirements for the first shippable Tramp desktop player. Domain vocabulary: [`CONTEXT.md`](../CONTEXT.md). Architecture map: [`architecture.md`](architecture.md). Stack decision: [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md).

Wayfinding decisions that produced this document live under [`.scratch/tramp-v1-spec/`](../.scratch/tramp-v1-spec/).

## Vision

Tramp is a multi-platform desktop music player — a spiritual successor to Winamp: dense, control-forward, playlist-centric, with distinctive chrome rather than a generic streaming-app shell. v1 is local playback only, with a scalable custom UI (classic Winamp skins come later).

## Platforms

- **Windows, Linux, and macOS** desktop.
- One codebase; shippable artifacts via Flutter desktop packaging (`flutter build` / normal platform installers as appropriate).
- **App-store listings** (Microsoft Store, Mac App Store, Flathub, etc.) are **not** required for v1.

## Stack

| Locked | Preferred default (swappable for same job) | Not locked |
|--------|--------------------------------------------|------------|
| **Flutter** (desktop) | `window_manager` — app chrome | State management |
| | media_kit (libmpv) — playback / decode | Routing |
| | | Exact Flutter/Dart SDK versions |
| | | Design-system packages |

**Not v1:** Tauri, Electron, or a second UI toolkit.

## Window and chrome

- **One window** — transport, now-playing, and playlist in a single surface (no detachable multi-window layout).
- **App chrome** — no OS title bar or standard window frame; the visible UI is the app surface.
- **Resize** from edges/corners.
- **Drag** a designated region to move the window.
- **Controls:** in-app close and minimize. Maximize/fullscreen not required for v1.

## UI direction

Locked from the throwaway prototype ([`.scratch/tramp-v1-spec/prototype/`](../.scratch/tramp-v1-spec/prototype/), variant **W**):

- **Layout:** transport stack — brand + now-playing/transport on top; playlist fills the rest.
- **Design language:** paper/ink — warm surface `#f2ebe0`, ink `#1a1410`, terracotta accent `#c43c17`, Syne (or equivalent) wordmark, hard 2px borders, ink primary buttons.
- Dense, playlist-centric, playful but not “streaming clean.” Not Material/Fluent defaults.

Implementation should match this direction in Flutter; the HTML prototype is a reference, not production code.

## Playback

### Formats (local tracks)

Must decode and play: **MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus**.

### Controls

- Play / pause, stop, previous / next
- Seek bar
- Volume and mute
- Shuffle
- Repeat: off / all / one
- Show current-track title / artist / album from tags when present
- OS media-key support

### How music enters

- Open files/folders, drag-and-drop, and playlists only.
- **No media library** (no scanned catalog / database) in v1.

## Playlist manager

- Create, open, and save playlists as **M3U / M3U8**.
- Add, remove, and reorder tracks.
- Play from selection (e.g. double-click / activate row).
- Restore the last playlist across sessions.

## File associations

Associate Tramp with v1 audio formats and `.m3u` / `.m3u8` so “Open with Tramp” / double-click opens (or focuses) the app and loads/plays appropriately. OS registration details are implementation concerns.

## Accessibility

- Keyboard can drive transport and playlist selection.
- Use Flutter semantics defaults where cheap.
- No WCAG certification or full a11y audit as a v1 gate.

## Non-goals (v1)

- Classic Winamp skins
- Media library / scanned catalog
- Streaming services
- Plugin ecosystem
- Equalizer and visualizations as requirements
- Gapless playback
- Crossfade
- Detachable multi-window layout (main / playlist / EQ frames)
- App-store listing requirements
- Licensing posture inside this document (decide separately via `LICENSE` / README)

## Success criteria

v1 is done when a user can install Tramp on Windows, Linux, and macOS, open local audio and playlists, manage a playlist, control playback with the transport chrome above, and use a frameless, resizable, paper/ink UI that matches the locked direction — without depending on a library, skins, or store distribution.

## Related artifacts

| Artifact | Role |
|----------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Domain glossary |
| [`architecture.md`](architecture.md) | Living structure map |
| [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md) | Flutter stack ADR |
| [`.scratch/tramp-v1-spec/research/v1-stack.md`](../.scratch/tramp-v1-spec/research/v1-stack.md) | Stack research evidence |
| [`.scratch/tramp-v1-spec/prototype/`](../.scratch/tramp-v1-spec/prototype/) | UI direction prototype (variant W) |
| [`.scratch/tramp-v1-spec/map.md`](../.scratch/tramp-v1-spec/map.md) | Wayfinder decision index |
