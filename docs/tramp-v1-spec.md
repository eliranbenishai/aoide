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

- **One window** — transport, now-playing, and playlist/equalizer in a single surface (no detachable multi-window layout).
- **App chrome** — no OS title bar or standard window frame; the visible UI is the app surface.
- **Resize** from edges/corners.
- **Drag** a designated region to move the window.
- **Controls:** in-app minimize, maximize, and close are all implemented.
- **Zoom:** six discrete steps from 100% to 300%, persisted across sessions; steps larger than the current display’s work area are disabled.

## UI direction

Dark **graphite chrome** with acid chartreuse phosphor — near-black panels, warm yellow title-bar rails, fixed logical canvases (main player 812×242, equalizer 812×206) with absolute placement and a single root zoom transform. Dense, playlist-centric, control-forward. Not Material/Fluent defaults; not the earlier light brushed-metal / vector-chrome direction.

Visual target: [`docs/superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md`](superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md). Reference mockup: [`docs/mockups/graphite-chrome.png`](mockups/graphite-chrome.png). Classic Winamp **WSZ skins** remain a non-goal for v1.

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

- Classic Winamp **WSZ** / bitmap skins (graphite chrome is the v1 look; skin loading comes later)
- Media library / scanned catalog
- Streaming services
- Plugin ecosystem
- Equalizer **chrome** ships; audible equalization does not, because the shipped libmpv cannot construct filter graphs
- L/R VU meters (that chrome is the volume slider)
- Gapless playback
- Crossfade
- Detachable multi-window layout (main / playlist / EQ frames)
- App-store listing requirements
- Licensing posture inside this document (decide separately via `LICENSE` / README)

## Success criteria

v1 is done when a user can install Tramp on Windows, Linux, and macOS, open local audio and playlists, manage a playlist, control playback with the graphite chrome above, and use a frameless, resizable, zoomable UI that matches the locked direction — without depending on a library, WSZ skins, or store distribution.

## Related artifacts

| Artifact | Role |
|----------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Domain glossary |
| [`architecture.md`](architecture.md) | Living structure map |
| [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md) | Flutter stack ADR |
| [`adr/0002-fixed-canvas-zoom.md`](adr/0002-fixed-canvas-zoom.md) | Fixed-canvas zoom ADR |
| [`.scratch/tramp-v1-spec/research/v1-stack.md`](../.scratch/tramp-v1-spec/research/v1-stack.md) | Stack research evidence |
| [`superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md`](superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md) | Graphite chrome UI direction |
| [`mockups/graphite-chrome.png`](mockups/graphite-chrome.png) | Style reference mockup |
| [`.scratch/tramp-v1-spec/map.md`](../.scratch/tramp-v1-spec/map.md) | Wayfinder decision index |
