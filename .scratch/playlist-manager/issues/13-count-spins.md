# 13 — Count spins

**What to build:** The fourth and last invented number in the About well becomes real.
A **spin** is one track played through to the end, so skipping around never inflates
it however late the skip comes. Each pass under repeat-one counts, because the track
genuinely played through.

The count is a lifetime total, so it survives restarts. It lives in its own small
usage store rather than in settings — it is history, not a preference, so resetting
settings must spare it, and keeping it out of settings avoids rewriting the whole
settings document every time a track ends.

**Blocked by:** 12

**Status:** done

- [x] A track reaching its natural end increments the spin count
- [x] Skipping to the next track never increments it, however close to the end
- [x] Stopping never increments it
- [x] Each repeat-one pass increments it
- [x] The count survives a restart and reads as a lifetime total
- [x] It persists in its own usage store, written debounced, not on the settings document
- [x] Resetting settings leaves the count alone
- [x] The count reaches the About window over the session bus with the other three figures
- [x] A missing or corrupt usage store reads as zero rather than failing startup
- [x] After this ticket the About well contains no invented figures at all

## Comments

Delivered together with ticket 12 — it needs 12's `AboutStatsEvent` wiring. Test
and analyzer numbers are in [ticket 12's comments](12-about-collection-figures.md):
**636 passing / 2 skipped / 6 failing** at unchanged golden margins, analyzer at
**23**.

### Where a spin is counted

`PlaybackController._onCompleted()`, first line, before the repeat-mode switch.
That is the **only** end-of-stream hook, and nothing else reaches it: `next`,
`previous` and `stop` drive the engine directly. So "a skip never counts however
late" and "stopping never counts" are structural rather than branches that could
be got wrong later, and each repeat-one pass counts because each pass genuinely
completed. The last track of a list counts even though `_onCompleted` then stops
playback — it played through; the stop is what comes after.

### Its own store

`UsageCounters` / `UsageStore` / `FileUsageStore` in
`lib/platform/usage_store.dart` → `usage.json`, following
`session_resume_store.dart` and ticket 06's `altered_playlist_store.dart`:
`session.json` is not extended, and any decode failure falls back to
`UsageCounters.empty` so a hand-edited or truncated file costs the count rather
than the launch. A negative count reads as zero too — no machine has un-played
music.

Writes are debounced 2s inside the controller (`spinPersistDebounce`, injectable
and set to `Duration.zero` in tests), the same debounce and the same place the
altered current playlist keeps its own. `loadUsage()` reads once during
`_bootstrap`.

### Reset Settings, and how it is proved

`_handleResetSettings` rewrites `settings.json` and nothing else, so the
guarantee is structural — but the test does not take the host's word for it.
`test/platform/usage_store_test.dart` writes a count, then runs **what
`_handleResetSettings` actually does** (`FileSettingsStore.write(defaults)`)
against the same support directory, and asserts the count still reads back
*while the preference really did reset*. That is the same shape ticket 03 used
for the collection, and it is proved at the seam the guarantee lives at rather
than in a widget no test can pump.

### The one thing quit now does

Quit persists the resume snapshot; it now flushes the pending spin count beside
it (`_persistOnQuit`). Two small file writes, no teardown, so the quit budget is
unaffected — but without it a listener who closes Tramp within the debounce
would lose the track that just finished, and a lifetime total that drops the
last one is not a lifetime total.

**Measured, 2026-08-13.** "The quit budget is unaffected" was reasoning, not a
number, and the [quit-latency blocker](../../quit-latency/issues/01-fast-main-quit.md)
had been cleared *before* this second write existed. `tool/measure_quit_latency.sh`
on a fresh Linux release build now reports **72, 74, 74, 75, 79 ms** across five
runs against the 500ms budget — inside the 200ms stretch target, and faster than
the 194–460ms the blocker was closed on. The extra write costs nothing findable.

Unverified hypothesis for the *improvement*, offered for whoever next touches
this path: `_persistOnQuit` resolves the support directory to write, and commit
13cb01c stopped resolving it through `path_provider`'s FFI lookup into libgio on
every call. That would make both quit-path writes cheaper. Nobody has bisected
it, so treat it as a lead rather than a finding.

### Anything a later ticket should know

- `usage.json` is **history**. Anything added to it inherits the Reset Settings
  guarantee, and anything that needs resetting belongs in `settings.json`
  instead.
- The count reaches the About window through `AboutStatsEvent` with the other
  three figures, and only while that window is open — see ticket 12.
