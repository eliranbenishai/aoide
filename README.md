<div align="center">

<img src="assets/branding/logo.png" alt="" width="120">

# Aoide

### The Muse of Old Made New

**A Winamp-inspired music player for the files you already have.**<br>
Free, open source, for Windows, Linux and macOS — open a folder and press play.

[**Download**](https://aoide.music) · [What's new](https://github.com/eliranbenishai/aoide/releases/latest) · [Report a bug](https://github.com/eliranbenishai/aoide/issues)

[![Latest release](https://img.shields.io/github/v/release/eliranbenishai/aoide?label=release)](https://github.com/eliranbenishai/aoide/releases/latest)
[![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)
![Windows, Linux, macOS](https://img.shields.io/badge/platforms-Windows%20%C2%B7%20Linux%20%C2%B7%20macOS-lightgrey)

</div>

---

We loved Winamp. Who didn't? It had a glorious run, but it's been decades and
the world has changed. So we made Aoide (pronounced *I-O-D*): a player for music
that lives on your own disk, with a playlist manager that lets you keep YOUR
music the way YOU want it.

There is no library to import, no account to make, and nothing phones home.
Simple but powerful, beautiful but approachable. It's open source but doesn't
look like it. And it's free. Forever.

## Download

Get it from **[aoide.music](https://aoide.music)** — pick your machine and start
listening. No sign-up, no trial, nothing to unlock later.

| Platform | You get | Worth knowing |
|---|---|---|
| **Windows** | Installer (`.exe`) | Windows may say it doesn't recognise the app the first time — choose **More info**, then **Run anyway**. The installer takes care of the rest. |
| **Linux** | AppImage, or a portable `.tar.gz` | The AppImage is one file with everything inside it: mark it executable and it runs. The tarball is the same player as a folder you can keep anywhere, including a USB stick. |
| **macOS** | Disk image (`.dmg`) | One universal build for Apple silicon and Intel, signed and notarised, so macOS will not refuse it. Needs macOS 13 or later. |

Every file is published with the SHA-256 it carried when it was uploaded, so you
can check that the copy on your disk is the one we published.

## The face

<div align="center">
<img src="https://aoide.music/screenshots/1.0/main_player_window.png" alt="The main Aoide player: transport controls, track details and a live spectrum" width="720">
</div>

*Big clear controls, your track's details, and a live spectrum.*

<div align="center">
<img src="https://aoide.music/screenshots/1.0/playlist_window.png" alt="The playlist manager: saved playlists on the left, the songs inside them on the right" width="720">
</div>

*Your playlists, beside the songs inside them.*

<div align="center">
<img src="https://aoide.music/screenshots/1.0/equalizer_window.png" alt="The equaliser: ten bands, a preamp and a response curve" width="720">
</div>

*Ten bands, a shelf of presets, and the curve you're making.*

## What it does

### Your playlists, your way

Drag tracks in, put them in the order you want, and save the playlist as M3U or
M3U8. Pick a handful of files and save them as a playlist of their own; it takes
over without cutting off the track you're on. Or add playlists you already have
to the playlist manager. Reopen Aoide and it picks up where you left off.

Aoide keeps *references* to your files — it never copies your music into a
library of its own, and it won't quietly rewrite a playlist you wrote to suit
this machine. Your file changes when you save it, and not before.

### Plays what you already have

Double-click a song, drop a whole folder on the window, or send it over from any
disk, USB or network drive. MP3, AAC and M4A, FLAC, WAV, Ogg Vorbis and Opus all
play like butter on toast. Aoide reads the artist and album already written into
your tracks, and leaves them alone.

### Music to your ears

A ten-band equaliser with presets, just like old times — and the preamp does
come in handy. There's a MONO button for those of us who sometimes listen with
only one ear, a twenty-bar spectrum that follows what's playing, shuffle and
repeat, and your pick of output device.

### It draws its own face

Aoide draws its own window rather than borrowing your desktop's. The playlist
and the equaliser dock to the player the way they used to, and collapse to their
title bars when you want them out of the way. The whole thing scales to
whichever of the six zoom steps suits your screen.

## Skins

<div align="center">
<img src="https://aoide.music/screenshots/1.0/skins_window.png" alt="The skins panel: a grid of skin previews" width="720">
</div>

*Pick a face, or bring one in from a folder or a zip.*

Eight skins come with Aoide. It takes more from a folder or a zip — no store, no
account, no converter — and you can bring your own font while you're at it.

## Bugs and wishes

If Aoide misbehaves, or there's something you wish it did, we want to hear about
it. Two ways in — pick whichever suits you.

- **In the open:** [open an issue](https://github.com/eliranbenishai/aoide/issues).
  Have a quick look for an existing one first. Everything is public, so you can
  watch a report go from filed to fixed.
- **Or quietly:** write to <support@proximamagnifica.com>. Tell us what you did,
  what happened, and what you expected — that's usually enough to get started.

## Your music stays yours

The player stays on your computer. Nothing you play, open, or change is sent to
us: no telemetry, no analytics, no crash reporting, no account. The full
[privacy policy](https://aoide.music/privacy) says it at length, but that's the
whole of it.

## Building from source

Aoide is Qt 6 (QWidget + QPainter) in [`src/`](src/). Build instructions,
platform notes and the CI story are in
[`docs/development.md`](docs/development.md).

## License

Copyright (C) 2026 Proxima Magnifica

Aoide is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Aoide is distributed in the hope that it is useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

The full license is in [`LICENSE`](LICENSE). Other works shipped with Aoide are
listed in [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

The name Aoide, the maker's plate, Proxima Magnifica, and aoide.music are
trademarks; the GPL does not grant trademark rights.
