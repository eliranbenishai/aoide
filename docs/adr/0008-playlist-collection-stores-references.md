# 8. The playlist collection stores references; skins stay copies

Date: 2026-08-13

## Status

Accepted

## Context

The Playlist Manager introduces a **playlist collection**: a persisted set of
playlists the listener keeps, shown beside the current playlist.

Tramp already persists listener-supplied content exactly one way. `LookInstaller`
copies a skin pack into `<app-support>/skins/<id>/` and Tramp owns the copy from
then on; the original zip or folder is never read again. Mirroring that for
playlists is the obvious move, and it is the wrong one — which is precisely why
this ADR exists. A future reader who finds the two subsystems disagreeing will
otherwise assume one of them is a mistake.

A skin is inert content that Tramp renders through. A playlist is a live document
the listener also manages with a file manager and other players.

## Decision

- The collection stores **references** — normalized absolute paths to playlist
  files where the listener put them. Tramp never copies, moves, or rewrites the
  location of a playlist file.
- Creating a playlist writes the file wherever the listener chooses, then adds a
  reference to it automatically.
- Entry identity **is** the normalized absolute path. Adding a file already in
  the collection selects the existing entry instead of creating a twin.
- A display name may be overridden in Tramp's index only. Renaming an entry
  never renames the file on disk.
- Removing an entry never touches disk. There is no delete-the-file action.
- References are validated **after** the app has loaded, never on the startup
  path. A missing file makes the entry **disabled**: it cannot be loaded and can
  only be removed. Disabled is derived from the last validation pass rather than
  stored, so a file that comes back (remounted drive, reconnected share)
  re-enables its entry with no listener action.
- **Skins remain copies.** This is not an inconsistency to be tidied up later.

The principle that governs both, and should govern the next such choice:

> Tramp **references** what the listener authors and manages elsewhere, and
> **copies** what Tramp consumes as a dependency.

## Considered options

**Copy playlist files into Tramp's storage** (the skins model). Rejected: it
silently forks the listener's file. Edits made in Tramp and edits made in their
other tools diverge, with nothing in the UI to reveal that the file they think
they are curating is no longer the one being played.

**Hybrid — own playlists created in Tramp, reference the ones added from disk.**
Rejected as a distinction without a difference: creating a playlist already asks
the listener where to save it, so there is nothing left for Tramp to own. The
second storage path and the per-entry flag to select between them bought nothing.

### Why skins are not the same case

- A pack is installed from a zip, and a zip must be extracted before it can be
  used. Referencing the zip is not coherent — either extract on every launch, or
  cache the extraction, and a cached extraction *is* a copy.
- A pack is multi-file (`skin.json` plus fonts) whose paths must stay valid for
  the life of the process. A reference lets the listener half-break their own
  chrome by deleting one font.
- A broken skin reference changes the entire appearance of the app underneath the
  listener via the `builtin` fallback. A broken playlist reference costs exactly
  one unloadable row.
- Interop — the argument that decided the playlist case — has no analogue here.
  Every listener has other tools that manage M3U files. None has another tool
  managing Tramp skins.

## Consequences

- Deduplicated About counters cannot be derived from the referenced files at
  launch without the cold indexing pass this model forbids. Each entry's
  normalized track paths are therefore cached and maintained incrementally on
  add, on save, and when a file's modification time moves. Storage splits so the
  startup cost stays flat: `playlists.json` holds only what the left pane paints
  (name, path, count, duration, mtime); the per-entry track sets live in a
  companion file read lazily when the About stats are wanted.
- **Disabled playlists still count** toward the About stats. Unplugging a drive
  must not appear to rewrite the listener's history.
- An **altered current playlist** is persisted continuously and debounced, in the
  manner of `session_resume.json` — never at quit, because Tramp deliberately
  exits immediately instead of tearing down its engines.
- Reset Settings must spare the collection, exactly as it already spares
  installed skins. Content survives; preferences reset.
- None of this creates a track **Library**. That word stays reserved and unspent
  in [`CONTEXT.md`](../../CONTEXT.md); a catalog of playlists is not a catalog of
  tracks.
