# Fast main-window quit

Status: ready-for-agent

Priority: **release blocker** — must ship with near-instant close.

## Problem

Closing the main player (title-bar X / quit) takes **~4–5+ seconds** before the process exits. Reproduced on Linux under both `flutter run` (debug) and `flutter run -d linux --release`. Not acceptable for release.

## Root cause (current understanding)

`SessionHostApp._quit` (`lib/ui/session/session_host.dart`):

1. Optional confirm dialog
2. Persist resume snapshot
3. **Serially** `await` `session_shutdown` on EQ, playlist, then settings (`WindowController.invokeMethod`)
4. Each secondary handles `session_shutdown` by `setPreventClose(false)` + `windowManager.destroy()` (`session_client.dart`)
5. Then main `setPreventClose(false)` + `destroy()`

Linux teardown of each secondary Flutter engine is slow/messy (compositor shader cleanup failures, `RemoveWindow`, `g_mutex_clear` on uninitialised/locked mutex). Awaiting three destroys in order stacks that cost into a multi-second hang while the UI still looks “alive.”

## Acceptance

- Main close feels **instant** on Linux release (target: window gone and process exiting in well under **500ms** wall-clock from click when confirm-before-quit is off; stretch: &lt;200ms).
- Same path on Windows/macOS must not regress (no new multi-second hang).
- No orphaned secondary processes/windows after quit.
- Optional confirm-before-quit still works.

## Likely directions (pick one that meets acceptance)

- Fire secondary `session_shutdown` **in parallel** (still may be slow if each destroy is multi-second).
- **Do not await** full graceful destroy: hide/close secondaries best-effort, then exit the process (or destroy main immediately and let the OS reap children).
- Hybrid: best-effort parallel shutdown with a short timeout, then hard exit.

## Docs

- [`docs/architecture.md`](../../../docs/architecture.md#release-blockers)
- [`README.md`](../../../README.md#release-blockers)

## Comments

- 2026-08-12: Observed ~5s close on Linux release; product owner marked release blocker.
