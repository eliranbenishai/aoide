# 08 — Multi-select tracks

**What to build:** The listener can act on more than one track at a time. Today a tap
selects exactly one row and there is no way to select a range, so removing twelve
tracks means removing one twelve times.

Shift-click selects a contiguous range. The platform modifier click toggles
individual rows in and out, so a selection need not be contiguous. A plain tap
collapses back to a single row.

Selection is not a change to the playlist, so none of this marks the current playlist
altered. Acting on a selection — removing those tracks — does, by way of ticket 05.

**Blocked by:** 01

**Status:** done

- [x] Shift-click selects every row between the anchor and the clicked row
- [x] The platform modifier click adds and removes individual rows from the selection, using the convention native to each desktop
- [x] A plain tap collapses the selection to the clicked row
- [x] Every selected row renders as selected
- [x] Removing tracks acts on the whole selection
- [x] Selecting rows never marks the current playlist altered
- [x] Selection state survives a playlist snapshot round trip, since it already rides one
- [x] Shift-clicking with nothing previously selected behaves predictably rather than throwing

## Comments

Delivered with ticket 07 in one pass (shared test file). Suite: **555 passing /
2 skipped / 6 failing**, up from 519, with the 6 Windows-authored goldens
failing at byte-identical margins and analyzer still at 23, none in a touched
file. See ticket 07's comments for the full figures.

### Two new controller methods, no new selection channel

`selectRange(index)` and `toggleSelection(index)` sit beside `select`,
`setSelectedIndices`, `selectAll` and `invertSelection`, write the same
`_selectedIndices` / `_selectedIndex` pair, and ride the same
`PlaylistSnapshotEvent` — nothing new crosses the bus except two more
`PlaylistOpCommand` verbs (`selectRange`, `toggleSelect`) alongside the existing
`select`.

**`_selectedIndex` is the anchor.** It was already the "current" row and already
rode the snapshot, so no second field was needed. Two consequences worth
knowing:

- A shift-click **does not move the anchor**. Shift-clicking 5 then 8 then 3
  measures from the same starting row each time, so the range follows the mouse
  instead of crawling along behind it — the behaviour every desktop file manager
  has. Pinned by *a second shift-click re-measures from the same anchor*.
- A modifier click **does** move it, so a shift-click after one ranges from the
  row the listener last pointed at.

**Shift with no anchor is a plain select**, not a throw and not a no-op: with
nothing to measure from there is no range to describe, and the row under the
cursor is the only sensible answer. The same path covers the anchor going stale
— `removeSelected` clears the selection, and a shift-click straight after it
lands here rather than indexing into a list that shrank. Both pinned.

Dropping the anchor row with a modifier click falls back to the **lowest row
still selected**, the same fallback `setSelectedIndices` already used, or to no
anchor when the selection is emptied. Out-of-range indices are ignored by both
methods, as `select` already did.

Neither method touches `_altered`, and the altered-state group has a test that
runs a range, a toggle and a collapse and asserts the flag never rises. Removing
still raises it, via `removeSelected` — selecting is not a change, acting on a
selection is.

### How the platform modifier is detected

In the row widget, from Flutter's own state — no hardcoding, no new dependency:

```dart
final keyboard = HardwareKeyboard.instance;
return defaultTargetPlatform == TargetPlatform.macOS
    ? keyboard.isMetaPressed
    : keyboard.isControlPressed;
```

`isShiftPressed` wins over the toggle modifier when both are held. The whole
decision is one pure function, `trackRowSelectionFromKeyboard()`, returning a
three-valued `TrackRowSelection` (`replace` / `range` / `toggle`) that the row
hands to its `onSelect` callback. The row does not know what a playlist is; the
mapping from intent to controller method lives in `MockupPlaylist` beside the
existing echo to the host.

`defaultTargetPlatform` rather than `dart:io`'s `Platform.isMacOS`, which is
what the rest of the repo reaches for (`tramp_window.dart`, `file_open.dart`,
the docking layer). Deliberate, and the one place this ticket departs from
local precedent: those are process-level questions asked outside the widget
tree, while this is a widget asking which keyboard convention it is painting
for. `defaultTargetPlatform` is the notion Flutter itself uses for that, and
it is the only one `debugDefaultTargetPlatformOverride` can move — so the macOS
branch is a tested branch rather than a comment. `Platform.isMacOS` would have
left it unreachable from a Linux test run.

**How it is tested.** Modifiers are simulated the way this repo already does it
in `mockup_main_player_test.dart`: `tester.sendKeyDownEvent` /
`sendKeyUpEvent` with `LogicalKeyboardKey`, driving real `HardwareKeyboard`
state rather than a stub. *on macOS it is Command that toggles, and Control that
does not* flips `debugDefaultTargetPlatformOverride` to macOS inside a
`try`/`finally` (the framework asserts on a leaked override in `tearDown`, and
the `finally` has to run before that) and asserts both directions: Cmd-click
toggles, Ctrl-click collapses. Mutating the helper to always read
`isControlPressed` fails exactly that test and nothing else — checked, reverted.

### The row widget

Edited as this ticket allows, but narrowly: `pl-row-$index` keys unchanged,
painting unchanged, the only difference being that more rows can now be in the
selected state at once. `onSelect` gained a second parameter for the intent.
`onDoubleTap` is untouched, and a test asserts double-tap still plays the row it
landed on.

One test-only wrinkle worth passing on: with a `onDoubleTap` present, a tap does
not resolve until the double-tap window closes, so `pumpAndSettle` alone is not
enough after a click — the shared `clickRow` helper pumps 400ms first. A
multi-select test written without that will fail in a way that looks like the
modifier not registering.

### What was verified

Controller seam, 17 tests: ranges forwards, backwards, re-measured, and onto the
anchor itself; shift with no anchor and shift after a remove cleared it; toggle
adding, dropping, emptying the selection, and re-anchoring; plain tap
collapsing; out-of-range rows; `removeSelected` over both a contiguous and a
gapped selection; nothing raising altered; and a selection surviving a snapshot
round trip.

Widget seam, 8 tests: the gestures select the right rows *on screen* (asserted
on the rows' rendered selection, not just the controller); the macOS pair; a
plain tap collapsing; shift with nothing selected; removing acting on the whole
selection; selection not marking the playlist altered; double-tap still playing.

Message seam: both new verbs round-trip, and `PlaylistSnapshotEvent` carries a
multi-row selection **and its anchor** across the bus.

Mutation check: making a range measure from the clicked row instead of the
anchor fails 7 controller tests. Reverted.

### Notes for later tickets

- **Ticket 09 (create from selection)** — `selectedIndices` is a `Set<int>` in
  no particular order; sort it before writing, so the file matches what the
  listener sees. It is now routinely non-contiguous, which is the whole point of
  the toggle: do not assume a range. `removeSelected`'s ordering is the
  precedent to copy.
- **Ticket 10 (drag reorder)** — this is where the seams collide. The row's tap
  handling now reads modifier state at tap time, so a drag recognizer added to
  the same widget must not swallow the shift-click. `PlaylistController.move`
  already remaps the selection across a reorder (`_reindexAfterMove`), so a
  single-row drag keeps its highlight for free — but it moves **one** row, and
  a drag that starts inside a multi-selection arguably ought to carry the whole
  selection. That is a decision 10 should make deliberately: either collapse to
  the dragged row, or add a multi-row move and reindex it the way `move`
  already does for one.
