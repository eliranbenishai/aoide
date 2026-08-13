# 06 — An altered current playlist survives restart

**What to build:** Closing Tramp must never be how a listener loses work. Today an
ad-hoc pile of dropped tracks evaporates on quit, because only a path is remembered,
not a track list.

The listener drops tracks in, quits, reopens Tramp, and finds them still there and
still marked altered — so the protection from ticket 05 still applies.

Quitting must stay instant. Tramp deliberately exits rather than tearing down its
engines, and slow quit is a filed release blocker, so nothing here may add work to
shutdown. Persistence is continuous and debounced during the session instead.

**Blocked by:** 05

**Status:** ready-for-agent

- [ ] An altered current playlist is restored after quitting and reopening
- [ ] The restored playlist is still marked altered, so navigating away still prompts
- [ ] Its origin, if it had one, is restored too
- [ ] Persistence happens continuously and debounced during the session; nothing is written at quit
- [ ] Quit latency does not regress
- [ ] An unaltered current playlist restores as it does today, without becoming altered
- [ ] A corrupt or unreadable persisted list falls back to an empty playlist rather than failing startup
