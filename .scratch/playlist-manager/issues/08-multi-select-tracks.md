# 08 — Multi-select tracks

**What to build:** The listener can act on more than one track at a time. Today a tap
selects exactly one row and there is no way to select a range, so removing twelve
tracks means removing one twelve times.

Shift-click selects a contiguous range. The platform modifier click toggles
individual rows in and out, so a selection need not be contiguous. A plain tap
collapses back to a single row.

Selection is not a change to the playlist, so none of this marks the current playlist
altered. Acting on a selection — removing those tracks — does, by way of ticket 05.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] Shift-click selects every row between the anchor and the clicked row
- [ ] The platform modifier click adds and removes individual rows from the selection, using the convention native to each desktop
- [ ] A plain tap collapses the selection to the clicked row
- [ ] Every selected row renders as selected
- [ ] Removing tracks acts on the whole selection
- [ ] Selecting rows never marks the current playlist altered
- [ ] Selection state survives a playlist snapshot round trip, since it already rides one
- [ ] Shift-clicking with nothing previously selected behaves predictably rather than throwing
