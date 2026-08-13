# 05 — Altered current playlist and the confirmation dialog

**What to build:** Tramp starts noticing when the listener has changed the current
playlist, and stops throwing that work away silently.

Changing the track list — adding, removing, reordering, sorting, clearing — makes it
an **altered current playlist**. Merely loading a saved playlist does not; that sets
the baseline. When the listener clicks another saved playlist while the current one
is altered, Tramp asks first, and offers to save rather than only to discard.

The prompt has to stay meaningful, so it must never appear when nothing was changed.

**Blocked by:** 03

**Status:** done

- [x] Adding, removing, reordering, sorting, or clearing tracks marks the current playlist altered
- [x] Loading a saved playlist leaves it unaltered, however many times it is loaded
- [x] Navigating to another saved playlist while altered presents save, discard, and cancel
- [x] Cancel holds default focus, so an idle keypress cannot discard anything
- [x] Discard loads the new playlist and clears the altered state
- [x] Save writes straight to the current playlist's origin when it has one, then loads the new playlist
- [x] Save with no origin opens the save dialog; cancelling that dialog returns to the current playlist still altered, and does not fall through to loading
- [x] The altered state clears only when the whole current track list has been written to a file that becomes its origin
- [x] No prompt appears when the current playlist is unaltered
- [x] Altered state lives with the current playlist on the host side and rides the playlist snapshot, since the window rendering it is a separate engine

## Comments

Suite went from 470 passing / 2 skipped / 6 failing to **496 passing / 2 skipped /
6 failing** — 26 new tests, no regressions. The 6 are the same Windows-authored
goldens, failing by **exactly** the same margins as before (`title_bar_strip`
9.98% / 31134px, `main_player_window` 8.55% / 24560px, `main_player_window_quiet`
7.16% / 20550px, `screen_well` 0.48% / 1013px, `button_off_on` 0.41% / 626px,
`shell_plate_rail` 0.12% / 403px). The 2 skips are still the host-guarded
playlist goldens. `flutter analyze` reports the same **23** pre-existing info
issues, none in a file this ticket touched.

### Where the state lives, and what moves it

`PlaylistController.altered` (host side), riding `PlaylistSnapshotEvent.altered`.
There is deliberately **no setter**: "only a whole write lowers it" is then a
rule no caller can talk its way around, which matters because the tickets that
come next all want to touch this flag.

Raised by `addTracks`, `removeAt`, `removeSelected`, `move`, `sortBy`,
`reverseTracks`, `clear`. Lowered by `savePlaylistFile` only. `setTracks` sets a
fresh **baseline** rather than lowering — that one line is what makes
`openPlaylistFile`, `restoreLastPlaylist`, and a client applying a snapshot all
leave it down however often they run.

Two methods that look like mutations and are not:

- **`setTracks` must never raise.** It is what a load is made of *and* what the
  client applies every snapshot with, so raising there would raise the flag in
  the window's own mirror on every host broadcast.
- **`updateTrackByPath` must never raise.** It patches metadata from the
  background duration probe, which runs after *every* load — raising there would
  mark a freshly loaded playlist as changed within a second of loading it. It is
  not the track list, and the spec's list of mutations does not include it.

Selection (`select`, `setSelectedIndices`, `selectAll`, `invertSelection`) leaves
it alone for the same reason.

### Only a real change raises it

The ticket asks that the prompt never appear when nothing was changed, so the
raise is gated on the list actually moving, not merely on a method being called:

- `move` raises only when the insert index differs from the old one — dropping a
  row back where it was picked up is not a reorder (ticket 10 gets that for
  free).
- `sortBy` / `reverseTracks` compare the resulting list with the previous one
  (`listEquals`, and `Track` has value equality), so sorting an already-sorted
  list raises nothing.
- `removeSelected` raises only when the length actually dropped.
- `addTracks` / `removeAt` were already guarded against no-ops.

### The `clear()` decision

**`clear()` raises, but only when there were tracks to clear, and it keeps
dropping `sourcePath`.**

Raising is not optional — both the spec and `CONTEXT.md` list clearing as a
mutation — but clearing an *already empty* playlist changes nothing, and a
prompt about work that never existed makes every later prompt cheaper. So the
raise is gated on `tracks.isNotEmpty`, the same "only a real change" rule as
above.

Keeping the origin drop is the more interesting half. `clear()` means "start a
new, empty current playlist", not "empty the file I loaded" — which is also why
the host's `_onPlaylistChanged` already clears the collection highlight after a
clear, since nothing from the collection is loaded any more. The consequence is
deliberate: a cleared playlist is altered **with no origin**, so the
confirmation's save opens the save dialog instead of writing an empty list
straight over the file the listener loaded from. The alternative (keep the
origin) turns "save and load" after an accidental clear into a silent truncation
of a saved playlist, which is exactly the kind of loss this ticket exists to
prevent. Two tests pin it: *clearing tracks raises it* and *a clear leaves no
origin, so there is nowhere to save straight to*.

### The prompt

`showAlteredPlaylistDialog` in `lib/ui/playlist/altered_playlist_dialog.dart`,
shaped like `showLookConflictDialog`: an `AlertDialog` with everything through
`LookScope` / `TrampText` and no hardcoded palette colours, and the look read
from the *caller's* context before `showDialog` (the dialog route sits above the
window's `LookScope` — the same reason `mockup_settings.dart` reads it before
confirming Reset Settings). Keys: `pl-altered-dialog`, `pl-altered-cancel`,
`pl-altered-discard`, `pl-altered-save`. No existing key changed.

Cancel carries `autofocus: true`, so Return at an idle keyboard answers with the
harmless choice, and any other dismissal (barrier, Escape) also returns
`cancel` — the safe answer is the one that costs nothing.

`PlaylistWindow` owns the flow, intercepting the row tap that ticket 03 wired to
`LoadSavedPlaylistCommand`, and before ticket 04's `resolveForLoad` as that
ticket asked. A **disabled playlist** is still checked first, so its tap remains
a bare `SelectSavedPlaylistCommand` and the listener is never asked to protect
work in order to load something unloadable.

Save emits `PlaylistOpCommand('savePlaylist')` — straight to `sourcePath` when
there is one, otherwise to whatever the injected `pickSavePlaylistPath` answers
— and only then emits the load. Two ordered fire-and-forget commands is the
existing convention on this bus (`_dropPaths` already sends openPlaylist then
addPaths the same way).

### Altered is read from the snapshot *or* the window's own mirror

`PlaylistWindow` asks `widget.altered || widget.playlist.altered`. The prop is
the host's answer; the mirror catches the gap between a mutation made in this
window and the snapshot that confirms it, so a listener who removes a track and
immediately clicks another playlist is still asked. It cannot read stale-true,
because applying any snapshot goes through `setTracks`, which resets the mirror.

That made the widget test fixture's `addTracks` wrong, so it now uses
`setTracks`: in the client this mirror is *only* ever filled from a snapshot, and
a fixture that says otherwise would have hidden the mirror behind an
always-altered playlist. A test covers the gap directly (*a change the host has
not confirmed yet still asks*).

Nothing renders an altered *indicator* — the ticket does not ask for one, and no
golden moved as a result. If a later ticket wants one, this is the channel to
paint from.

### What was verified, and how

Most coverage is at the controller seam (15 tests): every mutating method raises;
`openPlaylistFile` leaves it down across three loads in a row; a load over an
altered playlist sets a fresh baseline; `restoreLastPlaylist` leaves it down;
`savePlaylistFile` lowers it and the next edit puts it straight back up;
selection and `updateTrackByPath` never raise; and one test walks the whole
no-op set (empty add, out-of-range remove, remove with nothing selected, a move
back to where it started, an already-sorted sort, clear and reverse on an empty
playlist) asserting the flag stays down.

The widget seam (9 tests) covers all four exits from a row tap: no prompt while
unaltered, the three choices while altered, cancel emitting nothing at all,
Return emitting nothing at all, discard emitting only the load, save-with-origin
emitting the save **then** the load in that order with the picker never opened,
save-with-no-origin opening the picker and emitting save then load, and the
cancelled save dialog. Message round-trip covers altered both ways plus a
snapshot from before the field decoding as unaltered.

**The cancelled save dialog** is the one the ticket called sharpest, so it is
tested as the listener would meet it: `pickSavePlaylistPath` returns null,
then the test asserts the picker really was opened, that **no** command of either
kind was emitted, that the current playlist is still on screen, and then clicks
the same row again and finds the dialog once more — proving the protection is
still up rather than merely that one click was quiet.

Mutation check on the three subtlest assertions, to show they bite:

- making `clear()` raise unconditionally fails *nothing changed means nothing
  raised*, and only that;
- removing `autofocus: true` from Cancel fails *an idle Return keypress cannot
  discard anything*, and only that — so the test is measuring the focus, not
  luck;
- letting the save branch fall through when the picker returns null fails
  *cancelling that save dialog keeps the current playlist, still altered*, and
  only that.

All three reverted, obviously.

### Anything a later ticket should know

- **Ticket 06 (survives restart)** needs to restore a list *and* raise the flag,
  and there is no setter to do it with. Add a purposeful method to the
  controller — `restoreAlteredTracks(tracks, sourcePath: …)` or similar — that
  sets the tracks and raises. It can only ever raise, so the "only a whole write
  lowers it" rule stays structural. Do **not** add a general `setAltered`, and do
  not make `setTracks` take an `altered:` flag: both hand every caller the power
  to lower it. Restoring through `setTracks` alone would silently restore the
  list as *unaltered*, which fails that ticket's second criterion. Metadata
  enrichment after the restore will not disturb the flag.
- **Ticket 07 (create from all tracks)** is a whole-list write, so it must go
  through `PlaylistController.savePlaylistFile` — that is the single place that
  lowers the flag, and routing around it would leave a saved playlist reading as
  altered. The confirmation's save already does exactly this.
- **Ticket 09 (create from selection)** must **not** lower it, and gets that for
  free by not touching `PlaylistController` at all: write the selection's file
  and add its reference through the collection module. If it ever needs a
  controller call, note that `savePlaylistFile` writes the *whole* list and
  adopts the path as the origin, so it is the wrong tool for a selection.
- **Ticket 10 (drag reorder)** wires `move`, which already raises — and already
  declines to raise when a row is dropped where it was picked up.
- The prompt only guards the **collection row** load. The footer's own load
  (`onLoadPlaylist` → `openPlaylist`) and a dropped playlist file still replace
  an altered current playlist silently, exactly as they did before this ticket.
  That is the ticket's scope as written ("navigating to another saved playlist"),
  and the wording in `_selectSavedPlaylist` is where a later ticket would widen
  it — `_saveWholeCurrentPlaylist` is already reusable for that.
- `PlaylistWindow.pickSavePlaylistPath` is injected the way `SkinsPanel`'s
  pickers are, so the save dialog stays out of the widget and the widget stays
  testable. The client passes the real `pickSavePlaylistPath`.
- `mockup_playlist_track_pane.dart` and `mockup_playlist_footer.dart` were **not**
  touched.
