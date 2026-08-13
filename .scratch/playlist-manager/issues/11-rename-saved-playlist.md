# 11 — Rename a saved playlist

**What to build:** The listener's collection reads as a list of names they chose
rather than a list of filenames. They rename a **saved playlist** to something
readable and that is what the row shows from then on.

The file's own name is never touched. Tramp holds a reference to a file the listener
owns, and reaching into their folders to rename it because they retitled a row would
break the promise the reference model makes.

**Blocked by:** 03

**Status:** done

- [x] A saved playlist can be renamed from the collection panel
- [x] The row shows the new name, and the collection re-sorts alphabetically under it
- [x] The playlist file's name on disk is unchanged
- [x] The rename survives a restart
- [x] An entry with no override still shows a sensible name derived from its filename
- [x] Clearing a name back to empty falls back to the filename rather than showing a blank row
- [x] Two entries may carry the same display name without either being lost or merged, since identity is the path
- [x] Renaming a disabled entry is allowed — it does not require reading the file

## Comments

Delivered with tickets 09 and 10 in one pass (shared test file). Suite:
**595 passing / 2 skipped / 6 failing**, up from 555, goldens at byte-identical
margins, analyzer at **23**. Figures in ticket 09's comments.

The plumbing really was mostly there: `SavedPlaylist.name`, `displayName`, and
`copyWith(clearName:)` all existed, and `copyWith(clearName:)` had no callers
until now. What was missing was a mutator, a command, and a control.

### How "the file is never renamed" is proved

Not by inspection — by a test that would catch it. *the row reads the new name
and the file keeps its own* writes a real playlist file to a temp directory,
renames the entry, and then asserts three things:

```dart
expect(File(path).existsSync(), isTrue);
expect(await File(path).readAsString(), contentsBefore);
expect(dir.listSync().whereType<File>().map((f) => p.basename(f.path)),
       ['dt-2019-03.m3u']);
```

The **directory listing** is the one that matters. `existsSync` alone would pass
a copy-and-keep; listing the whole directory catches a rename (the original
would be gone), a copy (a second file would appear), and a move (neither would
be there). The entry's `path` is asserted unchanged too, since a path is an
entry's identity. `PlaylistCollectionController.rename` reads no file at all,
which is also why renaming a **disabled** entry works — pinned by *a disabled
entry can be renamed without reading its file*, which renames an entry whose
file has been deleted and checks its figures and disabled state come through
untouched.

### The rest of the criteria

- **Re-sorts.** `rename` calls `_sort()` before writing the index, so the
  collection is ordered by what the listener *reads*. Dropping the `_sort()`
  fails *the collection re-sorts under the name the listener chose*; checked,
  reverted. Note the panel also sorts its own rows by display name, so the
  widget-level test passes either way — the module test is the one with teeth.
- **Survives a restart.** Pinned by writing through one controller and
  bootstrapping a second over the same store.
- **Clearing falls back to the filename.** Empty, whitespace-only, and null are
  all the same answer: the override is dropped via `copyWith(clearName: true)`
  and `displayName` reads the filename again. All three inputs are pinned, and
  the round-trip test carries `''` and `null` across the bus, because a cleared
  name arriving as a rename would be the obvious way to break this.
- **Duplicate display names.** Two entries renamed to the same string stay two
  entries with two distinct paths and two distinct rows.
- **No-ops.** A path the collection does not hold, and a rename to the name the
  entry already has, both return without writing the index or notifying.

### The dialog and the control

`showRenamePlaylistDialog` (new file, `lib/ui/playlist/rename_playlist_dialog.dart`)
is seeded with the name the row is currently reading — including the
filename-derived one, so clearing it is a visible, deliberate act rather than
something that happens by default — and the text is pre-selected so typing
replaces it. `null` back means cancel; `''` back means "read the filename
again". That distinction is the whole reason the dialog returns a nullable
string, and both paths are pinned at the widget seam.

Stateful only to own the `TextEditingController`: the field outlives the
`showDialog` future by one exit transition, so the controller has to be disposed
by the widget that holds it. The look is read *before* `showDialog` and passed
in, because the dialog's route sits above the window's `LookScope`.

The control is `pl-collection-rename` in the panel's control strip, beside
add/create/remove, greyed until a row is selected — matching how remove already
behaves. It needed a glyph the mockup's icon set does not have, so
`PlaylistCollectionRenameMark` is stroked in the same idiom as
`PlaylistCollectionCreateMark`: a nib over a baseline, writing on the row rather
than on the file. Making room for it is why the two creates folded into a menu;
see ticket 09.

### Notes for later tickets

- The panel's control strip is **full** at the minimum width. See ticket 09.
- `RenameSavedPlaylistCommand` carries the path because a path is an entry's
  identity and two rows may read the same name. Any future per-entry command
  must do the same — never address an entry by display name.
- ADR 0008 now has a second, sharper claim to make: the collection module writes
  exactly one kind of file (a new playlist at a path the listener chose in a
  save dialog) and renames none. If a later ticket wants to touch a referenced
  file, that is an ADR revision, not an implementation detail.
