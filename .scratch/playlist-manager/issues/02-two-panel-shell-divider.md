# 02 — Two-panel shell with a draggable divider

**What to build:** The window becomes the **Playlist Manager** and gains its second
panel. The listener sees the collection panel on the left and the current playlist
on the right, with a divider they can drag to give room to either side. They can
collapse the collection panel entirely and reopen it at the width they last used,
and wherever they leave the divider is where it sits after a restart.

The collection panel shows its **empty state** in this ticket — the real state a
listener sees before they have added anything. Storing and listing saved playlists
is ticket 03.

Also guard the playlist goldens by host, so the suite runs green through the rest of
this overhaul instead of carrying failures everyone learns to ignore. Regenerating
them is ticket 14.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] The window's title reads "Playlist Manager"
- [ ] Collection panel on the left, current playlist on the right, divider between them
- [ ] Dragging the divider resizes both panels live
- [ ] The divider position persists across a restart, as a logical width so zoom does not distort it
- [ ] The collection panel collapses, and reopening restores the width it had before collapsing
- [ ] The window's minimum width rises while the panel is shown and returns to today's minimum when collapsed; footer controls never overflow at the minimum
- [ ] Resizing the window holds the collection panel's width and gives all slack to the track list
- [ ] Global zoom still scales the window exactly as it does today
- [ ] The empty collection panel tells the listener how to add a playlist
- [ ] Playlist goldens are guarded by host, with the reason recorded where the guard lives, and the suite reports no failures on Linux
