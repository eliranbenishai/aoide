# Tramp

A multi-platform desktop music player positioned as a spiritual successor to Winamp.

## Language

**Tramp**:
The product — a desktop music player that can be built for Windows, Linux, and macOS.
_Avoid_: Winamp clone, media player (when meaning this product)

**Classic skin**:
A Winamp-compatible skin that replaces the built-in chrome with the skin's own layout and art (WSZ). Out of v1 product path for mockup chrome.
_Avoid_: theme (when meaning a classic Winamp skin), skin pack (unless referring to a collection)

**Skin** (mockup recolor):
A shareable folder or zip (`skin.json` preferred, legacy `look.json` accepted + optional TTF/OTF) that recolors and optionally retypes the built-in mockup chrome — palette, a few named materials (bevel, spectrum/rail gradients), and font roles — without changing layout or art. Friendly slug ids; may extend `builtin` or another skin. Not a classic Winamp WSZ skin. Catalog default directory name is `skins`.
_Avoid_: classic skin, WSZ, theme (when meaning this pack), graphite skin, look pack (retired product term — same concept)

**App chrome**:
Tramp's own window decoration — no OS title bar or standard window frame; the visible UI is the app surface. Five detachable windows (main, equalizer, playlist, settings, about) with Winamp-style docking among main/EQ/PL. Settings and about are freestanding (not snappable, not in the main drag cohort). Main player and equalizer never stretch; on-screen size follows the global zoom step only. The playlist window may be freely resized. Main title bar carries logo + wordmark; EQ/playlist/settings/about title bars show role title only. EQ band faders use a spectrum-gradient value fill.
_Avoid_: borderless (alone), frameless window (implementation jargon in product talk), Scalable UI (retired as a whole-chrome free-resize mode), stretching the main or EQ canvas, single-window EQ/PL swap (retired product model)

**Mockup chrome** / **code-constructed chrome**:
The built-in look for main, equalizer, and playlist — painted in Flutter from the recipe in `player-mockup-2.html` (tokens, geometry, gradients, type, icon paths). No PNG panel faces or nine-slice graphite pack on the product path.
_Avoid_: graphite skin (as the *current* look), PNG skin, coded chrome (vague), vector chrome (alone), theme (when meaning this construction), Classic skin (Winamp WSZ — different thing)

**Graphite skin** *(historical)*:
The retired PNG-first chrome look (panel faces under `assets/skin/graphite/`). Kept only as history; must not steer new work. Superseded by mockup chrome / [ADR 0007](docs/adr/0007-code-constructed-mockup-chrome.md).
_Avoid_: using this term for the current product look

**Session host**:
The single Flutter process that owns shared controllers (playback, playlist, EQ, zoom, settings) and the docking coordinator; the five OS windows are views onto that session.
_Avoid_: multi-process, separate apps per window

**Docking** / **dock group**:
Winamp-style edge snap between windows. Dragging the **main** title bar moves every **visible** EQ/playlist window (settings/about excluded; snap state irrelevant). Dragging EQ or playlist moves only that window and peels its dock edges; snap runs only on EQ/PL drag end. Settings and about never snap and are never snap targets. EQ may snap to any side; playlist snaps top/bottom only (plus optional left/right flush when already within threshold). Undock via peel, break-threshold, and/or Shift. Main minimize may hide/restore visible secondaries (including settings/about) when the preference is on; always-on-top applies to visible tramp windows. Settings stays raised above other Tramp windows. On Windows, only main appears in the taskbar.
_Avoid_: tiling WM, snap layouts (OS), tabs; assuming dock edges gate main’s group move; docking settings/about to main/EQ/PL

**Playlist**:
An ordered list of playable tracks the user can manage (add, remove, reorder, play from).
_Avoid_: queue (unless a separate now-playing queue is later distinguished), library (the collection of known media)

**Playlist file**:
A saved playlist on disk (v1: M3U/M3U8) that Tramp can open and write. Its track lines are **hints, not addresses**: the same album is `\\server\share` on Windows and a mount point on Linux, and a mount point that moves leaves every absolute line stale, so a line that lands on nothing is re-read against the folder the playlist itself sits in. Tramp resolves on load and never rewrites the listener's file to suit this machine — that would break it for the machine that wrote it ([ADR 0008](docs/adr/0008-playlist-collection-stores-references.md)).
_Avoid_: playlist document, playlist export (unless meaning a one-way share), fixing / correcting a playlist file (Tramp reinterprets, it does not edit)

**Playlist collection**:
The set of playlists Tramp keeps for the listener — *references* to playlist files at the paths the listener chose, never copies. Shown in the Playlist Manager's left panel.
_Avoid_: library (deliberately unspent — see Library), playlist library, collection (alone, where a track collection could be meant)

**Saved playlist**:
One entry in the playlist collection: a display name plus a reference to a playlist file. The name may differ from the filename; renaming an entry never renames the file.
_Avoid_: playlist entry, collection item, imported playlist (nothing is imported — the file stays where it is)

**Current playlist**:
The one playlist loaded for playback, shown in the Playlist Manager's right panel; what the transport plays from. May come from a saved playlist, from an arbitrary playlist file, or from tracks added ad hoc.
_Avoid_: active playlist, now playing (that is the transport's track, not this list), queue

**Altered current playlist**:
A current playlist whose track list has changed since it was loaded or last saved — tracks added, removed, reordered, sorted, or cleared. Navigating to another saved playlist asks the listener before discarding it. Only writing the whole list to a file makes it unaltered again.
_Avoid_: dirty (implementation jargon in product talk), modified playlist, unsaved playlist (a current playlist can be unsaved without being altered)

**Disabled playlist**:
A saved playlist whose file was missing at the last validation pass. It cannot be loaded and can only be removed from the collection; if the file comes back, it enables itself again.
_Avoid_: broken playlist, orphaned playlist, deleted playlist (the entry survives — the file is what is gone)

**Track**:
A single local audio file Tramp can play; v1 supported kinds are MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, and Opus.
_Avoid_: song (when meaning the file), media item, clip

**Unplayable track**:
A track the transport was told to play and the engine then refused — the file has moved, the share dropped, the codec is absent. The transport stops reading as playing rather than sitting silently on it, and says which track and what the engine said. The listener's counterpart concept for a whole playlist is a **disabled playlist**.
_Avoid_: missing track (the file may be present and still unplayable), broken track, dead track, failed playback (as a state name)

**Library** *(reserved — deliberately unused)*:
A persisted, browsable catalog of known tracks on disk, built by indexing designated folders. Out of scope for v1 — music enters via open, drag-and-drop, and playlists only. The word stays parked rather than repurposed: what Tramp stores is the **playlist collection**, which is not a track catalog.
_Avoid_: collection, media database (in v1 discussions); using "library" for the playlist collection

**Zoom step**:
One of the discrete scale factors (50%, 75%, 100%, 125%, 150%, 200%, 250%, 300%; default **75%**) applied globally to the three windows’ logical canvases. Persisted; steps that would not fit the display’s work area are disabled. Changes via main title-bar zoom-in / zoom-out (and matching menu or shortcut). Scales main/EQ canvases and the playlist’s stored logical size; does not replace playlist free resize.
_Avoid_: DPI scale (OS setting), continuous zoom, maximize (as a window-size control), per-window zoom (product model is global), stretching main/EQ via window drag

**Clutterbar**:
The vertical letter strip on the main player. Product letters: **O** (options), **A** (always-on-top for the visible docked group), **I** (track info). No D, no V.
_Avoid_: toolbar (when this strip is meant), doublesize button, viz button

**Phosphor**:
The **cyan** colour of the mockup chrome’s “screen glow” — used for lit LCD text, spectrum bars, and other live readouts (`#3de7ff` / hot variants). Not chartreuse.
_Avoid_: neon green, lime, acid chartreuse (retired graphite accent), LCD green (the older pure-green token)

**Rail**:
The cyan→magenta grip rails / brushed filler strips in the mockup chrome (title-bar grips and transport rail).
_Avoid_: rule, divider, separator (when meaning those accents); warm yellow rails (retired graphite title-bar accent)

**Well**:
A recessed inset surface — groove, slider track, or `.screen` glass — cut into the chrome.
_Avoid_: trough, channel, inset (alone)

**Windowshade**:
The collapsed equalizer or playlist state that shows only the title bar, matching the classic shade gesture. Docking layout uses shaded height.
_Avoid_: minimize (window chrome), collapse (alone when shade is meant)

**Display well**:
The large recessed LCD glass area on the main player that holds spectrum, track title, times, and format metadata.
_Avoid_: LCD (alone), screen, display (when the inset glass region is meant)

**Synthetic levels**:
`AudioLevels` frames marked `synthetic: true` — a hard-fail / development signal when real spectrum cannot be measured. Not the product end-state; normal play must use real analyser levels.
_Avoid_: treating synthetic levels as the shipped spectrum design, fake levels (pejorative), mock levels, placeholder spectrum (as a planned deliverable)

**Spin**:
One track played through to the end — the unit the About stats well counts.
_Avoid_: play (ambiguous with the transport verb), listen, stream

**Maker's plate**:
The brushed strip along the bottom of the About window carrying the company mark, name, copyright, and website — named after the plate riveted to real hardware.
_Avoid_: footer, credits bar, branding strip
