# 04 — Validate references after load; disable missing, refresh edited

**What to build:** Because the collection holds references to the listener's own
files, those files move, vanish, and get edited by other programs. Tramp checks on
them — after the app has finished loading, never during startup, so a large
collection never costs launch time.

When a playlist file has gone missing its entry becomes a **disabled playlist**: the
listener can see it and remove it, but not load it. When the file comes back — a
drive remounted, a share reconnected — the entry works again on its own, with no
action needed. When a playlist was edited in another program, its track count
catches up.

**Blocked by:** 03

**Status:** ready-for-agent

- [ ] Reference checking runs after the app has loaded and does not delay startup
- [ ] An entry whose file is missing renders distinctly as disabled
- [ ] A disabled entry cannot be loaded; attempting it does nothing destructive
- [ ] A disabled entry can still be removed from the collection
- [ ] Disabled state is derived from the most recent check rather than stored, so a returning file re-enables its entry with no listener action
- [ ] A playlist edited outside Tramp shows its updated track count and duration after the next check
- [ ] Only entries whose files actually changed are re-read
- [ ] A collection where every file is missing still lists every entry — nothing is silently dropped
