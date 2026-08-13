# 10 — Drag tracks to reorder

**What to build:** The listener sets the running order by hand, dragging a track to
where they want it. The playlist controller already has a tested move operation that
is wired to nothing, so this is mostly a matter of giving it a gesture and a path
through to the host.

Reordering is a change to the track list, so it marks the current playlist altered
and gets the same protection as tracks the listener added.

**Blocked by:** 01, 05

**Status:** done

- [x] Dragging a track to a new position moves it there, and the rest close up around it
- [x] The drag shows the listener where the track will land before they release
- [x] Reordering marks the current playlist altered
- [x] The playing track keeps playing across a reorder, and the playing row stays marked as the same track
- [x] Reordering reaches the host as a command and comes back in the playlist snapshot, so both windows agree
- [x] Dragging a row to where it already was is a no-op and does not mark the playlist altered
- [x] Reorder and the custom scrollbar do not fight each other; a drag near the list edge behaves sanely

## Comments

Delivered with tickets 09 and 11 in one pass (shared test file). Suite:
**595 passing / 2 skipped / 6 failing**, up from 555, goldens at byte-identical
margins, analyzer at **23**. Figures in ticket 09's comments.

### A drag carries the row it started on, and only that row

**The decision 08 asked for, made deliberately: collapse to the dragged row.**
Three reasons, in order of weight:

1. **The list can only show one row in flight.** Criterion two of this ticket is
   that the drag shows where the track will land *before release*. A
   multi-row move has no honest preview in a single-proxy list — the listener
   would carry one row and three would arrive. A preview the gesture cannot keep
   is worse than a gesture that does less.
2. **A gapped multi-selection has no single landing place.** Selection is now
   routinely non-contiguous (08). Dropping `{1, 4, 7}` between rows 2 and 3
   means inventing a rule — do they close up together, or keep their gaps? Every
   answer is a guess about intent, and none of them is what the preview showed.
3. **Nothing is lost.** `move` remaps the selection across the reorder
   (`_reindexAfterMove`), so the other selected rows keep their highlights and
   keep following their tracks. The listener can drag the rest, or use
   sort/reverse, which already act on the whole list.

Pinned by *drag: one row out of a selection keeps every highlight* — a
three-row selection, one of them dragged to the end, all three still marking
the same tracks afterwards. If 12/13/14 ever wants a multi-row drag, it needs a
multi-row `move` **and** a proxy that shows more than one row; do not add the
first without the second.

### The gesture

`ListView.builder` became `ReorderableListView.builder` inside the same
`MockupPlaylistScrollbar`, sharing the same `_scrollController` (as
`scrollController:`), so the custom scrollbar still drives and still tracks —
they never had a reason to fight, they are looking at one scroll position.
Pinned by *drag: past the bottom of the list it behaves sanely*, which pumps a
window short enough that the list actually scrolls, drags a row six rows down
past the end, and asserts no exception, five tracks still present, the right one
moved, and the custom scrollbar still mounted.

`buildDefaultDragHandles: false` with each row wrapped in a
`ReorderableDragStartListener`: **the whole row is the grip**, the way the
classic playlist behaved, rather than a handle column the mockup has no room
for. Every `pl-row-$index` key moved out to the listener so finders are
unchanged.

**The shift-click survives**, which was 08's warning. It survives for a
structural reason rather than a lucky one: the drag recognizer only claims the
pointer once it has moved past the hit slop, so a click never reaches it and the
row's own tap handler — modifiers and all — runs untouched. Pinned by *drag: the
recognizer does not swallow the shift-click*, which asserts both the rendered
highlights and the emitted `select:1` / `selectRange:3` ops. Double-tap-to-play
still has its own test too.

`proxyDecorator` puts back two things the framework takes away: the proxy is
built in the app's `Overlay`, above the window's `LookScope`, so the row is
handed the look again or it cannot paint at all; and Material's default floats
the row on an elevated card, which is the wrong idiom for a row that has the
list well behind it, so a transparent `Material` replaces it. The row in flight
looks exactly like the row that will land.

### One index convention, converted once

Flutter's `onReorderItem` answers with the row's **resting** position, having
already accounted for the hole it left; `PlaylistController.move` accounts for
that hole itself. `insertBeforeIndex(oldIndex, restingIndex)` — a top-level
function in the track pane, one line — converts resting to insert-before, and
that is the only place the two conventions meet. `PlaylistOpCommand` gained a
`toIndex` field carrying insert-before terms, so the host calls `move` with
exactly what the window did. Flattening the conversion to the identity fails
three widget tests and nothing else; checked, reverted.

`move` is emitted **whichever way it lands**, including a drop back where the
row came from. The controller is the single place that judges what counts as a
reorder and it makes that judgement identically at both ends, so a no-op is a
no-op twice over rather than a rule written down twice. Pinned by *drag:
dropping a row where it was picked up changes nothing* (a real drag — out one
row and back — with `altered` still down afterwards).

### The playing track

Free, and now pinned. `PlaybackController` follows the playing **path**, not the
index, so a reorder re-derives `playingIndex` without re-opening the engine. Two
new tests in `playback_controller_test.dart`: dragging the playing track itself
(`openCount` unchanged, still playing, mark moved with it) and reordering
*around* it (mark slides up as the row above leaves). Nothing stutters because
nothing is re-opened.

### A note on testing drags

Two things cost real time here and are worth passing on:

- **Use a mouse pointer.** `tester.startGesture(..., kind:
  PointerDeviceKind.mouse)`. A touch pointer spends 18px of the first row's
  travel on the touch slop, so a row-height move lands the proxy short of its
  neighbour and every expectation is off by one.
- **Move a row at a time with a pump between, then settle a few pixels inside
  the target slot.** The list reads the drop slot off the proxy's leading edge,
  and whole rows of travel put that edge exactly on a row boundary, where the
  answer is a coin toss. The shared `dragRow` helper does both.

### Notes for later tickets

- **Anything adding a gesture to a track row** now shares the pointer with a
  drag recognizer. Taps are safe (hit slop); a *horizontal* drag would not be.
- `PlaylistOpCommand.toIndex` is the first two-ended playlist op. A future
  multi-row move should carry a list rather than widening this one.
- `insertBeforeIndex` is exported from `mockup_playlist_track_pane.dart` and
  unit-testable; it is the only place the framework's index convention is
  translated. Keep it that way.
