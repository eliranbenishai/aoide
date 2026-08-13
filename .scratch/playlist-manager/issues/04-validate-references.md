# 04 — Validate references after load; disable missing, refresh edited

**What to build:** Because the collection holds references to the listener's own
files, those files move, vanish, and get edited by other programs. Tramp checks on
them — after the app has finished loading, never during startup, so a large
collection never costs launch time.

When a playlist file has gone missing its entry becomes a **disabled playlist**: the
listener can see it and remove it, but not load it. When the file comes back — a
drive remounted, a share reconnected — the entry works again on its own, with no
action needed. When a playlist was edited in another program, its track count
catches up.

**Blocked by:** 03

**Status:** done

- [x] Reference checking runs after the app has loaded and does not delay startup
- [x] An entry whose file is missing renders distinctly as disabled
- [x] A disabled entry cannot be loaded; attempting it does nothing destructive
- [x] A disabled entry can still be removed from the collection
- [x] Disabled state is derived from the most recent check rather than stored, so a returning file re-enables its entry with no listener action
- [x] A playlist edited outside Tramp shows its updated track count and duration after the next check
- [x] Only entries whose files actually changed are re-read
- [x] A collection where every file is missing still lists every entry — nothing is silently dropped

## Comments

Suite went from 451 passing / 2 skipped / 6 failing to **470 passing / 2 skipped /
6 failing** — 19 new tests, no regressions. The 6 are the same Windows-authored
goldens, failing by **exactly** the same margins as before (`title_bar_strip`
9.98% / 31134px, `main_player_window` 8.55% / 24560px, `main_player_window_quiet`
7.16% / 20550px, `screen_well` 0.48% / 1013px, `button_off_on` 0.41% / 626px,
`shell_plate_rail` 0.12% / 403px). `flutter analyze` reports the same **23**
pre-existing info issues, none in a file this ticket touched.

### Where the pass lives

`PlaylistCollectionController.validateReferences()` — in the injectable module,
not in `SessionHostApp`. The host only *triggers* it: one `unawaited(...)` call
at the very end of `_bootstrap`, after `_revealWindows`, the second
`_applyAllFrames()`, and `_applyAlwaysOnTop()`, so it is both off the startup
path and last in it. Its result reaches the Playlist Manager the ordinary way —
the pass notifies, `_onCollectionChanged` broadcasts a snapshot — and a client
that connects later gets the current one from the `ClientReadyCommand` push, so
there is no race with window startup.

One `FileStat.stat` per entry answers existence *and* modification time in the
same visit and never throws, which is why the pass is a single loop with no
`exists()` / `lastModified()` pair. `add` now reads its stamp from the same
helper, so the stamp the index stores and the stamp the pass reads cannot drift
apart by construction rather than by discipline.

### Decisions the ticket left open

**Disabled is a set on the controller, not a field on `SavedPlaylist`.**
`disabledPaths` / `isDisabled(path)` are derived from the last check;
`SavedPlaylist` has no such field, so "not stored" is structural — there is no
`toJson` line to forget. It rides the snapshot beside `playlists` as
`disabledPaths`, and a snapshot from before this ticket decodes as none rather
than throwing.

**A disabled row's tap sends the new `SelectSavedPlaylistCommand`, not a load.**
Without this the "still removable" criterion is unreachable in practice: the
panel's remove control acts on the highlighted entry, and the only way a listener
highlights a row is by tapping it. Ticket 03 already established that the
highlight means "the entry the listener is acting on" rather than strictly
"loaded" (`add` highlights without loading), so highlighting a disabled row fits
the existing model. Tickets 09 and 11 both want select-without-load too.

**`resolveForLoad(path)` re-checks one file on click** instead of trusting the
last pass, so a file that vanished five seconds ago disables its row on the click
that found out rather than being a silent no-op until the next launch. It
deliberately does **not** recompute figures — the file would then be read twice
per click, and the spec puts recomputation on the pass. So an entry edited
elsewhere loads its new tracks while its row shows the old count until the next
pass; that matches the acceptance wording ("after the next check") and is noted
below as the one thing a future ticket might tighten.

**A file that is there but unreadable is not disabled.** Disabled means the file
is gone. A path that exists but cannot be parsed keeps its last known figures
(never a zero) and reports through `lastError`. Nothing renders `lastError` yet —
still the case flagged in ticket 03.

**Rendering.** The row keeps its display name *and* its track count — a disabled
playlist still counts toward every About figure, so hiding the number would lie.
Distinctness comes from `PlaylistCollectionMissingMark` (a struck ring, painted
in the same stroked idiom as the collapse chevron, keyed
`pl-collection-missing-$index`) plus `MockupHoverTokens.disabledOpacity` — the
same dim every disabled control in this chrome already uses, so no new alpha and
no palette colour was invented. The dim wraps the row *contents* only, leaving
the selection gradient at full strength, because a disabled row can be the
highlighted one and that highlight is what remove acts on. Semantics label gains
`, file missing`; the row deliberately does **not** report `enabled: false`,
because it still responds to a tap.

### How "recomputes nothing" was proved

Three tests, and a mutation to show they bite. `CountingM3uCodec` (test-local,
overrides `parse`) records every playlist file the controller opens — the
controller parses exactly once per read, so the count *is* the read count.

- *a pass over an unchanged collection recomputes nothing*: two entries, two
  passes back to back — parse count unchanged, `indexWrites` unchanged,
  `trackSetWrites` unchanged, and zero listener notifications.
- *a pass on the launch after a restart reads no playlist file*: the real trap.
  Entries are written through `FilePlaylistCollectionStore`, a **fresh**
  controller bootstraps from `playlists.json`, then validates — 0 parses, and
  `modified` compares equal across the round trip. This is the assertion that
  fails if `SavedPlaylist.modified` ever carries precision the index cannot hold.
- *only entries whose files actually changed are re-read*: proves the comparison
  works in the other direction too, so the two above cannot pass vacuously.

Mutation check: changing the stamp comparison by a single microsecond fails
exactly those three tests and nothing else. Reverted, obviously.

Note for anyone writing more of these: rewriting a playlist file in a test may
not move its modification time (same millisecond), so `editPlaylistElsewhere`
sets it forward explicitly. That is what another program's edit looks like, and
it keeps the tests honest about what Tramp actually watches.

### Anything a later ticket should know

- **The pass runs once per launch.** An edit made in another program while Tramp
  is running is not noticed until the next launch (or the next click on that row,
  which re-checks existence only). `validateReferences()` is re-runnable and
  idempotent, so if a ticket wants fresher figures the answer is another trigger
  — window focus, panel reopen — not a second code path. Do **not** put it
  anywhere reachable from `_bootstrap`'s awaited sequence.
- **Ticket 12 (About figures):** disabled entries stay in `entries` and keep
  their cached figures and their companion track-set rows, so the union in
  `readTrackSets()` already includes playlists on a disconnected drive. Nothing
  extra is needed to make "playlists on a disconnected drive still count" true —
  just do not filter by `disabledPaths`.
- **Ticket 11 (rename):** renaming a disabled entry already works — the module
  never reads the file to change a name, and `copyWith` leaves `modified` alone,
  so a rename cannot make the next pass think the file was edited. Keep it that
  way: writing a fresh stamp on rename would force a needless re-read.
- **Tickets 05 / 07 / 09:** `_handleLoadSavedPlaylist` is now
  `resolveForLoad` → `openPlaylistFile`. The altered-playlist confirmation from
  ticket 05 belongs **before** the resolve, or the listener gets prompted about
  discarding work in order to load a playlist that then turns out to be
  unloadable.
- **`SelectSavedPlaylistCommand`** exists now (host: `_collection.select(path)`).
  Use it for "highlight without loading" rather than adding a second message —
  ticket 09's "creates the entry and highlights it" is the same idea.
- **Do not persist a disabled flag.** If a future ticket wants a *reason* on the
  row (missing vs unreadable), extend the derived channel — a second set, or a
  map beside it — not `SavedPlaylist`.
- `mockup_playlist_track_pane.dart` and `mockup_playlist_footer.dart` were **not**
  touched. Ticket 14's golden regeneration now also has to account for a
  missing-file mark being possible on collection rows.
