# 14 — Regenerate the playlist goldens on Windows

**What to build:** Bring the golden set back in line with the shipped Playlist
Manager, and remove the host guard added in ticket 02.

This repo's golden set is authored on Windows. Rebaselining on Linux would fold host
font rasterisation into the references, which is why the playlist goldens were guarded
rather than regenerated while the overhaul was in progress.

**This ticket needs a Windows host.** It cannot be completed by an agent on the Linux
development machine, which is why it is marked for a human rather than for an agent.

**Blocked by:** 02, 03, 05, 08, 09, 10, 11

**Status:** ready-for-human

- [ ] Playlist goldens regenerated on Windows against the finished Playlist Manager
- [ ] Both the expanded two-panel window and the windowshade state are covered
- [ ] A collection with entries, an empty collection, and a disabled entry are each covered by a golden or an explicit note saying why not
- [ ] The host guard from ticket 02 is removed
- [ ] The full suite passes on Windows with no golden failures
- [ ] Any remaining Linux-side failures are confirmed to be font rasterisation only, and that expectation is recorded

## Comments

Tickets 01–13 are all done; this is the only one left, and it is blocked on hardware
rather than on work.

### What the playlist goldens should now show

The Playlist Manager, not the Playlist Editor. Two panels with a draggable divider,
the collection panel on the left carrying a header, rows with a name and track count,
and its own control strip (add, remove, rename, and a menu holding the two create
actions). The window's default canvas is now **1073×696**, not 825×696, and its 75%
native seed is **805×522**.

Worth covering deliberately, since each is a distinct rendering: an empty collection
(the `NO SAVED PLAYLISTS` state), a collection with rows, a disabled row, and a
collapsed panel with its reopen tab.

### The trap

`about_window_golden_test.dart` is now the **only** golden that depends on stats
being passed in explicitly. Ticket 12 made the widget default zeros so nothing can
ship invented figures by forgetting to pass real ones, and the test passes
`AboutStats.placeholder` to hold the image steady. If that golden is ever
regenerated, it must keep doing so, or the reference becomes a well of zeros.

### Also for whoever picks this up

- The playlist goldens are currently host-guarded (ticket 02). Removing the guard is
  part of this ticket.
- The chrome and main-player goldens fail on Linux at margins that have not moved
  across the whole overhaul — `title_bar_strip` 9.98%/31134px,
  `main_player_window` 8.55%/24560px, `main_player_window_quiet` 7.16%/20550px,
  `screen_well` 0.48%/1013px, `button_off_on` 0.41%/626px, `shell_plate_rail`
  0.12%/403px. Three of those still show the `1.0` version pill that commit 9a8bc5c
  removed, so they want regenerating on the same pass.
- `tool/measure_quit_latency.sh` was never run against this work. Ticket 13 added a
  second small file write to the quit path (the spin count, beside the resume state
  quit already wrote), and slow quit is a filed release blocker, so one run before
  release is worth it.
