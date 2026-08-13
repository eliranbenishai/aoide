# 14 — Regenerate the playlist goldens on Windows

**What to build:** Bring the golden set back in line with the shipped Playlist
Manager, and remove the host guard added in ticket 02.

This repo's golden set is authored on Windows. Rebaselining on Linux would fold host
font rasterisation into the references, which is why the playlist goldens were guarded
rather than regenerated while the overhaul was in progress.

**This ticket needs a Windows host.** It cannot be completed by an agent on the Linux
development machine, which is why it is marked for a human rather than for an agent.

**Blocked by:** 02, 03, 05, 08, 09, 10, 11

**Status:** ready-for-human

- [ ] Playlist goldens regenerated on Windows against the finished Playlist Manager
- [ ] Both the expanded two-panel window and the windowshade state are covered
- [ ] A collection with entries, an empty collection, and a disabled entry are each covered by a golden or an explicit note saying why not
- [ ] The host guard from ticket 02 is removed
- [ ] The full suite passes on Windows with no golden failures
- [ ] Any remaining Linux-side failures are confirmed to be font rasterisation only, and that expectation is recorded
