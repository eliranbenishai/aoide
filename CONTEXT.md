# Tramp

A multi-platform desktop music player positioned as a spiritual successor to Winamp.

## Language

**Tramp**:
The product — a desktop music player that can be built for Windows, Linux, and macOS.
_Avoid_: Winamp clone, media player (when meaning this product)

**Classic skin**:
A Winamp-compatible skin that fixes the player's chrome to the skin's layout; the window is not freely resizable in this mode.
_Avoid_: theme (when meaning a classic Winamp skin), skin pack (unless referring to a collection)

**Scalable UI**:
The modern, freely resizable and scalable interface mode — used when not running under a classic skin.
_Avoid_: modern skin, freestyle UI

**App chrome**:
Tramp's own window decoration — no OS title bar or standard window frame; the visible UI is the app surface, with edges that still resize the window.
_Avoid_: borderless (alone — resizing must remain), frameless window (implementation jargon in product talk)

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
