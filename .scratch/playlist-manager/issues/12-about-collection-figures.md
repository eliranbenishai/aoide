# 12 — Real playlist, track, and total-time figures in the About well

**What to build:** The About window's stats well currently shows four invented
numbers. Three of them stop being fiction here: how many playlists the listener
keeps, how many tracks that is, and how much running time that adds up to. Spins is
ticket 13.

A track that sits in three playlists counts once — the well answers "how much music
do I keep", and counting it three times inflates the figure back into fiction.
Playlists on a disconnected drive still count; unplugging a disk must not appear to
rewrite the listener's history.

The figures are maintained as the collection changes, not by indexing everything at
launch. The deduplicated set is the union of entries' track paths, read only when the
figures are actually wanted, so it never lands on the startup path.

**Blocked by:** 03, 04

**Status:** done

- [x] The playlists figure counts entries in the collection, including disabled ones
- [x] The tracks figure counts distinct tracks across the collection, each once, compared in normalized form
- [x] Total time sums those distinct tracks' durations
- [x] Disabled entries contribute to all three figures
- [x] The unsaved current playlist does not contribute
- [x] Figures are recomputed when a playlist is added, saved, removed, or found changed on disk — never by a full pass at launch
- [x] Startup time does not regress; the per-entry track sets are read lazily, not at launch
- [x] Figures reach the About window over the session bus, not by a client reaching into a controller
- [x] The figures report as measured, and the placeholder is retired or kept only as a test fixture
- [x] The existing four rows are reused — the well's layout and canvas do not change

## Comments

Delivered together with ticket 13, because 13 needs the About-window bus wiring
this ticket lays down. Suite went from **595 passing / 2 skipped / 6 failing** to
**636 / 2 / 6**. The 6 are the same Windows-authored goldens failing by
**exactly** the same margins (`title_bar_strip` 9.98% / 31134px,
`main_player_window` 8.55% / 24560px, `main_player_window_quiet` 7.16% /
20550px, `screen_well` 0.48% / 1013px, `button_off_on` 0.41% / 626px,
`shell_plate_rail` 0.12% / 403px). **`about_window_golden_test.dart` still
passes** — see below. `flutter analyze` reports the same **23** pre-existing info
issues, none in a file these tickets touched.

### Where the figures come from

`PlaylistCollectionController.readFigures()` returns a `CollectionFigures`
(`lib/domain/collection_figures.dart`) built **in memory at the point it is
wanted**: the union of the entries' cached track paths, counted, and their
running times summed. Nothing is accumulated incrementally, exactly as ticket 03
and the spec insisted — a running total cannot be subtracted correctly when an
entry is removed or rewritten, because the tracks it held may also be held
elsewhere. There is a test for precisely that (`removing an entry subtracts only
what nothing else holds`).

Disabled entries are in `_entries`, so they contribute to all three figures for
free; there is no branch to get wrong. The current playlist is not in the
collection, so it contributes nothing by construction.

### The one storage change

Total time is the sum of the **distinct** tracks' durations, and the companion
file only held paths — so it now holds a running time per distinct track path as
well. `readTrackSets()` / `writeTrackSets()` carry a `CollectionTrackSets`
(`byEntry` + `durationsMs`) rather than a bare map, because both live in one
file and must be written together: a write that knew only the track sets would
erase the running times beside them.

Running times are keyed by **track**, not by entry. A length is a property of
the file — a track kept in three playlists runs for one length, and the
deduplicated total wants exactly one of them — so the global map is both correct
and smaller. It is pruned to the paths the entries still hold on every write, so
tidying the collection cannot leave the file growing forever. A companion file
written before this change still reads: `trackDurations` is absent, the count is
unaffected, and total time comes back on that entry's next refresh.

### Null durations: they count as a track, and add no time

An M3U line with no `#EXTINF` before it gives `Track.duration == null`. Such a
track **counts in the tracks figure** — Tramp knows the listener keeps it — but
**adds nothing to total time**. Rounding up to a guessed average would put the
invented number back in the well by the side door, and this ticket exists to get
it out. Zero-length durations are treated the same as null, matching what
`_enrichMissingTrackMetadata` already considers unfilled. Once the background
metadata probe fills a duration in, the figure follows on the entry's next
refresh, which is where every other stale-figure fix in this module already
lives.

### How "never at launch" is actually enforced

Three guards on `_refreshAboutStats`, and the first is the load-bearing one:

1. **The About window must be open.** It is hidden at launch, so the companion
   file is not read during startup at all — not on `ClientReady`, which happens
   while the session is still coming up. Opening the window is the moment the
   figures are wanted, and `ToggleWindowCommand` is where the refresh hangs.
2. `figuresRevision` must have moved. It is a new counter on the collection that
   bumps on exactly the four events the ticket lists — added, saved, removed,
   found changed on disk (plus `bootstrap`, which changes the entry list) — and
   deliberately **not** on `select`, `rename`, or a no-op validation pass. Two
   tests pin both halves. Without it, every highlight would cost a file read,
   because `_onCollectionChanged` cannot tell why it fired.
3. `spins` must have moved. Playback notifies on every position tick.

A test also proves the figures never open a playlist file: a restarted
controller with a counting codec answers `readFigures` with **zero** parses.

### The golden

`AboutWindow.stats` now defaults to `AboutStats.unmeasured` (zeros, `measured`
false) rather than the placeholder, so a window whose host has not spoken yet
says so. That would have changed the golden, so
`about_window_golden_test.dart` passes `AboutStats.placeholder` explicitly — the
placeholder is retired from `lib/` and kept as a **test fixture**, which the
ticket allows, and the rendered image is unchanged. It still passes. The well's
four rows, layout, and canvas are untouched.

### Anything a later ticket should know

- The figures are pushed **only while the About window is open**. Anything that
  wants them elsewhere should widen that guard deliberately, not remove it —
  it is the whole reason the companion file stays off the startup path.
- `AboutStats.placeholder` must not be referenced from `lib/`. That is the only
  thing keeping the invented figures from reaching a listener again.
