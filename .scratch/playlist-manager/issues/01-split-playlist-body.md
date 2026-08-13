# 01 — Split the playlist body into track-pane and footer modules

**What to build:** Nothing a listener can see. This is the prefactor that gives the
Playlist Manager's second panel somewhere to go. The playlist body is currently one
long file holding the list well, the rows, the footer, and four painters; the
two-panel redesign, multi-select, and drag-to-reorder all land on top of it.

Separate the track pane (well, rows, painters, scrollbar wiring) from the footer
(button strip, total readout, status line) so each can be worked on and tested
without the other. Behaviour must not change at all.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] The track pane and the footer live in separate modules from the playlist body
- [x] No rendered output changes — the existing playlist widget tests pass untouched
- [x] No public constructor or callback signature visible to the window changes
- [x] Row height and footer height remain the single source of truth they are today, not duplicated across the new modules
- [x] The analyzer reports no new issues
- [x] Test count does not drop; no test is weakened or skipped to accommodate the split

## Comments

Verified rendering is untouched rather than merely "tests still pass": the two
playlist goldens fail by exactly the same margin as before the split —
`playlist_window` at 5.11% / 29336px and `playlist_window_shade` at 1.48% /
512px, to the pixel. Those failures are the known Windows-authored golden
mismatch, unrelated to this work.

Suite is 393 passing / 8 failing before and after, and the analyzer reports the
same 23 pre-existing issues with none in the playlist files.

Row height was previously the literal `37` in three places — the list item
extent, the row box, and the stripe painter — which is now
`MockupPlaylistTrackPane.rowHeight`. Footer height was the literal `110`
alongside separate `74` / `10` / `26` literals, and is now derived from them.

The playlist body went from 855 lines to 128; the track pane is 362 and the
footer 400.
