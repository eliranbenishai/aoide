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

**Status:** ready-for-agent

- [ ] Adding a playlist file stores a reference to it and shows a row for it
- [ ] The playlist file is never copied, moved, or rewritten anywhere else
- [ ] Rows show a display name and the track count, ordered alphabetically by name
- [ ] Clicking a row loads that playlist's tracks into the current playlist
- [ ] Removing a row takes the entry out of the collection and never touches disk
- [ ] Adding a path already in the collection selects the existing row rather than duplicating it
- [ ] Paths are compared in a normalized form, so the same file reached by a differently written path is recognised as the same entry
- [ ] The collection panel carries its own controls, distinct from the footer's track-level add and remove
- [ ] The collection survives a restart
- [ ] Resetting settings leaves the collection intact, the way it already leaves installed skins intact
- [ ] The collection index holds only what the panel paints, so startup reads stay small
- [ ] Collection logic lives in an injectable module, not in the session host widget, and is covered by tests driving it against both a fake store and a temporary directory
- [ ] Collection operations reach the host as commands and state returns as a snapshot, following the existing playlist command and snapshot pattern, with round-trip coverage for the new messages
