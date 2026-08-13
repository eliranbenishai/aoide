# 01 — Split the playlist body into track-pane and footer modules

**What to build:** Nothing a listener can see. This is the prefactor that gives the
Playlist Manager's second panel somewhere to go. The playlist body is currently one
long file holding the list well, the rows, the footer, and four painters; the
two-panel redesign, multi-select, and drag-to-reorder all land on top of it.

Separate the track pane (well, rows, painters, scrollbar wiring) from the footer
(button strip, total readout, status line) so each can be worked on and tested
without the other. Behaviour must not change at all.

**Blocked by:** None — can start immediately.

**Status:** ready-for-agent

- [ ] The track pane and the footer live in separate modules from the playlist body
- [ ] No rendered output changes — the existing playlist widget tests pass untouched
- [ ] No public constructor or callback signature visible to the window changes
- [ ] Row height and footer height remain the single source of truth they are today, not duplicated across the new modules
- [ ] The analyzer reports no new issues
- [ ] Test count does not drop; no test is weakened or skipped to accommodate the split
