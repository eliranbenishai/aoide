# 09 — Create a new playlist from the selected tracks

**What to build:** The listener pulls a shorter playlist out of a longer one — select
five tracks from a sixty-track playlist and keep them as their own **saved playlist**.

This is deliberately *not* the same operation as ticket 07. Only some of the current
tracks are being written, so the rest are still unsaved. Creating from a selection
must therefore leave the current playlist and its altered state completely alone, and
must not navigate anywhere — navigating would either discard the current playlist or
raise the very prompt this avoids. It creates the entry and highlights it.

**Blocked by:** 07, 08

**Status:** done

- [x] Creating from a selection writes a playlist file containing exactly the selected tracks, in their current order
- [x] A reference to that file joins the collection automatically
- [x] The new entry is highlighted in the collection panel
- [x] The current playlist's tracks are unchanged
- [x] The current playlist's origin is unchanged
- [x] The current playlist's altered state is unchanged — an altered playlist stays altered, an unaltered one stays unaltered
- [x] Nothing is loaded or navigated to; no confirmation prompt appears
- [x] The action is unavailable, or clearly inert, when no tracks are selected
- [x] Cancelling the save dialog changes nothing at all

## Comments

Delivered with tickets 10 and 11 in one pass (shared test file). Suite:
**595 passing / 2 skipped / 6 failing**, up from 555, with the 6
Windows-authored goldens failing at byte-identical margins (`title_bar_strip`
9.98%/31134px, `main_player_window` 8.55%/24560px, `main_player_window_quiet`
7.16%/20550px, `screen_well` 0.48%/1013px, `button_off_on` 0.41%/626px,
`shell_plate_rail` 0.12%/403px) and analyzer still at **23**, none in a touched
file.

### The altered state cannot move, because nothing here can reach it

`CreatePlaylistFromSelectionCommand` is a sibling of
`CreatePlaylistFromCurrentCommand`, not a flag on it, exactly as 07 asked. The
host handler is three lines and never mentions `PlaylistController` as anything
but a **reader**:

```dart
if (_playlist.selectedIndices.isEmpty) return;
await _collection.createFromSelection(
  path, _playlist.playlist.tracks, _playlist.selectedIndices);
```

`PlaylistCollectionController.createFromSelection` writes the file itself and
then calls `addWritten`, so the whole operation lives in the collection module.
`savePlaylistFile` — the one method that lowers `altered` — is not on the path
and could not be: the collection module holds no reference to the playlist
controller. That is what makes "an altered playlist stays altered" structural
rather than a branch someone has to remember. Pinned both ways at the widget
seam: *the current playlist and its altered state are untouched* (altered stays
up) and *an unaltered playlist is still unaltered afterwards*.

The command carries only the path. The **selection is not carried**: the host
holds the same selection the window is painting — it rides
`PlaylistSnapshotEvent` already — and reads it when the command lands. One
fewer thing that can disagree across the bus.

### Sorted, gapped, and bounded

`selectedIndices` is sorted before the tracks are pulled, per 08's note, so the
file reads in the running order the listener is looking at rather than in set
iteration order. Out-of-range indices are dropped, matching every other
selection method here. Pinned by *writes the selected tracks in the order the
listener sees them*, which uses a **gapped, reverse-inserted** selection
(`{4, 0, 2}`) so a passing test cannot be an accident of insertion order —
mutating the sort to `.reversed` fails exactly that test and nothing else.

An empty selection is refused at both ends: the menu item is greyed while
nothing is selected (so no dialog opens at all — pinned by *with nothing
selected it is inert, and opens no dialog*), and `createFromSelection` returns
null without writing if one somehow arrives. Cancelling the save dialog emits
no command, so there is no file and no entry.

### The two creates folded into one menu button

07 anticipated this and it happened: a third icon crowds the narrowest panel
the window allows (`TrampMetrics.playlistCollectionMinWidth`), and ticket 11
wanted a control in the same strip. `pl-collection-create` **keeps its key** and
its glyph but is now a menu button (`menu: true`, lit while open) offering
`pl-create-from-current` and `pl-create-from-selection`. The strip stays at four
controls: add, create, rename, remove.

A menu also buys something an icon could not: the two halves grey
**independently and legibly**. An empty current playlist and an empty selection
are different refusals, and the menu says which in words.

`MockupPlaylistCollectionPane` became a `StatefulWidget` for one reason — the
menu-open flag that lights the button. Everything else about it is unchanged.

### Notes for later tickets

- **Ticket 12/13** — the collection panel's control strip is now **full** at the
  minimum width. Another collection-level action should join the create menu or
  become a row context menu, not a fifth button.
- The window listens to `widget.playlist` as well as the collection, so
  from-selection wakes the instant a row is selected rather than when the host's
  next snapshot lands. Any future control gated on selection wants the same
  `ListenableBuilder`.
- `PlaylistCollectionController` now writes exactly one kind of file: a **new**
  playlist at a path the listener chose. It still never copies, moves, or
  renames a file the listener owns. Keep that sentence true.
