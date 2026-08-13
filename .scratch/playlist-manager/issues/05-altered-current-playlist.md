# 05 — Altered current playlist and the confirmation dialog

**What to build:** Tramp starts noticing when the listener has changed the current
playlist, and stops throwing that work away silently.

Changing the track list — adding, removing, reordering, sorting, clearing — makes it
an **altered current playlist**. Merely loading a saved playlist does not; that sets
the baseline. When the listener clicks another saved playlist while the current one
is altered, Tramp asks first, and offers to save rather than only to discard.

The prompt has to stay meaningful, so it must never appear when nothing was changed.

**Blocked by:** 03

**Status:** ready-for-agent

- [ ] Adding, removing, reordering, sorting, or clearing tracks marks the current playlist altered
- [ ] Loading a saved playlist leaves it unaltered, however many times it is loaded
- [ ] Navigating to another saved playlist while altered presents save, discard, and cancel
- [ ] Cancel holds default focus, so an idle keypress cannot discard anything
- [ ] Discard loads the new playlist and clears the altered state
- [ ] Save writes straight to the current playlist's origin when it has one, then loads the new playlist
- [ ] Save with no origin opens the save dialog; cancelling that dialog returns to the current playlist still altered, and does not fall through to loading
- [ ] The altered state clears only when the whole current track list has been written to a file that becomes its origin
- [ ] No prompt appears when the current playlist is unaltered
- [ ] Altered state lives with the current playlist on the host side and rides the playlist snapshot, since the window rendering it is a separate engine
