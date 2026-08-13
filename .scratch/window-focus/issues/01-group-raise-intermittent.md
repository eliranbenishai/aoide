# 1 — Refocusing Tramp sometimes raises main alone, without EQ and PL

**Status:** needs-info

**Reported:** 2026-08-13, Linux (XWayland, main dev host). Observed while moving
between Tramp and other apps during Windows-VM preparation: clicking Tramp to come
back brought the main player forward and left the equalizer and playlist behind.
**Did not reproduce** when instrumented an hour later.

## What was ruled out

Instrumented `onWindowFocus` / `_raiseVisibleGroupWithMain` / the client raise handler
with `[DEBUG-focus]` prints (hot-reloaded into the running session, so live state was
preserved rather than reset) and exercised refocus several times. Every cycle was
healthy:

```
raise entry raising=false bootstrapped=true dragging=false minimizeActive=false
  eqVisible=true eqReady=true eqWindow=true plVisible=true plReady=true plWindow=true
client equalizer raise received
client equalizer presented visible=true focused=true
pushRaise(eq) returned
... same for playlist ...
main refocused
```

So none of these is the cause, at least in that state:

- `onWindowFocus` **is** delivered on Linux — despite `window_manager` missing
  `onWindowMoved` there, focus-in arrives (the plugin connects `focus-in-event`).
- No guard was blocking: `_nativeDragging` was `false`, so the "a drag never finalised
  and wedged the flag" theory is dead for this occurrence.
- The raise crosses the session bus and the secondaries really do come up —
  `visible=true focused=true` from the client's own `windowManager`.
- Nothing throws; there were no `raise(eq) failed` prints, before or during.

## Leading hypothesis

**Focus-stealing prevention refusing a timestampless present.** The pinned
`window_manager` fork implements `focus` as a bare
[`gtk_window_present()`](https://docs.gtk.org/gtk3/method.Window.present.html)
(`packages/window_manager/linux/window_manager_plugin.cc:80`), with no timestamp. GTK
documents that as discouraged precisely because EWMH window managers may ignore an
activation request whose timestamp is missing or stale, and Tramp's secondaries are
plain `_NET_WM_WINDOW_TYPE_NORMAL` toplevels with **no `WM_TRANSIENT_FOR`**, so the WM
has no grouping relationship telling it to raise them with main.

That fits both observations: refused while the app's last interaction timestamp was
stale (the reporter had been away in another app for a while), honoured during rapid
back-and-forth clicking, which is exactly the state it was instrumented in.

## Also found, and worth fixing regardless

Every refocus ping-pongs focus across three windows: the client raise handler ends in
`windowManager.focus()`, so raising EQ takes focus off main (the log shows
`host onWindowBlur` arriving mid-sequence), then PL takes it, then the host calls
`windowManager.focus()` to put it back. It works here, but it depends on the WM
honouring that final refocus, and it is a visible flicker.

`SessionBus.pushOrderTop` already exists for "show + order above peers **without**
focusing" and is what settings uses. Switching the EQ/PL raise to order-top semantics
would remove the ping-pong and stop the group raise depending on three activation
requests being honoured in order. Worth confirming order-top actually re-stacks an
already-visible window on Linux before relying on it.

## If it recurs

1. Note **how long** Tramp had been unfocused first — that is the discriminator for the
   timestamp hypothesis.
2. Re-apply the `[DEBUG-focus]` instrumentation (see commit history around this ticket)
   and look for `presented ... focused=false`, which would confirm a refused present.
3. Fixes in increasing order of blast radius: order-top instead of focus for the
   secondaries; timestamped present (`gtk_window_present_with_time`) in the fork; or
   `WM_TRANSIENT_FOR` on the secondaries so the WM raises the group itself. The last
   one fixes it independently of any focus event, but it changes stacking (transients
   sit above their parent) and needs a Windows/macOS answer too.

## Comments

- Instrumentation was removed after the session; nothing from it is committed. The repo
  is unchanged by this investigation, deliberately — no speculative fix landed for a
  bug that would not reproduce.
