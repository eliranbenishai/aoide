# Wire real usage counters into the About stats well

Status: needs-triage

Blocked by: playlist-manager overhaul (no ticket yet — this issue exists so the
placeholder does not ship unnoticed).

## Problem

The About window shows a stats well — playlists, tracks, total time, spins —
styled as the machine's hour meter. Nothing measures those counters yet, so
`AboutWindow` falls back to `AboutStats.placeholder`
(`lib/domain/about_stats.dart`): 12 playlists, 1,284 tracks, 3 d 22 h, 4,096
spins. Plausible figures, but fiction.

Shipping fiction in a release would tell every listener the same four lies, so
the real counters must land — or the well must be hidden — before v1 ships.

## Acceptance

- `AboutWindow` receives an `AboutStats` built from real data, with
  `measured` true.
- Playlists / tracks / total time come from the playlist manager; spins come
  from playback (a track played to the end).
- The About window is a secondary engine, so the numbers arrive over the
  session bus like every other snapshot (`AboutStatsEvent`, or folded into an
  existing snapshot event) — no direct controller access from the client.
- Counters that need to survive restarts are persisted with the rest of the
  settings/resume state.
- `AboutStats.placeholder` is gone, or kept only as a test fixture.

## Notes

- The placeholder is deliberately marked: `AboutStats.placeholder.measured` is
  false, so a caller (or a release check) can tell a stand-in from a reading.
- Design intent and the rejected layouts are in `.scratch/about-redesign/`.
