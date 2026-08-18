# Tramp v1 product spec

Product requirements for the first shippable Tramp desktop player. Domain vocabulary: [`CONTEXT.md`](../CONTEXT.md). Architecture map: [`architecture.md`](architecture.md). Stack decision: [`adr/0016-qt-for-v1.md`](adr/0016-qt-for-v1.md).

**UI authority:** [`player-mockup-2.html`](../player-mockup-2.html) (visual + geometry), the redesign design doc [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md), and polish rules in [`superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md) (product overrides such as compact EQ/PL titles and EQ band fill). Earlier PNG-graphite / single-window docs are historical and must not steer new work. Flutter-era host wording and five-OS-window host wording in those design docs are historical; the host is Qt ([ADR 0016](adr/0016-qt-for-v1.md)) with one host window and internal panels ([ADR 0017](adr/0017-one-host-window-internal-panels.md)).

Wayfinding that produced the prior single-window graphite direction is historical; the lock is [ADR 0016](adr/0016-qt-for-v1.md) and the product rules in this file.

## Vision

Tramp is a multi-platform desktop music player — a spiritual successor to Winamp: dense, control-forward, playlist-centric, with distinctive chrome rather than a generic streaming-app shell. v1 is local playback only, with three Winamp-style dockable panels inside one host window, **code-constructed** chrome that matches the mockup exactly, and **full libmpv** so audible EQ, real spectrum, and force-mono are real (classic Winamp WSZ skins come later).

## Platforms

- **Windows, Linux, and macOS** desktop.
- One codebase; shippable artifacts via the Qt host (`src/`) and platform installers.
- **Official download** is `https://tramp.music`. Windows lists on the **Microsoft Store** as **tramp.music** (MSIX) **and** offers an unsigned website EXE ([ADR 0011](adr/0011-windows-store-and-exe.md)). Linux lists on **Flathub** **and** offers an AppImage ([ADR 0013](adr/0013-linux-flathub-and-appimage.md)). macOS is a notarized DMG from the site. Mac App Store and Snap are **not** v1.
- License: **GPL-3.0-or-later** ([ADR 0012](adr/0012-gpl-3.md)).
- Release artifacts are built on **GitHub Actions**. v1 CPUs: Windows x64, Linux x86_64, macOS universal ([ADR 0014](adr/0014-ci-and-architectures.md)).
- In-app new-version prompt follows **install channel** (Store → Store, Flathub → Flathub, otherwise tramp.music). The app does not replace itself.

## Stack

| Locked | Preferred default (swappable for same job) | Not locked |
|--------|--------------------------------------------|------------|
| **Qt 6** (desktop, QWidget + QPainter) | One host window, five panels + docking coordinator — app chrome | State helpers inside `src/` |
| CMake; version in [`VERSION`](../VERSION) | **full libmpv** — playback / decode / EQ / mono | |

**Not v1:** Flutter, Tauri, Electron, or a second UI toolkit. See [ADR 0016](adr/0016-qt-for-v1.md) and [ADR 0005](adr/0005-full-libmpv.md).

## Window and chrome

- **Five panels, one host window** — Main Player, Equalizer, Playlist Manager, settings, and about are **panels** inside a single OS **host window** ([ADR 0017](adr/0017-one-host-window-internal-panels.md)). Main/EQ/PL keep **Winamp-style docking**; settings and about are freestanding. EQ and playlist may both be open; main EQ/PL toggles show/hide those panels ([ADR 0006](adr/0006-multi-window-docking.md)). Gaps between panels punch through so the desktop is clickable. The playlist panel holds the **playlist collection** beside the current playlist ([ADR 0008](adr/0008-playlist-collection-stores-references.md)); its default canvas is 1073×696.
- **Move / snap ownership** — dragging the main title bar translates every panel inside the host (cluster moves as a unit; host stays the virtual desktop). Dragging EQ, playlist, settings, or about moves only that panel; siblings stay put. Snap only from EQ/playlist: EQ any side of any panel; playlist **top/bottom only**, with left/right flush when already within threshold. Details: [`2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md), [ADR 0019](adr/0019-virtual-desktop-punched-host.md).
- **App chrome** — no OS title bar or standard window frame; the visible UI is the app surface. Title-bar window buttons match mockup `.wbtn` bevel chrome. Main title bar shows logo + TRAMP wordmark; EQ/playlist title bars show **role title only**.
- **No version in the title bar** — the mockup's `TRAMP<sup>1.0</sup>` superscript is dropped (approved delta). The version belongs to the About panel's readout, where it is real and comes from [`VERSION`](../VERSION).
- **Main / equalizer never stretch** — permanent. On-screen size follows the global zoom step only (logical canvases: main **825×348**, EQ **825×348**). Playlist is freely resizable (default logical **825×696**); user size is stored in logical coordinates and scaled by zoom ([ADR 0002](adr/0002-fixed-canvas-zoom.md), [ADR 0006](adr/0006-multi-window-docking.md)).
- **Controls:** in-app minimize (main hides/restores visible secondaries then minimizes the host window), zoom-in / zoom-out on the main title bar (global), and close (main quits; EQ/PL hide). No maximize-as-size-control.
- **Taskbar:** the host window (Tramp) is the taskbar/pager entry ([ADR 0017](adr/0017-one-host-window-internal-panels.md)).
- **Zoom:** eight discrete steps from 50% to 300% (default **75%**), persisted across sessions; steps larger than the current display’s work area are disabled. Main title-bar zoom-in / zoom-out (and matching menu/shortcuts) change the step for **all three** dockable panels.
- **Windowshade:** EQ and playlist title-bar collapse → title bar only; docking uses shaded height.
- **EQ band faders:** bottom→thumb fill using the spectrum cyan→magenta gradient (product enhancement vs mockup HTML bands).
- **Options cog** on main (top-left of body): opens Always on top / Look packs… / Track info / About / Quit. Replaces mockup clutter **O / A / I** (approved delta).

## UI direction

Dark shell chrome with **cyan phosphor** — multi-stop shell gradients, screen wells, brushed plates/rails — matching `player-mockup-2.html` at 100% zoom (geometry, tokens, type, materials). Look is **code-constructed** in Qt from the mockup recipe ([ADR 0007](adr/0007-code-constructed-mockup-chrome.md)); no PNG panel faces, no nine-slice graphite pack. Dense, playlist-centric, control-forward. Not Material/Fluent defaults; not whole-chrome “Scalable UI”; not the retired PNG-graphite skin path.

Fidelity contract and palette: design doc §3. Fonts: **Tramp Condensed** (700) and **Tramp Mono** (500), embedded and used on chrome. Classic Winamp **WSZ** skin loading remains a non-goal for v1.

## Playback

### Formats (local tracks)

Must decode and play: **MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus**.

### Engine

- Bundle **full libmpv** (+ required FFmpeg) on Windows, macOS, and Linux — features first; binary size later ([ADR 0005](adr/0005-full-libmpv.md)).
- Talk to libmpv through the in-process `PlayerEngine` seam (`MpvEngine`).
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
- Add, remove, reorder, clear, and select one row (current logic).
- Playlist Options: sort (title / artist / duration / path / reverse) and bulk selection (select-all / invert).
- Play from selection (e.g. double-click / activate row).
- Restore the last playlist across sessions (path, logical panel size, shade, dock offsets).

## File associations

Associate Tramp with v1 audio formats and `.m3u` / `.m3u8` so “Open with Tramp” / double-click opens (or focuses) the app and loads/plays appropriately. OS registration details are implementation concerns.

## Accessibility

- Keyboard can drive transport and playlist selection.
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
- Mac App Store and Snap Store listings

## Success criteria

v1 is done when a user can install Tramp on Windows, Linux, and macOS, open local audio and playlists, manage a large playlist in a freely resizable playlist panel, control playback with three dockable panels inside one host window whose chrome matches `player-mockup-2.html` at 100% zoom, hear measurement-proven EQ, see a real 20-bar spectrum, and use Mono — without depending on a library, WSZ skins, PNG graphite faces, or any single store. Windows install must work from the Microsoft Store and from the website EXE; Linux from Flathub and from the AppImage.

## Related artifacts

| Artifact | Role |
|----------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Domain glossary |
| [`architecture.md`](architecture.md) | Living structure map |
| [`player-mockup-2.html`](../player-mockup-2.html) | Visual + geometric authority |
| [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md) | Redesign design (multi-window cutover) |
| [`superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md) | Docking / title / EQ fill / taskbar polish |
| [`adr/0016-qt-for-v1.md`](adr/0016-qt-for-v1.md) | Qt 6 host ADR |
| [`adr/0001-flutter-for-v1.md`](adr/0001-flutter-for-v1.md) | Historical Flutter stack lock (superseded) |
| [`adr/0002-fixed-canvas-zoom.md`](adr/0002-fixed-canvas-zoom.md) | Fixed-canvas zoom ADR (revised) |
| [`adr/0005-full-libmpv.md`](adr/0005-full-libmpv.md) | Full libmpv bundling ADR |
| [`adr/0006-multi-window-docking.md`](adr/0006-multi-window-docking.md) | Docking + snap + shade (five-OS-window host superseded by 0017) |
| [`adr/0017-one-host-window-internal-panels.md`](adr/0017-one-host-window-internal-panels.md) | One host window, five panels, punch-click |
| [`adr/0019-virtual-desktop-punched-host.md`](adr/0019-virtual-desktop-punched-host.md) | Host is the virtual desktop; no resize on panel drag |
| [`adr/0007-code-constructed-mockup-chrome.md`](adr/0007-code-constructed-mockup-chrome.md) | Code-constructed mockup chrome ADR |
| [`adr/0003-zoom-only-window-size.md`](adr/0003-zoom-only-window-size.md) | Superseded (single-window framing) |
| [`adr/0004-png-graphite-skin.md`](adr/0004-png-graphite-skin.md) | Superseded (PNG graphite) |
| [`superpowers/specs/2026-08-02-graphite-skin-delivery-design.md`](superpowers/specs/2026-08-02-graphite-skin-delivery-design.md) | Historical PNG graphite delivery (superseded look) |
