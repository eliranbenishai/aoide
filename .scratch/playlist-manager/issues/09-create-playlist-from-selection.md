# 09 — Create a new playlist from the selected tracks

**What to build:** The listener pulls a shorter playlist out of a longer one — select
five tracks from a sixty-track playlist and keep them as their own **saved playlist**.

This is deliberately *not* the same operation as ticket 07. Only some of the current
tracks are being written, so the rest are still unsaved. Creating from a selection
must therefore leave the current playlist and its altered state completely alone, and
must not navigate anywhere — navigating would either discard the current playlist or
raise the very prompt this avoids. It creates the entry and highlights it.

**Blocked by:** 07, 08

**Status:** ready-for-agent

- [ ] Creating from a selection writes a playlist file containing exactly the selected tracks, in their current order
- [ ] A reference to that file joins the collection automatically
- [ ] The new entry is highlighted in the collection panel
- [ ] The current playlist's tracks are unchanged
- [ ] The current playlist's origin is unchanged
- [ ] The current playlist's altered state is unchanged — an altered playlist stays altered, an unaltered one stays unaltered
- [ ] Nothing is loaded or navigated to; no confirmation prompt appears
- [ ] The action is unavailable, or clearly inert, when no tracks are selected
- [ ] Cancelling the save dialog changes nothing at all
