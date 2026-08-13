# Wire real usage counters into the About stats well

Status: done

Closed by the playlist-manager overhaul:
[ticket 12](../../playlist-manager/issues/12-about-collection-figures.md)
(playlists, tracks, total time) and
[ticket 13](../../playlist-manager/issues/13-count-spins.md) (spins).

## Problem

The About window shows a stats well — playlists, tracks, total time, spins —
styled as the machine's hour meter. Nothing measures those counters yet, so
`AboutWindow` falls back to `AboutStats.placeholder`
(`lib/domain/about_stats.dart`): 12 playlists, 1,284 tracks, 3 d 22 h, 4,096
spins. Plausible figures, but fiction.

Shipping fiction in a release would tell every listener the same four lies, so
the real counters must land — or the well must be hidden — before v1 ships.

## Acceptance

- [x] `AboutWindow` receives an `AboutStats` built from real data, with
  `measured` true.
- [x] Playlists / tracks / total time come from the playlist manager; spins come
  from playback (a track played to the end).
- [x] The About window is a secondary engine, so the numbers arrive over the
  session bus like every other snapshot (`AboutStatsEvent`, or folded into an
  existing snapshot event) — no direct controller access from the client.
- [x] Counters that need to survive restarts are persisted with the rest of the
  settings/resume state. **Deviation, deliberate:** the spin count went into its
  own `usage.json` rather than onto settings or the resume snapshot. It is
  history, not a preference — resetting settings must spare it — and folding it
  into settings would rewrite the whole preferences document every time a track
  ended. Ticket 13 records the reasoning.
- [x] `AboutStats.placeholder` is gone, or kept only as a test fixture. Kept as
  a fixture, referenced from the widget and golden tests and from nothing in
  `lib/`.

## Notes

- The placeholder is deliberately marked: `AboutStats.placeholder.measured` is
  false, so a caller (or a release check) can tell a stand-in from a reading.
- Design intent and the rejected layouts are in `.scratch/about-redesign/`.
