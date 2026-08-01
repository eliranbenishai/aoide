# Tramp v1 — whole-branch review fix report

**Date:** 2026-08-01  
**Branch:** feat/tramp-v1

## Fixed (playback desync cluster)

1. **`currentTrack` / now-playing** — `PlaybackController` tracks `playingIndex` + `_playingPath` separately from playlist `selectedIndex`. Transport and OS media metadata follow the playing track; row highlight follows selection.

2. **`playPause` with mismatched selection** — When nothing is open or selection differs from playing index, `playPause` opens and plays the selected row (same as `playIndex(selected)`). Same-track toggle still pauses/resumes.

3. **`removeAt` while playing** — On single-track removal of the playing item, advances to the next track at the same index or stops if none remain. Bulk playlist replace stops playback.

4. **Tests** — Added unit tests for select-while-playing, select-then-play, remove-playing-advance, remove-last-stop; widget test for Space with selection mismatch.

## Documented (known v1 limits)

- Linux MPRIS stub (in-app keys when focused still work)
- Second-instance “Open with” not implemented (cold-start argv works)
- macOS/Linux packaging smoke not run on dev host

## Verification

- `flutter test` — 32 passed
- `flutter build windows --debug` — OK
