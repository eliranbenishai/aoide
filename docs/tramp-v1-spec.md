# Tramp v1 product spec

Product requirements for the first shippable Tramp desktop player. Domain vocabulary: [`CONTEXT.md`](../CONTEXT.md). Architecture map: [`architecture.md`](architecture.md). Stack decision: [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md).

**UI authority:** [`player-mockup-2.html`](../player-mockup-2.html) (visual + geometry) and the redesign design doc [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md). Earlier PNG-graphite / single-window docs are historical and must not steer new work.

Wayfinding decisions that produced the prior single-window graphite direction live under [`.scratch/tramp-v1-spec/`](../.scratch/tramp-v1-spec/).

## Vision

Tramp is a multi-platform desktop music player — a spiritual successor to Winamp: dense, control-forward, playlist-centric, with distinctive chrome rather than a generic streaming-app shell. v1 is local playback only, with three Winamp-style dockable windows, **code-constructed** chrome that matches the mockup exactly, and **full libmpv** so audible EQ, real spectrum, and force-mono are real (classic Winamp WSZ skins come later).

## Platforms

- **Windows, Linux, and macOS** desktop.
- One codebase; shippable artifacts via Flutter desktop packaging (`flutter build` / normal platform installers as appropriate).
- **App-store listings** (Microsoft Store, Mac App Store, Flathub, etc.) are **not** required for v1.

## Stack

| Locked | Preferred default (swappable for same job) | Not locked |
|--------|--------------------------------------------|------------|
| **Flutter** (desktop) | Multi-window host + docking coordinator — app chrome | State management |
| | media_kit control seam + **full libmpv** binaries — playback / decode / EQ / mono | Routing |
| | | Exact Flutter/Dart SDK versions |
| | | Design-system packages |

**Not v1:** Tauri, Electron, or a second UI toolkit. Fall back to direct libmpv FFI only if the media_kit + custom-binary spike fails. See [ADR 0005](adr/0005-full-libmpv.md).

## Window and chrome

- **Three detachable windows** — Main Player, Equalizer, and Playlist Editor — with **Winamp-style docking** (edge snap + sticky group drag). EQ and playlist may both be open; main EQ/PL toggles show/hide those windows ([ADR 0006](adr/0006-multi-window-docking.md)).
- **App chrome** — no OS title bar or standard window frame; the visible UI is the app surface.
- **Main / equalizer never stretch** — permanent. On-screen size follows the global zoom step only (logical canvases: main **825×348**, EQ **825×348**). Playlist is freely resizable (default logical **825×696**); user size is stored in logical coordinates and scaled by zoom ([ADR 0002](adr/0002-fixed-canvas-zoom.md), [ADR 0006](adr/0006-multi-window-docking.md)).
- **Drag** title-bar / grip regions to move that window or its dock group.
- **Controls:** in-app minimize (main minimizes the visible docked group), zoom-in / zoom-out on the main title bar (global), and close (main quits; EQ/PL hide). No maximize-as-size-control.
- **Zoom:** six discrete steps from 100% to 300%, persisted across sessions; steps larger than the current display’s work area are disabled. Main title-bar zoom-in / zoom-out (and matching menu/shortcuts) change the step for **all three** windows.
- **Windowshade:** EQ and playlist title-bar collapse → title bar only; docking uses shaded height.
- **Clutterbar** on main: product letters **O / A / I** only (Options, always-on-top for the visible docked group, track info). No D, no V, no ghost glyphs.

## UI direction

Dark shell chrome with **cyan phosphor** — multi-stop shell gradients, screen wells, brushed plates/rails — matching `player-mockup-2.html` at 100% zoom (geometry, tokens, type, materials). Look is **code-constructed** in Flutter from the mockup recipe ([ADR 0007](adr/0007-code-constructed-mockup-chrome.md)); no PNG panel faces, no nine-slice graphite pack. Dense, playlist-centric, control-forward. Not Material/Fluent defaults; not whole-chrome “Scalable UI”; not the retired PNG-graphite skin path.

Fidelity contract and palette: design doc §3. Fonts: **Tramp Condensed** (700) and **Tramp Mono** (500), embedded and used on chrome. Classic Winamp **WSZ** skin loading remains a non-goal for v1.

## Playback

### Formats (local tracks)

Must decode and play: **MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus**.

### Engine

- Bundle **full libmpv** (+ required FFmpeg) on Windows, macOS, and Linux — features first; binary size later ([ADR 0005](adr/0005-full-libmpv.md)).
- Keep media_kit as the control seam for open, transport, seek, volume, and playlist advance; packaging must load our full binaries, not stock slim ones.
- **Audible 10-band EQ** (measurement-gated before the UI claims it), **real** LCD spectrum (**20** bars), and **Mono** (force downmix when on). Synthetic spectrum levels are a failure/dev signal, not the product end-state.

### Controls

- Play and Pause as **separate** controls; stop; previous / next; open/eject
- Seek bar
- Volume and mute
- **Mono** (force-downmix output mode; meta STEREO/MONO reflects source layout)
- Shuffle
- Repeat: off / all / one
- Show current-track title / artist / album from tags when present
- OS media-key support

### How music enters

- Open files/folders, drag-and-drop, and playlists only.
- **No media library** (no scanned catalog / database) in v1.

## Playlist manager

- Create, open, and save playlists as **M3U / M3U8**.
- Add, remove, reorder, sort (title / artist / duration / path / reverse), and selection ops (select-all / invert, clear, etc.).
- Play from selection (e.g. double-click / activate row).
- Restore the last playlist across sessions (path, logical window size, shade, dock offsets).

## File associations

Associate Tramp with v1 audio formats and `.m3u` / `.m3u8` so “Open with Tramp” / double-click opens (or focuses) the app and loads/plays appropriately. OS registration details are implementation concerns.

## Accessibility

- Keyboard can drive transport and playlist selection.
- Use Flutter semantics defaults where cheap.
- No WCAG certification or full a11y audit as a v1 gate.

## Non-goals (v1)

- Classic Winamp **WSZ** skin loading
- Classic visualization modes/plugins (clutterbar **V**)
- Clutterbar **D** / doublesize (zoom controls already exist)
- Freely resizing or stretching the main player or equalizer canvases (permanent)
- Whole-chrome continuously scaling UI (“Scalable UI”); playlist free resize is allowed
- PNG-first graphite skin delivery (retired — [ADR 0004](adr/0004-png-graphite-skin.md) superseded)
- Single-window EQ/PL mutual exclusion as the product model (retired — [ADR 0003](adr/0003-zoom-only-window-size.md) superseded)
- Media library / scanned catalog
- Streaming services
- Plugin ecosystem
- Gapless playback
- Crossfade
- Trusting mpv filter “success” without measuring output
- Approximate or partial chrome cutovers (no half-mockup / half-graphite ship)
- App-store listing requirements
- Licensing posture inside this document (decide separately via `LICENSE` / README)

## Success criteria

v1 is done when a user can install Tramp on Windows, Linux, and macOS, open local audio and playlists, manage a large playlist in a freely resizable playlist window, control playback with three dockable windows whose chrome matches `player-mockup-2.html` at 100% zoom, hear measurement-proven EQ, see a real 20-bar spectrum, and use Mono — without depending on a library, WSZ skins, PNG graphite faces, or store distribution.

## Related artifacts

| Artifact | Role |
|----------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Domain glossary |
| [`architecture.md`](architecture.md) | Living structure map |
| [`player-mockup-2.html`](../player-mockup-2.html) | Visual + geometric authority |
| [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md) | Redesign design (current direction) |
| [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md) | Flutter stack ADR |
| [`adr/0002-fixed-canvas-zoom.md`](adr/0002-fixed-canvas-zoom.md) | Fixed-canvas zoom ADR (revised) |
| [`adr/0005-full-libmpv.md`](adr/0005-full-libmpv.md) | Full libmpv bundling ADR |
| [`adr/0006-multi-window-docking.md`](adr/0006-multi-window-docking.md) | Multi-window + docking ADR |
| [`adr/0007-code-constructed-mockup-chrome.md`](adr/0007-code-constructed-mockup-chrome.md) | Code-constructed mockup chrome ADR |
| [`adr/0003-zoom-only-window-size.md`](adr/0003-zoom-only-window-size.md) | Superseded (single-window framing) |
| [`adr/0004-png-graphite-skin.md`](adr/0004-png-graphite-skin.md) | Superseded (PNG graphite) |
| [`.scratch/tramp-v1-spec/research/v1-stack.md`](../.scratch/tramp-v1-spec/research/v1-stack.md) | Stack research evidence (historical) |
| [`superpowers/specs/2026-08-02-graphite-skin-delivery-design.md`](superpowers/specs/2026-08-02-graphite-skin-delivery-design.md) | Historical PNG graphite delivery (superseded look) |
| [`.scratch/tramp-v1-spec/map.md`](../.scratch/tramp-v1-spec/map.md) | Wayfinder decision index |
