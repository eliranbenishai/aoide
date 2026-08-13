# 11 — Rename a saved playlist

**What to build:** The listener's collection reads as a list of names they chose
rather than a list of filenames. They rename a **saved playlist** to something
readable and that is what the row shows from then on.

The file's own name is never touched. Tramp holds a reference to a file the listener
owns, and reaching into their folders to rename it because they retitled a row would
break the promise the reference model makes.

**Blocked by:** 03

**Status:** ready-for-agent

- [ ] A saved playlist can be renamed from the collection panel
- [ ] The row shows the new name, and the collection re-sorts alphabetically under it
- [ ] The playlist file's name on disk is unchanged
- [ ] The rename survives a restart
- [ ] An entry with no override still shows a sensible name derived from its filename
- [ ] Clearing a name back to empty falls back to the filename rather than showing a blank row
- [ ] Two entries may carry the same display name without either being lost or merged, since identity is the path
- [ ] Renaming a disabled entry is allowed — it does not require reading the file
