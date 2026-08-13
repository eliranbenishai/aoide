# 06 — An altered current playlist survives restart

**What to build:** Closing Tramp must never be how a listener loses work. Today an
ad-hoc pile of dropped tracks evaporates on quit, because only a path is remembered,
not a track list.

The listener drops tracks in, quits, reopens Tramp, and finds them still there and
still marked altered — so the protection from ticket 05 still applies.

Quitting must stay instant. Tramp deliberately exits rather than tearing down its
engines, and slow quit is a filed release blocker, so nothing here may add work to
shutdown. Persistence is continuous and debounced during the session instead.

**Blocked by:** 05

**Status:** done

- [x] An altered current playlist is restored after quitting and reopening
- [x] The restored playlist is still marked altered, so navigating away still prompts
- [x] Its origin, if it had one, is restored too
- [x] Persistence happens continuously and debounced during the session; nothing is written at quit
- [x] Quit latency does not regress
- [x] An unaltered current playlist restores as it does today, without becoming altered
- [x] A corrupt or unreadable persisted list falls back to an empty playlist rather than failing startup

## Comments

Suite went from 496 passing / 2 skipped / 6 failing to **519 passing / 2 skipped /
6 failing** — 23 new tests, no regressions. The 6 are the same Windows-authored
goldens failing by **exactly** the same margins (`title_bar_strip` 9.98% / 31134px,
`main_player_window` 8.55% / 24560px, `main_player_window_quiet` 7.16% / 20550px,
`screen_well` 0.48% / 1013px, `button_off_on` 0.41% / 626px, `shell_plate_rail`
0.12% / 403px). The 2 skips are still the host-guarded playlist goldens.
`flutter analyze` reports the same **23** pre-existing info issues, none in a file
this ticket touched.

### Where the list is kept

`lib/playlist/altered_playlist_store.dart` — `AlteredPlaylistStore` (abstract) with
`FileAlteredPlaylistStore` writing **`altered_playlist.json`** in the app support
dir, beside `settings.json`. It carries the whole track list plus the origin. Its
own file, not `session.json`: that store means "last session" and is the one
reader in the codebase with no error handling, so a malformed file there would
break startup. This one follows `FileSettingsStore` / `FilePlaylistCollectionStore`
and falls back to `AlteredPlaylist.empty` on **any** decode failure — missing file,
truncated JSON, a top-level array, even a directory where the file should be. A
single unreadable track row is skipped and the rest of the pile survives, the way
the collection index already treats a bad entry.

Reset Settings rewrites only `settings.json`, so a kept pile survives it like
installed skins and the collection do.

### The restore API

As ticket 05 asked for, and nothing wider:

```dart
void restoreAlteredTracks(List<Track> tracks, {String? sourcePath})
```

It sets the whole list and **raises** `altered`. There is no argument that could
ask for unaltered, so it can only ever raise — no `setAltered`, no `altered:` flag
on `setTracks`, and "only a whole write lowers it" stays structural rather than a
convention. Restoring through `setTracks` alone would have silently restored the
pile as unaltered and failed this ticket's second criterion.

Alongside it, `restoreCurrentPlaylist()` is what launch calls now: it prefers the
kept altered list and otherwise falls through to `restoreLastPlaylist()`, which is
the ordinary load and therefore leaves the playlist unaltered. `restoreLastPlaylist`
is untouched, and its own tests still pass unchanged.

### Why the debounce lives in the controller, not the host

The persistence is armed from `PlaylistController.notifyListeners`, which every
mutation already ends with, and debounced 2 seconds — the same figure as the host's
`_scheduleResumeSave` / `_persistResume` pair, which is the model the ticket named.
Putting it in the controller rather than the host widget is what makes it testable
at all: the host is bound to the multi-window and window-manager plugins and no
test can pump it, and the spec already says session logic that needs testing gets
extracted this way. The host's only change is the store it injects and the one call
site it restores from.

Two guards keep the writes honest. Nothing is written while the playlist is
unaltered **and** nothing is on disk, so a session of loads and selections never
touches the file. And when the state is lowered — a save, or a load replacing the
pile — the kept list is *forgotten*, so a saved playlist cannot come back altered
next launch.

### Nothing at quit

No quit hook, no dialog, no `await` on shutdown: `_quit` and `finishSessionQuit`
are byte-for-byte unchanged, and `_persistResume` is still the only thing quit
persists. A pending keep is dropped rather than flushed, including on `dispose`.
The cost is deliberate and stated in the code: an edit made in the last 2 seconds
before the process leaves is not kept. Flushing it would be exactly the quit-time
work commit `b6caff3` removed.

### What was verified, and how

23 tests. The store seam (11) is in the temp-directory shape of
`playlist_collection_store_test.dart`: round-trip of a pile with and without an
origin, `clear` forgetting it, clearing what was never kept, and four malformed
shapes. One asserts the store does **not** create `session.json`.

The controller seam (12) covers the restore method on its own (raises; raises over
an already-altered playlist; raises even for an empty list — it has no way to
lower), a pile surviving a simulated restart through a shared store, the origin
coming back with it, an unaltered playlist restoring by path and staying unaltered
with **zero** writes, a save and a load each forgetting the kept pile, an unaltered
session writing nothing at all, and a corrupt file on a real `FileAlteredPlaylistStore`
yielding an empty playlist without throwing.

Debounce is proved the way `equalizer_controller_test.dart` proves its persist
debounce — an injected debounce and real elapsed time, not `fake_async`, which is
not a declared dependency here and would have added an analyzer info issue to a
file this ticket touched. Twelve mutations in a row leave the store at **zero**
writes; 50ms later there is exactly **one**, holding the final list. A second test
pins the production default at 2 seconds so the injection cannot quietly drift.

Mutation check on the three assertions that matter, to show they bite:

- making `restoreAlteredTracks` lower instead of raise fails the five restore
  tests and only those;
- persisting per mutation instead of on the timer fails *a run of edits costs one
  write* and *teardown writes nothing*, and only those;
- dropping the `store.clear()` when the state is lowered fails *a saved playlist is
  not kept* and *loading over an altered playlist forgets the pile it replaced*,
  and only those.

All three reverted.

### What could NOT be verified

- **The quit path itself is not covered by a test.** `SessionHostApp` is bound to
  the multi-window and window-manager plugins and no test pumps it, so there is no
  way to assert from a test that quit writes nothing. What is asserted instead is
  the seam that owns the writing: `PlaylistController.dispose` drops a pending keep
  and writes nothing. The rest is by inspection — `_quit`, `finishSessionQuit`, and
  `_persistResume` are unchanged, and the only new call in the host is a store
  injection and `restoreCurrentPlaylist()` on the launch path.
- **Quit latency was not measured.** `tool/measure_quit_latency.sh` needs a built
  Linux release bundle and a running display, neither of which this environment
  has. The claim rests on the diff: nothing was added to shutdown, so there is
  nothing new for it to wait on. Worth one run of the harness on a machine that can
  before release.
- **The restart is simulated, not real.** Two controllers over one store stand in
  for two runs of the app. Restoring end to end through `_bootstrap` is host code,
  and untestable for the reason above.

### Decisions a later ticket should know

- **An empty altered playlist is not kept.** `AlteredPlaylist.isEmpty` — no tracks
  and no origin — reads the same as nothing kept, so `clear()` followed by a quit
  does not persist the emptiness, and the last-playlist path brings the old list
  back on launch. That is the safer direction (a listener regains a playlist rather
  than opening to a blank window after an accidental clear), and it keeps the store
  contract in the house pattern where the fallback value and "nothing here" are the
  same value. If a later ticket wants a cleared playlist to stay cleared, the change
  is to signal presence by the file's existence rather than by `isEmpty`.
- **The restore is gated on "resume last session"**, because it sits in the same
  `if (_resumeLastSession)` block `restoreLastPlaylist` already did. A listener who
  asked Tramp not to resume gets a clean start, including of the altered pile. The
  keeping is *not* gated — the file is written either way, so turning the
  preference back on finds whatever the last session left.
- **Metadata enrichment after a restore does not disturb the flag**, as ticket 05
  predicted: `updateTrackByPath` never raises, and the debounced keep simply
  rewrites the file with the filled-in durations.
- **Ticket 07 (create from all tracks)** still routes through `savePlaylistFile`,
  which now also forgets the kept list — one more reason not to route around it, or
  a saved playlist would come back altered after a restart.
- **Ticket 09 (create from selection)** must still not touch `PlaylistController`,
  and gets the right behaviour free: the current playlist stays altered and stays
  kept.
- **Ticket 10 (drag reorder)** wires `move`, which raises and therefore is kept
  automatically — no extra wiring for persistence.
- `PlaylistController` now overrides `dispose` (to cancel the keep timer). The host
  does not call it, deliberately: `_playback` holds the playlist and its own dispose
  is unawaited, so disposing the playlist controller there risks a notify on a
  disposed notifier. The process leaves without teardown anyway.
