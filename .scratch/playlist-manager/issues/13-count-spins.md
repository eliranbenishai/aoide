# 13 — Count spins

**What to build:** The fourth and last invented number in the About well becomes real.
A **spin** is one track played through to the end, so skipping around never inflates
it however late the skip comes. Each pass under repeat-one counts, because the track
genuinely played through.

The count is a lifetime total, so it survives restarts. It lives in its own small
usage store rather than in settings — it is history, not a preference, so resetting
settings must spare it, and keeping it out of settings avoids rewriting the whole
settings document every time a track ends.

**Blocked by:** 12

**Status:** ready-for-agent

- [ ] A track reaching its natural end increments the spin count
- [ ] Skipping to the next track never increments it, however close to the end
- [ ] Stopping never increments it
- [ ] Each repeat-one pass increments it
- [ ] The count survives a restart and reads as a lifetime total
- [ ] It persists in its own usage store, written debounced, not on the settings document
- [ ] Resetting settings leaves the count alone
- [ ] The count reaches the About window over the session bus with the other three figures
- [ ] A missing or corrupt usage store reads as zero rather than failing startup
- [ ] After this ticket the About well contains no invented figures at all
