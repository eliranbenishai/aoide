# 07 — Create a new playlist from every track in the current playlist

**What to build:** The listener has built up a pile of tracks by dropping files in
and wants to keep it. They create a playlist from the current playlist, choose where
it goes, and it becomes a **saved playlist** in their collection immediately — no
separate "now add it" step.

Because the whole current track list has been written to a file that becomes its
origin, the current playlist stops being altered.

**Blocked by:** 03, 05

**Status:** done

- [x] Creating a playlist from the current playlist asks the listener where to save it
- [x] The playlist file is written at the listener's chosen location
- [x] A reference to that file joins the collection automatically, with no extra step
- [x] The new entry appears in the collection panel straight away
- [x] The current playlist's origin becomes the new file
- [x] The current playlist is no longer altered afterwards
- [x] Cancelling the save dialog changes nothing — no entry, no file, altered state untouched
- [x] Saving over a file already in the collection updates that entry rather than adding a second one
- [x] Creating from an empty current playlist is either refused or writes an empty playlist consistently, not half of each

## Comments

Delivered with ticket 08 in one pass (they share `mockup_playlist_test.dart`).
Suite went from 519 passing / 2 skipped / 6 failing to **555 passing / 2 skipped
/ 6 failing** — 36 new tests across both tickets, no regressions. The 6 are the
same Windows-authored goldens failing by **exactly** the same margins
(`title_bar_strip` 9.98% / 31134px, `main_player_window` 8.55% / 24560px,
`main_player_window_quiet` 7.16% / 20550px, `screen_well` 0.48% / 1013px,
`button_off_on` 0.41% / 626px, `shell_plate_rail` 0.12% / 403px). The 2 skips
are still the host-guarded playlist goldens. `flutter analyze` reports the same
**23** pre-existing info issues, none in a file this ticket touched.

### One command, because the two halves are ordered

`CreatePlaylistFromCurrentCommand(path)`. Not a save followed by an add: the
reference can only be counted once the file exists, and two fire-and-forget
commands on this bus give no way to say so. The host handler is the whole
feature in two lines —

```dart
await _playlist.savePlaylistFile(path);   // whole-list write: lowers altered,
await _collection.addWritten(path);       // adopts the origin, forgets the keep
```

— which is exactly what tickets 05 and 06 asked for. Routing around
`savePlaylistFile` would have left a freshly saved playlist reading as altered
*and* coming back altered after a restart, since that method is also where the
kept list is forgotten.

Ticket 09 gets an obvious sibling here rather than an extension: a
`CreatePlaylistFromSelectionCommand` that writes the *selected* tracks and calls
`_collection.addWritten` **without** touching `PlaylistController` at all. Do
not add a `fromSelection:` flag to this command — the two differ in the one
thing that matters (whether the current playlist's altered state moves), and a
flag would put that difference inside a branch instead of in the type.

### `addWritten`, and why `add` could not just be reused

`PlaylistCollectionController.add` deliberately leaves an existing entry's
figures alone, because a re-add has no reason to think the file moved — a moved
mtime belongs to `validateReferences`. That is wrong for a file **Tramp just
wrote**: the cached count and duration are known stale the instant the write
lands, and the panel would paint the old number until the next validation pass.

So both now run through one private `_keepReference(path, rewritten:)`. Same
stat, same figures read, same companion track-set write, same sort, same
highlight — one code path, one behaviour difference: `rewritten` updates an
entry in place instead of returning it untouched. A saved-over entry **keeps
the name the listener gave it** (they saved over a playlist, they did not rename
it), and because the fresh mtime is stored with the fresh figures, the next
validation pass reads no external edit and leaves the entry exactly as this left
it — pinned by a test.

### The empty-playlist decision: **refused**

An empty current playlist cannot be kept. The panel's create control is
disabled while there are no tracks, so the save dialog never opens and no
command is emitted; the host also returns early on an empty list, so the refusal
holds even if a future caller reaches the command another way. Consistent at
both ends, which is what the criterion asks for.

Why refused rather than writing an empty playlist:

- A reference to a file with nothing in it is a row reading `0` that loads
  nothing, counts nothing toward the About figures, and cannot be told apart
  from a playlist whose file went empty by accident.
- Creating is the listener saying "keep this pile". There is no pile.
- It would have a side effect out of all proportion to the gesture: the write
  goes through `savePlaylistFile`, so creating from a *cleared* playlist would
  quietly lower its altered state and adopt an origin — the exact protection
  ticket 05 built, spent on nothing.

Disabling rather than hiding is deliberate: the control stays where it is, in
the disabled treatment every other control in this chrome uses, so the listener
learns where it lives before they have anything to put in it.

The one consequence worth noting is that the collection pane now sits under a
`ListenableBuilder` on the current playlist as well as the collection props —
otherwise the control would stay live for a beat after the last track left,
until the host echoed a snapshot back. A test drops a track into the window's
own mirror and finds the control awake before any snapshot arrives.

### Where it lives

`pl-collection-create`, between `pl-collection-add` and `pl-collection-remove`
in the panel's own control strip — collection-level actions, kept apart from the
footer's track-level add / remove exactly as the spec requires. Semantic label
"Create playlist from current playlist". Three 30px-wide controls with 6px
between them come to 102px, inside the 162px the narrowest panel leaves (180px
less 18px of padding), and a test pins that at `playlistMinWithCollection`.

The glyph (`PlaylistCollectionCreateMark`) is drawn in the collection pane file
in the same stroked idiom as the chevron and the missing mark, rather than added
to `MockupIcons`: the mockup has no glyph for an action the window did not have.

`pickSavePlaylistPath` was already injected into `PlaylistWindow` by ticket 05,
so the save dialog stayed out of the widget and no client change was needed.

### What was verified, and how

Collection seam (6 tests): a written file becomes an entry with real figures and
a track set; **saving over a kept playlist updates that entry and leaves one
row**, with count, duration and track set all moved on; the refreshed stamp
means a following `validateReferences` changes nothing; a saved-over entry keeps
its name override; saving over a file that had gone missing re-enables its
entry; an unreadable file is reported rather than kept.

Widget seam (5 tests): the control opens the picker and emits exactly one
`CreatePlaylistFromCurrentCommand` with the chosen path; **cancelling emits
nothing at all** and leaves the current playlist and collection on screen; an
empty current playlist opens no picker and emits nothing; the control wakes as
soon as a track lands; and it is its own control at the narrowest panel width.

Message seam: round-trip both ways including a Windows path, and an empty path
rejected alongside the other saved-playlist commands.

Mutation check, to show they bite:

- making `addWritten` take `add`'s existing-entry path fails the three
  saving-over tests and only those;
- letting the create flow emit when the picker returns null fails *cancelling
  the save dialog changes nothing at all* and only that.

Both reverted.

### What was NOT verified

The host handler itself (`_handleCreatePlaylistFromCurrent`) is not covered by a
test, for the standing reason: `SessionHostApp` is bound to the multi-window and
window-manager plugins and no test can pump it. Both halves it calls are covered
at their own seams — `savePlaylistFile` lowering the state and forgetting the
keep (tickets 05 and 06), `addWritten` here — and the handler is two awaited
calls in that order.

### Notes for later tickets

- **Ticket 09 (create from selection)** — sibling command, as above. It must
  **not** touch `PlaylistController`, and gets that for free. `addWritten` is
  the right call for it too: a selection written to a path already in the
  collection has the same stale-figures problem. Its control belongs beside
  `pl-collection-create` in the same strip; if a third button crowds the
  narrowest panel, the two creates fold into one menu button rather than moving
  to the footer, which is track-level by definition.
- **Ticket 10 (drag reorder)** — nothing here constrains it. Note the create
  control is disabled purely on `tracks.isEmpty`, so a reorder never touches it.
- The panel's control strip is now the place collection-level actions go.
  Rename (ticket 11) belongs there or on the row, not in the footer.
