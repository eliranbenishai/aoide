# Playlist Manager

Status: ready-for-agent

Domain vocabulary: [`CONTEXT.md`](../../CONTEXT.md) — **playlist collection**, **saved
playlist**, **current playlist**, **altered current playlist**, **disabled playlist**.
Storage decision: [ADR 0008](../../docs/adr/0008-playlist-collection-stores-references.md).

## Problem Statement

Tramp can hold exactly one playlist at a time. A listener who keeps several — a
work playlist, a driving playlist, one per record — has no way to move between
them inside the app. They open a playlist file, and the one they had is simply
gone. To go back they must remember where on disk the other one lived and open it
again through a file dialog.

Nothing warns them, either. A listener can drop thirty tracks into the current
playlist, open a saved playlist file, and lose all thirty with no prompt, because
Tramp has no idea the list was ever changed.

The window is called the Playlist Editor and behaves like one: a single list with
no notion of the playlists a listener actually keeps. And because Tramp knows
nothing about a listener's playlists as a set, the About window's stats well —
playlists, tracks, total time, spins — shows four invented numbers.

## Solution

The playlist window becomes the **Playlist Manager**: the listener's **playlist
collection** on the left, the **current playlist** on the right, and a divider
between them they can drag.

The collection lists the playlists the listener keeps. Clicking one loads it
straight into the current playlist. They can add playlist files they already
have, and create new playlists from the tracks in the current playlist — either
all of them, or just a selection. Tramp stores **references** to those files
where the listener put them and never copies or moves them, so the playlists
stay theirs and keep working in their file manager and their other players.

Tramp tracks whether the current playlist has been changed since it was loaded
or saved. Navigating away from an **altered current playlist** asks first, and
offers to save rather than only to discard. An altered list also survives a
restart, so closing the app is never how a listener loses work.

Because Tramp now knows the collection, the About window's stats well stops
lying: real playlist and track counts, real total running time, and a real
count of **spins**.

## User Stories

1. As a listener, I want the playlist window titled "Playlist Manager", so that its name matches what it now does.
2. As a listener, I want to see my playlist collection beside the current playlist, so that I can see what I keep without opening a dialog.
3. As a listener, I want to click a saved playlist and have it load immediately, so that switching between playlists takes one action.
4. As a listener, I want to drag the divider between the two panels, so that I can give room to whichever side I am working in.
5. As a listener, I want the divider to stay where I put it across restarts, so that I set my layout once.
6. As a listener, I want to collapse the collection panel entirely, so that a single-playlist session looks like the old window.
7. As a listener, I want the collapsed panel to reopen at the width I last used, so that collapsing is not destructive.
8. As a listener, I want the track list to absorb the extra room when I resize the window, so that widening the window shows me more of what I care about.
9. As a listener, I want to add a playlist file I already have to my collection, so that playlists I made elsewhere are reachable from Tramp.
10. As a listener, I want Tramp to leave my playlist file exactly where I put it, so that my file manager and my other players keep working.
11. As a listener, I want adding a file already in my collection to just select it, so that I never end up with two rows for one playlist.
12. As a listener, I want to create a new playlist from every track in the current playlist, so that an ad-hoc pile I have built becomes something I keep.
13. As a listener, I want to create a new playlist from only the tracks I have selected, so that I can pull a shorter playlist out of a longer one.
14. As a listener, I want creating a playlist from a selection to leave my current playlist untouched, so that capturing a few tracks never costs me the rest.
15. As a listener, I want a newly created playlist to appear in my collection automatically, so that I do not have to add what I just made.
16. As a listener, I want to rename a saved playlist to something readable, so that my collection is not a list of filenames.
17. As a listener, I want renaming to leave the file's name alone, so that Tramp never reaches into my folders and renames my things.
18. As a listener, I want to remove a playlist from my collection, so that I can tidy it without losing the file.
19. As a listener, I want removing an entry to never delete the file, so that tidying my collection can never destroy music I keep.
20. As a listener, I want my collection sorted by name, so that I can find a playlist by reading.
21. As a listener, I want each row to show how many tracks the playlist holds, so that I can tell my playlists apart at a glance.
22. As a listener, I want the app to start just as fast as it does now, so that a larger collection never costs me launch time.
23. As a listener, I want to be told when a saved playlist's file has gone missing, so that I am not left wondering why nothing loads.
24. As a listener, I want a disabled playlist to be removable but not loadable, so that the only thing I can do is the thing that works.
25. As a listener, I want a playlist to work again by itself when its drive comes back, so that unplugging a disk does not cost me my collection.
26. As a listener, I want playlists I edited in another program to show their new track counts, so that the collection reflects reality.
27. As a listener, I want to be warned before an altered current playlist is replaced, so that changes I have not saved are not thrown away silently.
28. As a listener, I want that warning to offer to save for me, so that protecting my work does not mean backing out and starting over.
29. As a listener, I want cancelling that warning to leave everything exactly as it was, so that the safe choice is genuinely safe.
30. As a listener, I want no warning when I have changed nothing, so that the prompt still means something when it appears.
31. As a listener, I want an altered current playlist to come back after I quit and reopen Tramp, so that closing the app is never how I lose work.
32. As a listener, I want quitting to stay instant, so that protecting my work does not slow down closing the app.
33. As a listener, I want to select a range of tracks with shift-click, so that I can act on many tracks at once.
34. As a listener, I want to add and remove individual tracks from my selection with a modifier click, so that I can build a selection that is not contiguous.
35. As a listener, I want to drag a track to a new position, so that I can set the running order by hand.
36. As a listener, I want reordering to count as a change, so that a running order I have not saved gets the same protection as tracks I have added.
37. As a listener, I want the About window to show how many playlists I keep, so that the number means something.
38. As a listener, I want the About window's track count to count each track once, so that a track in three playlists does not inflate the figure.
39. As a listener, I want total time to reflect the music I keep, so that the reading is honest.
40. As a listener, I want playlists on a disconnected drive to still count, so that unplugging a disk does not appear to rewrite my history.
41. As a listener, I want spins counted when a track plays through to the end, so that skipping around does not inflate the number.
42. As a listener, I want my spin count to survive restarts, so that it reads as a lifetime total.
43. As a listener, I want resetting my settings to leave my collection and my spin count alone, so that fixing a preference does not erase my history.
44. As a listener, I want to open a one-off playlist file without it joining my collection, so that looking at something does not mean keeping it.
45. As a listener, I want to add that one-off playlist to my collection if I decide to keep it, so that the decision stays mine.

## Implementation Decisions

### Storage model

- The collection stores **references** to playlist files, never copies. This is
  binding and the reasoning (including why skins deliberately differ) is recorded
  in ADR 0008.
- A saved playlist's **identity is its normalized absolute path**. Adding a path
  already present selects the existing entry instead of creating a second one.
- Each entry carries a display name that may be overridden independently of the
  filename. Renaming never touches the file.
- Creating a playlist writes the playlist file wherever the listener chooses,
  then adds a reference to it automatically.
- Removing an entry never touches disk. There is no delete-the-file action.

### On-disk layout

- A **collection index** holds only what the left panel needs to paint: display
  name, path, track count, total duration, and last-seen modification time. It
  stays small and is read at startup.
- A **companion track-set file** holds each entry's normalized track paths. It is
  read lazily, only when the deduplicated About figures are needed, so its size
  never lands on the startup path.
- A **usage file** holds the lifetime spin count, written debounced.
- The altered current playlist's track list is persisted continuously and
  debounced, in the manner of the existing playback resume state.
- The existing last-session store is **not** extended. It means "last session",
  and it is the one store whose reader has no error handling, so a malformed file
  would break startup.
- Resetting settings must spare the collection index, the companion file, and the
  usage file — the same way it already spares installed skins. Content survives;
  preferences reset.

### Validation and aggregates

- References are validated **after** the app has loaded, never on the startup
  path. The pass checks existence and modification time together.
- A missing file makes an entry **disabled**. Disabled is **derived** from the
  last pass rather than stored, so a file that returns re-enables its entry with
  no listener action.
- A moved modification time recomputes that entry's cached count, duration, and
  track set.
- Aggregates are maintained **incrementally** — on add, on save, and on a moved
  modification time. There is no cold indexing pass, because there is never a
  state with entries but no numbers.
- Deduplicated totals are computed as the union of entries' normalized track
  paths, in memory, at the point they are needed. Incremental arithmetic on a
  running total is deliberately avoided: it cannot subtract correctly when an
  entry is removed or rewritten.
- Disabled entries still contribute to every About figure.

### Altered state

- The altered state is raised **only by mutation** of the current playlist's track
  list: add, remove, reorder, sort, clear. Loading a saved playlist sets the
  baseline and leaves it unaltered.
- It is lowered **only when the entire current track list is written to a file
  that becomes the current playlist's origin**. That covers save, save-as, and
  create-from-all-tracks.
- Create-from-selection is a different operation: it writes a separate playlist
  file, adds its reference, and leaves the current playlist and its altered state
  untouched. It does **not** navigate — it creates the entry and highlights it.
- Altered state lives with the current playlist on the host side so it can be
  broadcast, because the window that renders it is a separate engine.

### Confirmation flow

- Navigating to another saved playlist while altered presents three choices:
  save and load, discard and load, cancel.
- Cancel takes default focus, so an idle keypress cannot destroy anything.
- Save writes straight to the origin when the current playlist has one, and opens
  the save dialog when it does not. Cancelling that dialog returns to the current
  playlist with the altered state still raised — it does not fall through to
  loading.

### Layout

- The window keeps its current free-resize behaviour and its existing relationship
  to global zoom. Nothing about the zoom model changes.
- The divider position persists as a logical pixel width alongside the window
  frames in settings.
- The collection panel collapses, and remembers a restore width.
- The minimum window width rises while the panel is shown and returns to today's
  minimum when it is collapsed.
- On window resize the collection panel holds its pixel width and all slack goes
  to the track list, matching how the footer and rows already stay fixed while
  the list well grows.
- The collection is ordered alphabetically by display name. Manual ordering is out
  of scope; see below.
- Rows show display name and track count, and render distinctly when disabled.
- The collection panel carries **its own** controls. The existing footer's add and
  remove act on tracks, and collection-level add and remove must not sit beside
  them meaning something different.
- The existing footer load and save items act on the current playlist and stay
  where they are.

### Track list interaction

- Shift-click selects a contiguous range; the platform modifier click toggles
  individual rows; a plain tap collapses the selection to one row.
- Drag-to-reorder is delivered in this work. The playlist controller already
  exposes a tested move operation that is currently wired to nothing.
- Reorder counts as a mutation and raises the altered state.

### Boundaries and wiring

- The collection lives in an **injectable module**, not inside the session host
  widget. The host widget is not exercised by any test and cannot be — it is
  bound to the multi-window and window-manager plugins — so logic placed there
  is untestable by construction. Every session decision in this codebase that
  needed testing was extracted this way.
- The Playlist Manager is a secondary engine. Collection operations travel to the
  host as commands and state returns as snapshot events, following the existing
  playlist command and snapshot pattern.
- About figures reach the About window over the session bus like every other
  snapshot, never by reaching into a controller from a client.
- The About stats placeholder is retired. Real figures report as measured.
- Spins are counted from natural end-of-stream only, at the existing playback
  completion hook. A skip never counts, however late. Each repeat-one pass counts,
  because the track genuinely played through.
- The window's user-facing title changes. Internal type and role names are left
  alone; renaming them is churn with no product effect.

## Testing Decisions

A good test here asserts **observable behaviour at a module's edge** — what a
listener would notice, or what a caller receives. It does not reach into private
state, and it does not pin the internal shape of the collection module, because
that shape is expected to change. Three seams, agreed with the developer; only
the first is new.

### Seam 1 — the playlist collection module, against an injected store

The heaviest logic and the widest coverage. Exercises adding, creating, renaming,
and removing entries; dedupe on normalized path; the validation pass marking
entries disabled and re-enabling them when files return; modification-time
recomputation; incremental aggregate maintenance; and the deduplicated figures.
Also covers that resetting settings spares stored content.

Prior art: the playlist controller tests drive a controller against an in-memory
store, and the playlist store tests round-trip a real file store through a
temporary directory. Follow both — fake store for logic, temp directory for the
on-disk contract.

### Seam 2 — the Playlist Manager window, pumped as a widget

Extends the existing playlist widget tests rather than adding a new pattern.
Covers two-panel layout, divider drag, minimum widths, collapse and restore, row
rendering including disabled rows, multi-select gestures, drag-to-reorder, the
three confirmation outcomes, and click-to-load. Assertions stay on rendered
geometry, rendered text, and emitted commands — the way that file already works.

### Seam 3 — the session message vocabulary

Extends the existing session message tests. Cheap, but it is the contract between
the other two seams and the only thing preventing both sides from passing green
while the feature is broken end to end.

### Folded into existing seams

- Altered state belongs with the playlist controller tests, where playlist state
  already lives.
- Spins belong with the playback controller tests, beside the completion
  behaviour they hang off.
- The new on-disk files get a store test in the established temp-directory shape.

### Goldens

The playlist goldens are authored on Windows and already fail on a Linux host on
font rasterisation alone; a two-panel redesign invalidates them outright. They
give no signal during this work. Treat them as knowingly stale, drive the work
with the seams above, and regenerate them on Windows in one pass at the end.
Decide separately whether to guard them by host so the suite can run green.

## Out of Scope

- A track **Library** — an index over designated folders. The word stays reserved
  in the glossary and this work does not create one.
- Playlist formats beyond the M3U family already supported.
- Manual ordering of the collection. It needs a persisted position per entry and a
  second drag interaction competing with track reorder.
- Deleting a playlist file from disk.
- Folders, nesting, tags, or grouping within the collection.
- Searching or filtering the collection.
- Smart or generated playlists.
- Per-playlist shuffle or repeat state.
- Wiring command-line arguments, which are parsed today but consumed nowhere.
- Regenerating the playlist goldens, which happens on Windows after this work.

## Further Notes

- Quitting is deliberately immediate: the app exits rather than tearing down its
  engines, and slow quit is a filed release blocker. Nothing in this feature may
  add work to shutdown — hence continuous debounced persistence of the altered
  list rather than a save or a prompt at quit.
- The playlist window is already the only freely resizable window, so a divider
  fits the existing model. There is no reusable splitter anywhere in the codebase;
  the settings window's side tabs are a fixed-width column, not a draggable one.
- The playlist controller's move operation is implemented and covered by tests but
  wired to no UI or command, which makes drag-to-reorder unusually cheap right now.
- The About window is a fixed canvas whose stats well gives its four rows equal
  flex with no scroll, so its vertical budget is tight. This work replaces the
  values in the existing four rows; it does not add rows.
- The baseline before this work is 393 tests passing and 8 failing, all 8 being
  Windows-authored goldens that cannot pass on a Linux host. Any new failure is
  therefore ours.
