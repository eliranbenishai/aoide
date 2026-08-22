# Tramp v1 product spec

Product requirements for the first shippable Tramp desktop player. Domain vocabulary: [`CONTEXT.md`](../CONTEXT.md). Architecture map: [`architecture.md`](architecture.md).

**UI authority:** [`player-mockup-2.html`](../player-mockup-2.html) (visual + geometry), the redesign design doc [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md), and polish rules in [`superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md) (product overrides such as compact EQ/PL titles and EQ band fill). Earlier PNG-graphite / single-window docs are historical and must not steer new work. Flutter-era host wording and five-OS-window host wording in those design docs are historical; the host is Qt 6 with one host window and internal panels.

Wayfinding that produced the prior single-window graphite direction is historical; the lock is Qt 6 and the product rules in this file.

## Vision

Tramp is a multi-platform desktop music player — a spiritual successor to Winamp: dense, control-forward, playlist-centric, with distinctive chrome rather than a generic streaming-app shell. v1 is local playback only, with three Winamp-style dockable panels inside one host window, **code-constructed** chrome that matches the mockup exactly, and **full libmpv** so audible EQ, real spectrum, and force-mono are real (classic Winamp WSZ skins come later).

## Platforms

- **Windows, Linux, and macOS** desktop.
- One codebase; shippable artifacts via the Qt host (`src/`) and platform installers.
- **Official download** is `https://tramp.music`. Windows lists on the **Microsoft Store** as **tramp.music** (MSIX) **and** offers an unsigned website EXE. Linux lists on **Flathub** **and** offers an AppImage. macOS is a notarized DMG from the site. Mac App Store and Snap are **not** v1.
- License: **GPL-3.0-or-later**.
- Release artifacts are built on **GitHub Actions**. v1 CPUs: Windows x64, Linux x86_64, macOS universal.
- **In-app new-version prompt — intended, not built.** The design is to follow the **install channel** (Store → Store, Flathub → Flathub, otherwise tramp.music) and send the listener there rather than replace the app in place. Nothing in `src/` detects the channel, checks for a new version, or prompts.

## Stack

| Locked | Preferred default (swappable for same job) | Not locked |
|--------|--------------------------------------------|------------|
| **Qt 6** (desktop, QWidget + QPainter) | One host window, five panels + docking coordinator — app chrome | State helpers inside `src/` |
| CMake; version in [`VERSION`](../VERSION) | **full libmpv** — playback / decode / EQ / mono | |

**Not v1:** Flutter, Tauri, Electron, or a second UI toolkit. Qt 6 and full libmpv are the stack lock.

## Window and chrome

- **Five panels, one host window** — Main Player, Equalizer, Playlist Manager, settings, and about are **panels** inside a single OS **host window**. Main/EQ/PL keep **Winamp-style docking**; settings and about are freestanding. EQ and playlist may both be open; main EQ/PL toggles show/hide those panels. Gaps between panels punch through so the desktop is clickable. The playlist panel holds the **playlist collection** beside the current playlist; its default canvas is 1073×696.
- **Move / snap ownership** — dragging the main title bar translates every panel inside the host (cluster moves as a unit; host stays the virtual desktop). Dragging EQ, playlist, settings, or about moves only that panel; siblings stay put. Snap only from EQ/playlist: any side of any panel (both axes in one drop). Details: [`2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md).
- **App chrome** — no OS title bar or standard window frame; the visible UI is the app surface. Title-bar window buttons match mockup `.wbtn` bevel chrome. Main title bar shows logo + TRAMP wordmark; EQ/playlist title bars show **role title only**.
- **No version in the title bar** — the mockup's `TRAMP<sup>1.0</sup>` superscript is dropped (approved delta). The version belongs to the About panel's readout, where it is real and comes from [`VERSION`](../VERSION).
- **Main / equalizer never stretch** — permanent. On-screen size follows the global zoom step only (logical canvases: main **825×348**, EQ **825×348**). Playlist is freely resizable (default logical **1073×696**, as above); user size is stored in logical coordinates and scaled by zoom.
- **Controls:** in-app minimize (main hides/restores visible secondaries then minimizes the host window), zoom-in / zoom-out on the main title bar (global), and close (main quits; EQ/PL hide). No maximize-as-size-control.
- **Taskbar:** the host window (Tramp) is the taskbar/pager entry.
- **Zoom:** four discrete steps — 75%, 100%, 125%, 150% (default **75%**) — persisted across sessions; a saved factor that is no longer a step snaps to the nearest surviving one, and steps larger than the current display’s work area are disabled. Main title-bar zoom-in / zoom-out change the step for **all three** dockable panels, and are the only control that does: there is no zoom row in the options menu and no zoom shortcut (see Accessibility).
- **Windowshade:** EQ and playlist title-bar collapse → title bar only; docking uses shaded height.
- **EQ band faders:** bottom→thumb fill using the spectrum cyan→magenta gradient (product enhancement vs mockup HTML bands).
- **Options cog** on main (top-left of body): opens Always on top / Settings… / Track info / About Tramp / Open files… / Quit. Replaces mockup clutter **O / A / I** (approved delta). Skins are a tab inside the settings panel, not a menu row — “look pack” is a retired term ([`CONTEXT.md`](../CONTEXT.md): **Skin**).

## UI direction

Dark shell chrome with **cyan phosphor** — multi-stop shell gradients, screen wells, brushed plates/rails — matching `player-mockup-2.html` at 100% zoom (geometry, tokens, type, materials). Look is **code-constructed** in Qt from the mockup recipe; no PNG panel faces, no nine-slice graphite pack. Dense, playlist-centric, control-forward. Not Material/Fluent defaults; not whole-chrome “Scalable UI”; not the retired PNG-graphite skin path.

Fidelity contract and palette: design doc §3. Fonts: **Tramp Condensed** (700) and **Tramp Mono** (500), embedded and used on chrome. Classic Winamp **WSZ** skin loading remains a non-goal for v1.

## Playback

### Formats (local tracks)

Must decode and play: **MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus**.

### Engine

- Bundle **full libmpv** (+ required FFmpeg) on Windows, macOS, and Linux — features first; binary size later.
- Talk to libmpv through the in-process `PlayerEngine` seam (`MpvEngine`).
- **Audible 10-band EQ** (measurement-gated before the UI claims it), **real** LCD spectrum (**20** bars), and **Mono** (force downmix when on). Synthetic spectrum levels are a failure/dev signal, not the product end-state — an unmeasured spectrogram (including a decode past its 120 s deadline) is a **transient notice** in the display well. Vocabulary: [`CONTEXT.md`](../CONTEXT.md): **Failure surface**.

### Controls

- Play and Pause as **separate** controls; stop; previous / next; open/eject
- Seek bar
- Volume and mute
- **Mono** (force-downmix output mode; meta STEREO/MONO reflects source layout)
- Shuffle
- Repeat: off / all / one
- Show current-track title / artist / album from tags when present
- **Media keys** — play/pause, stop, next and previous, while Tramp is the focused application. They are application shortcuts, not a system-wide registration: there is no MPRIS or SMTC surface, so a media key pressed while another app has focus does nothing for Tramp.
- **Session resume** (*Resume playback*, on by default): the transport reopens the track the last session was left on and seeks back to where it stopped, paused if it was paused and playing if it was playing. A session quit from **stop** launches with the playlist and an empty transport. The current playlist itself comes back either way. Vocabulary: [`CONTEXT.md`](../CONTEXT.md): **Session resume**, **Resume playback**.

### How music enters

- Open files/folders, drag-and-drop, and playlists only.
- **No media library** (no scanned catalog / database) in v1.
- **Empty states** (no first-run flag, no tour): an empty track well paints `THIS LIST IS EMPTY` / `Drop files here, or open one from PLAYLISTS.`; an empty collection well paints `NO SAVED PLAYLISTS` / `A playlist is a file you keep. Tramp does not scan a library.`; the main title is `Drop files to play` while it would otherwise be `No track` and the current list has no rows. The footer drop hint is chrome, not a third well line.

## Playlist manager

- Create, open, and save playlists as **M3U / M3U8**.
- Add, remove, reorder, clear, and select one row (current logic).
- Playlist Options: sort (title / artist / duration / path / reverse) and bulk selection (select-all / invert).
- Play from selection (e.g. double-click / activate row).
- Restore the last playlist across sessions (path, logical panel size, shade, dock offsets).

## File associations

Associate Tramp with v1 audio formats and `.m3u` / `.m3u8` so “Open with Tramp” / double-click opens (or focuses) the app and loads/plays appropriately. OS registration details are implementation concerns.

## Accessibility

- **The whole keyboard surface:** Space toggles play/pause; Ctrl+A selects every row of the current playlist; Delete and Backspace remove the selected rows; the four media keys drive play/pause, stop, next and previous. Arrow keys, Enter and Escape work inside an open options menu. Shift and Ctrl qualify a mouse gesture rather than standing alone — range- and toggle-select in the track list, and **Shift** to undock a panel that a slow drag would never peel. That is all of it.
- **Keyboard navigation and the accessibility tree are deferred whole** (2026-08-21) — deferred, not dropped, and not staged either. There is no way to *move* the playlist selection from the keyboard, no focus policy anywhere, and nothing in `src/` builds an accessibility tree (no `QAccessible`, no `setAccessibleName`). Volume, seek, the EQ bands and presets, the Playlist Manager, settings, skins, zoom, shade and dock are mouse-only — Shift-undock included, since the modifier only qualifies a drag. The reasoning and the price are recorded in [`premises.md`](premises.md) §8; this is the one deferral that excludes people rather than inconveniencing them, and it should be the first thing picked up.
- No WCAG certification or full a11y audit as a v1 gate.

## Non-goals (v1)

- Classic Winamp **WSZ** skin loading
- Classic visualization modes/plugins (clutterbar **V**)
- Clutterbar **D** / doublesize (zoom controls already exist)
- Freely resizing or stretching the main player or equalizer canvases (permanent)
- Whole-chrome continuously scaling UI (“Scalable UI”); playlist free resize is allowed
- PNG-first graphite skin delivery (retired)
- Single-window EQ/PL mutual exclusion as the product model (retired)
- Media library / scanned catalog
- Streaming services
- Plugin ecosystem
- Gapless playback
- Crossfade
- Trusting mpv filter “success” without measuring output
- Approximate or partial chrome cutovers (no half-mockup / half-graphite ship)
- Mac App Store and Snap Store listings

## Premises

Eight rules in this file are bets rather than findings: no media library, playlist-only organisation on paths-as-hints, the 75% default against mockup-absolute fidelity at 100%, recolour-only skins, the homage as the product, the `tramp.music` Store listing, Free Forever with no funding model, and the virtual-desktop host with fully custom chrome. [`premises.md`](premises.md) records each one dated, with what is genuinely known, what it costs today, and the observable event that should reopen it. There is no telemetry, so none of them will be settled by measurement; a dated note that a premise is unsettled is the honest version, and that register is where it lives. Glossary entries resting on one carry a `_Premise_` line.

## Success criteria

v1 is done when a user can install Tramp on Windows, Linux, and macOS, open local audio and playlists, manage a large playlist in a freely resizable playlist panel, control playback with three dockable panels inside one host window whose chrome matches `player-mockup-2.html` at 100% zoom, hear measurement-proven EQ, see a real 20-bar spectrum, and use Mono — without depending on a library, WSZ skins, PNG graphite faces, or any single store. Windows install must work from the Microsoft Store and from the website EXE; Linux from Flathub and from the AppImage.

## Related artifacts

| Artifact | Role |
|----------|------|
| [`CONTEXT.md`](../CONTEXT.md) | Domain glossary |
| [`architecture.md`](architecture.md) | Living structure map |
| [`premises.md`](premises.md) | Dated record of the product bets and what would reopen each |
| [`player-mockup-2.html`](../player-mockup-2.html) | Visual + geometric authority |
| [`superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md) | Redesign design (multi-window cutover) |
| [`superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md`](superpowers/specs/2026-08-09-ui-polish-docking-taskbar-design.md) | Docking / title / EQ fill / taskbar polish |
| [`superpowers/specs/2026-08-02-graphite-skin-delivery-design.md`](superpowers/specs/2026-08-02-graphite-skin-delivery-design.md) | Historical PNG graphite delivery (superseded look) |
