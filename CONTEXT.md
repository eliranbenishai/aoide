# Tramp

A multi-platform desktop music player positioned as a spiritual successor to Winamp.

## Language

**Tramp**:
The product — a desktop music player that can be built for Windows, Linux, and macOS.
_Avoid_: Winamp clone, media player (when meaning this product)

**Classic skin**:
A Winamp-compatible skin that replaces the built-in graphite chrome with the skin's own layout and art.
_Avoid_: theme (when meaning a classic Winamp skin), skin pack (unless referring to a collection)

**App chrome**:
Tramp's own window decoration — no OS title bar or standard window frame; the visible UI is the app surface. Main player and equalizer never stretch; in equalizer mode window size follows the zoom step only. In playlist mode the window may be freely resized so the playlist well can grow.
_Avoid_: borderless (alone), frameless window (implementation jargon in product talk), Scalable UI (retired as a whole-chrome free-resize mode), stretching the main or EQ canvas

**Graphite skin**:
The built-in chrome’s look for the main player, equalizer, and playlist panels — delivered primarily as PNG assets (panel faces, button faces including pressed/active, slider grooves, and other static chrome; SVG only where already useful, e.g. brand marks). Code supplies hit targets, which skin asset is shown, thumb position, spectrum visualization, and all LCD/playlist text.
_Avoid_: coded chrome, vector chrome (as the primary look), theme (when meaning this asset pack), Classic skin (Winamp WSZ — different thing)

**Playlist**:
An ordered list of playable tracks the user can manage (add, remove, reorder, play from).
_Avoid_: queue (unless a separate now-playing queue is later distinguished), library (the collection of known media)

**Playlist file**:
A saved playlist on disk (v1: M3U/M3U8) that Tramp can open and write.
_Avoid_: playlist document, playlist export (unless meaning a one-way share)

**Track**:
A single local audio file Tramp can play; v1 supported kinds are MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, and Opus.
_Avoid_: song (when meaning the file), media item, clip

**Library**:
A persisted, browsable catalog of known tracks on disk. Out of scope for v1 — music enters via open, drag-and-drop, and playlists only.
_Avoid_: collection, media database (in v1 discussions)

**Zoom step**:
One of the discrete scale factors (100%, 125%, 150%, 200%, 250%, 300%) applied to the fixed logical panel canvases. Persisted; steps that would not fit the display’s work area are disabled. Changes via title-bar zoom-in / zoom-out (and matching menu or shortcut). Scales main/EQ canvases; does not replace playlist-mode free resize of the window around those canvases. Graphite skin PNGs are authored at one high master density (2×) and scaled with the step.
_Avoid_: DPI scale (OS setting), continuous zoom, maximize (as a window-size control), per-step asset exports (v1), stretching main/EQ via window drag

**Lower region**:
The panel stacked under the main player — either the equalizer or the playlist, switched by the EQ/PL toggles.
_Avoid_: bottom pane, sidebar, dock

**Phosphor**:
The acid chartreuse colour of the graphite chrome’s “screen glow” — used for lit LCD text, spectrum bars, and other live readouts drawn in code.
_Avoid_: neon green, lime (alone), LCD green (the older pure-green token)

**Rail**:
The warm yellow double lines that flank the title-bar wordmark on each panel.
_Avoid_: rule, divider, separator (when meaning the title-bar accent)

**Well**:
A recessed inset surface — groove or slider track — cut into the graphite panel face.
_Avoid_: trough, channel, inset (alone)

**Windowshade**:
The collapsed equalizer state that shows only the title bar, matching the classic shade gesture.
_Avoid_: minimize (window chrome), collapse (alone when the EQ shade is meant)

**Display well**:
The large recessed LCD glass area on the main player that holds spectrum, track title, times, and format metadata.
_Avoid_: LCD (alone), screen, display (when the inset glass region is meant)

**Synthetic levels**:
`AudioLevels` frames marked `synthetic: true` — plausible analyser data produced because the shipped media_kit/libmpv path cannot measure real spectrum. Honest contract, not a debug flag.
_Avoid_: fake levels (pejorative), mock levels, placeholder spectrum
