# 07 — Create a new playlist from every track in the current playlist

**What to build:** The listener has built up a pile of tracks by dropping files in
and wants to keep it. They create a playlist from the current playlist, choose where
it goes, and it becomes a **saved playlist** in their collection immediately — no
separate "now add it" step.

Because the whole current track list has been written to a file that becomes its
origin, the current playlist stops being altered.

**Blocked by:** 03, 05

**Status:** ready-for-agent

- [ ] Creating a playlist from the current playlist asks the listener where to save it
- [ ] The playlist file is written at the listener's chosen location
- [ ] A reference to that file joins the collection automatically, with no extra step
- [ ] The new entry appears in the collection panel straight away
- [ ] The current playlist's origin becomes the new file
- [ ] The current playlist is no longer altered afterwards
- [ ] Cancelling the save dialog changes nothing — no entry, no file, altered state untouched
- [ ] Saving over a file already in the collection updates that entry rather than adding a second one
- [ ] Creating from an empty current playlist is either refused or writes an empty playlist consistently, not half of each
