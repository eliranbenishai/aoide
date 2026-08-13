# 03 — Add playlist files to the collection, list them, load on click

**What to build:** The listener's **playlist collection** becomes real, and this is
the slice that proves the whole architecture end to end.

The listener adds a playlist file they already have. It appears in the collection
panel showing a readable name and how many tracks it holds. Clicking it loads those
tracks into the **current playlist** immediately. Removing the row takes it out of
the collection and leaves their file exactly where it was. Adding a file that is
already in the collection selects the existing row instead of creating a second one.

Tramp stores a **reference** to the file at the path the listener chose — never a
copy, never a relocation. See ADR 0008.

Clicking a saved playlist replaces the current playlist without warning in this
ticket. That matches today's behaviour, so it is not a regression; ticket 05 adds
the protection.

**Blocked by:** 02

**Status:** done

- [x] Adding a playlist file stores a reference to it and shows a row for it
- [x] The playlist file is never copied, moved, or rewritten anywhere else
- [x] Rows show a display name and the track count, ordered alphabetically by name
- [x] Clicking a row loads that playlist's tracks into the current playlist
- [x] Removing a row takes the entry out of the collection and never touches disk
- [x] Adding a path already in the collection selects the existing row rather than duplicating it
- [x] Paths are compared in a normalized form, so the same file reached by a differently written path is recognised as the same entry
- [x] The collection panel carries its own controls, distinct from the footer's track-level add and remove
- [x] The collection survives a restart
- [x] Resetting settings leaves the collection intact, the way it already leaves installed skins intact
- [x] The collection index holds only what the panel paints, so startup reads stay small
- [x] Collection logic lives in an injectable module, not in the session host widget, and is covered by tests driving it against both a fake store and a temporary directory
- [x] Collection operations reach the host as commands and state returns as a snapshot, following the existing playlist command and snapshot pattern, with round-trip coverage for the new messages

## Comments

Suite went from 410 passing / 2 skipped / 6 failing to **451 passing / 2 skipped /
6 failing**. The 6 are the same Windows-authored chrome and main-player goldens,
failing by **exactly** the same margins as before (`title_bar_strip` 9.98% /
31134px, `main_player_window` 8.55% / 24560px, `main_player_window_quiet` 7.16% /
20550px, `screen_well` 0.48% / 1013px, `button_off_on` 0.41% / 626px,
`shell_plate_rail` 0.12% / 403px), so nothing outside this ticket moved. The 2
skips are still the host-guarded playlist goldens. `flutter analyze` reports the
same **23** pre-existing info issues, none in a file this ticket touched.

### What was built, and where the seams are

The collection is a plain injectable `ChangeNotifier` over a store —
`PlaylistCollectionController` in `lib/playlist/`, sibling to
`PlaylistController` — so none of it sits in `SessionHostApp`, which no test can
pump. `SessionHostApp` gained a `playlistCollectionStore` parameter beside the
existing `settingsStore` / `playlistStore` injection points.

Storage splits as the spec asked: `playlists.json` holds only what the panel
paints (path, optional name override, track count, total duration, last-seen
mtime) and is read at bootstrap; `playlist_tracks.json` caches each entry's
normalized track paths, is written on add, cleaned on remove, and is **never**
read at startup — a test asserts the read count is 0 after `bootstrap`, which is
the only way that promise stays true as later tickets touch this. Both readers
fall back to empty on any decode failure, following `FileSettingsStore` rather
than `FilePlaylistStore`. `session.json` was not extended.

### Decisions the ticket left open

**Canonical path form: `p.canonicalize`** (in `normalizePlaylistPath`, one
function, `lib/domain/saved_playlist.dart`). It is absolute, resolves `.` / `..`,
normalizes separators for the host style, and case-folds **only** on Windows —
whose filesystem is case-insensitive — while leaving POSIX spellings alone, where
two spellings really are two files. Rolling our own case rule would have to guess
the filesystem's sensitivity, and `LookController` already trusts this same
function for skin pack roots.

Its one cost: on Windows the stored path is lowercased, so a display name
*defaulted* from the filename reads lowercase. Rows render the display name
uppercased in the chrome font — exactly what the footer's status line already does
with the playlist filename — so in practice this is invisible, and ticket 08's
rename gives the listener the last word either way. The alternative (store the
spelling, compare canonically) needs two path concepts and was rejected for it.

**The highlight follows the current playlist's origin, and only when the origin
changes.** `select(path)` **clears** when the path is not in the collection, so a
row can never read as loaded while an arbitrary playlist file is what is actually
loaded. The host syncs it in `_onPlaylistChanged`, but gated on `sourcePath`
actually moving — same "adopt only real changes" discipline as the divider's
`didUpdateWidget`. Without that gate, adding an entry (which highlights it
without loading it, per ADR 0008 and ticket 09) would have its highlight dragged
away by the next unrelated track-selection change.

**Panel controls.** `pl-collection-add` / `pl-collection-remove`, 30×24 in their
own strip below the well, against the footer's 52×52 pair — different size,
different place, and semantic labels that name the noun ("Add playlist to
collection" vs "Add tracks", "Remove playlist from collection" vs "Remove
selected tracks"). A test asserts all four coexist with distinct labels. Remove
acts on the highlighted entry and is disabled with nothing highlighted. Add opens
the existing `pickPlaylistFile()` in the client, mirroring `onAddFiles`.

**Empty-state second line** now reads `ADD A PLAYLIST FILE WITH + BELOW`,
answering ticket 02's note; it deliberately keeps the `ADD A PLAYLIST FILE`
wording, so 02's assertion still holds.

### What was verified, and how

Most coverage is at the module seam (`test/playlist/`, 28 new tests): figures
computed from the playlist file, dedupe on a path spelled with a `.` and a `..`
hop, display-name default and override, alphabetical order maintained across
adds, index round-trip and restart through a temp directory, malformed and
truncated files reading as an empty collection instead of throwing, one bad entry
not costing the others, and the companion file staying correct across add and
remove. Two tests pin the ADR: after add and after remove the playlist file still
exists with byte-identical contents **and an unchanged mtime**, and the directory
holds exactly the same entries as before — so nothing was copied beside it.

Reset Settings is proved at the seam it actually lives at: write a collection,
run what `_handleResetSettings` does (`FileSettingsStore.write(defaults)`) against
the same support directory, and both the index and the companion file still read
back while the preference really did reset.

The window seam covers rendered rows (name and count, scoped per row key),
alphabetical order from a deliberately unsorted input, the loaded row painting
its highlight and a different ink, click emitting `LoadSavedPlaylistCommand`,
remove emitting `RemoveSavedPlaylistCommand` for the highlighted entry and
nothing at all with no selection, add invoking the picker callback, the empty
state appearing only while empty, rows fitting at `playlistMinWithCollection`
with no exception, and — guarding 02's rule — a collection snapshot arriving
mid-drag leaving the dragged divider width alone.

### Anything a later ticket should know

- The panel **sorts what it is given** (`SavedPlaylist.compareByDisplayName`, the
  same comparator the module uses), so row order never depends on the order a
  host snapshot happened to arrive in. Keep using that comparator rather than
  adding a second ordering rule.
- `SavedPlaylist.modified` is deliberately truncated to millisecond precision
  when read off the filesystem, because that is the precision the index stores.
  Without it, ticket 04's validation pass would read a moved mtime on every
  launch and recompute everything.
- **Adding a path already held does not refresh its figures** — it only selects
  the entry. Recomputing on a moved mtime is ticket 04's pass, and it is the one
  place that should own it.
- `lastError` exists on the module and rides the new snapshot (an unreadable file
  is reported, not kept), but **no UI renders it yet**. No ticket in this plan
  owns surfacing it — whoever needs it should render this channel rather than
  invent a second one. `lib/ui/settings/skins_panel.dart` shows how the skins
  equivalent is rendered.
- Loading an entry whose file has since vanished is currently a silent no-op. The
  only wrong answer would have been throwing out of the command handler.
  **Ticket 04** turns it into a disabled row.
- Ticket 12 gets its deduplicated figures from
  `PlaylistCollectionController.readTrackSets()`; the union has to be computed in
  memory at the point it is needed, never accumulated incrementally.
- The collection list has **no scrollbar** — it scrolls, but the track pane's
  `MockupPlaylistScrollbar` was not reused. If a ticket wants one, that is where
  to take it from.
- The new controls strip changes the playlist canvas, so ticket 14 regenerates
  the playlist goldens against a panel that now has rows *and* a control strip.
- `mockup_playlist_track_pane.dart` and `mockup_playlist_footer.dart` were **not**
  touched.
