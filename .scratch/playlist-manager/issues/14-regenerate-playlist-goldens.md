# 14 — Regenerate the playlist goldens on Linux

**What to build:** Bring the golden set back in line with the shipped Playlist
Manager, and remove the host guard added in ticket 02.

This ticket was written expecting a Windows host, because the golden set was authored
there and rebaselining on Linux would have folded a second host's font rasterisation
into the references. That premise is gone: Linux is now the only development machine,
and Windows is used to test builds in a VM and nothing else. Linux is therefore the
reference platform, which unblocks this ticket on the machine it sits on.

**Blocked by:** 02, 03, 05, 08, 09, 10, 11

**Status:** done

- [x] Playlist goldens regenerated on Linux against the finished Playlist Manager
- [x] Both the expanded two-panel window and the windowshade state are covered
- [x] A collection with entries, an empty collection, and a disabled entry are each covered by a golden or an explicit note saying why not
- [x] The host guard from ticket 02 is removed
- [x] The full suite passes on Linux with no golden failures
- [x] The reference platform is recorded, so a future Windows-side failure reads as rasterisation rather than as a defect

## Comments

Tickets 01–13 are all done; this is the only one left, and it is blocked on hardware
rather than on work.

### Done — Linux is the reference platform

The blocker dissolved rather than being cleared: development is Linux-only now, so the
goldens were rebaselined here and the guard is gone. The suite is green with **no**
skips for the first time — 675 passing.

Five playlist goldens, one per rendering the ticket asked for:
`playlist_window` (two panels, three collection rows, the loaded one lit),
`playlist_window_shade`, `playlist_window_empty_collection` (`NO SAVED PLAYLISTS`),
`playlist_window_disabled_entry` (missing-mark and dimmed contents, figures intact),
and `playlist_window_collection_collapsed` (reopen tab only, window rendering as one).
The test now pumps at `TrampMetrics.playlistDefault` (1073×696) rather than the old
825×696, through one shared `pumpGolden` helper so the states differ only by the
collection props they pass.

The six stale chrome and main-player goldens were rebaselined on the same pass, which
also cleared the `1.0` version pill this ticket noted they still carried. The About
golden was regenerated for a separate reason — the copyright line now reads
`© 2026 Free Forever` from the new `trampFreePromise`, so the plate no longer repeats
the company name set above it in chrome type. The trap this ticket flagged still
holds: `about_window_golden_test.dart` continues to pass `AboutStats.placeholder`
explicitly, so the well is not a well of zeros.

The `tool/measure_quit_latency.sh` run this ticket asked for is now done: **72–79ms**
across five runs on a fresh Linux release build, against a 500ms budget. Recorded in
[ticket 13](13-count-spins.md) and the
[quit-latency ticket](../../quit-latency/issues/01-fast-main-quit.md). Nothing is
left outstanding here.

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
