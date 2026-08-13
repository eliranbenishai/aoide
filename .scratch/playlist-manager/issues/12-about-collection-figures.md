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

**Status:** ready-for-agent

- [ ] The playlists figure counts entries in the collection, including disabled ones
- [ ] The tracks figure counts distinct tracks across the collection, each once, compared in normalized form
- [ ] Total time sums those distinct tracks' durations
- [ ] Disabled entries contribute to all three figures
- [ ] The unsaved current playlist does not contribute
- [ ] Figures are recomputed when a playlist is added, saved, removed, or found changed on disk — never by a full pass at launch
- [ ] Startup time does not regress; the per-entry track sets are read lazily, not at launch
- [ ] Figures reach the About window over the session bus, not by a client reaching into a controller
- [ ] The figures report as measured, and the placeholder is retired or kept only as a test fixture
- [ ] The existing four rows are reused — the well's layout and canvas do not change
