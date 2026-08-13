# 10 — Drag tracks to reorder

**What to build:** The listener sets the running order by hand, dragging a track to
where they want it. The playlist controller already has a tested move operation that
is wired to nothing, so this is mostly a matter of giving it a gesture and a path
through to the host.

Reordering is a change to the track list, so it marks the current playlist altered
and gets the same protection as tracks the listener added.

**Blocked by:** 01, 05

**Status:** ready-for-agent

- [ ] Dragging a track to a new position moves it there, and the rest close up around it
- [ ] The drag shows the listener where the track will land before they release
- [ ] Reordering marks the current playlist altered
- [ ] The playing track keeps playing across a reorder, and the playing row stays marked as the same track
- [ ] Reordering reaches the host as a command and comes back in the playlist snapshot, so both windows agree
- [ ] Dragging a row to where it already was is a no-op and does not mark the playlist altered
- [ ] Reorder and the custom scrollbar do not fight each other; a drag near the list edge behaves sanely
