# Title-bar drag: punch, paint, and a failed cheap path

Read this before changing title-bar drag, playlist resize motion, `placePanels`, `applyPunch`, or `grabMouse`. The cheap path already shipped, felt wrong on KWin, and was undone. The constraints and measurements below are the starting map.

Domain words: **host window**, **panel**, **punch** — [`CONTEXT.md`](../../CONTEXT.md). Geometry law: [ADR 0019](../adr/0019-virtual-desktop-punched-host.md), [`.cursor/rules/compositor-geometry.mdc`](../../.cursor/rules/compositor-geometry.mdc).

## What “good” looks like

A later attempt is done when **all** of these hold on the pairing host (Fedora / Bazzite, Wayland / KWin), after a restart of `./build/tramp`:

- Dragging a panel leaves a clean canvas: no ghost rectangles, no vacated-hole trails.
- Punch stays a non-empty union of visible panel rects while the host is mapped (empty mask = full-desktop input on Qt Wayland).
- The host stays the virtual desktop: no `setGeometry` unless that rectangle changed; no `startSystemMove` on the host.
- Main title-bar drag still translates the cluster; other title-bar drags still move only that panel.
- Spectrum / live titles stay on the cheap path already shipped (`7ca6b62`): static phosphor titles on the chassis; marquee is clip+`drawText` only while `offset > 0`.
- The Qt Wayland line `This plugin supports grabbing the mouse only for popup windows` stays absent.

Offscreen QtTest is not that check. It honors `setGeometry` and often cannot set masks (`This plugin does not support setting window masks`). Green there is not proof the OS host or KWin punch looks right.

## Current shipped path (after the undo)

On each title-bar mouse-move:

1. `HostWindow` emits `nativeMoved`.
2. `TrampSession::windowMoved` runs docking + clamp/fit, then `applyDockToWindows` over all five panels.
3. `HostShell::placePanels` `setGeometry`s whoever moved, builds the panel-union mask, and **punches every call** (`QWidget::setMask` and `QWindow::setMask`). `updatePunch` is accepted and ignored.
4. Moved panels `paintEvent`. Main/EQ blit `chassis_` then a full **live** pass (clock, spectrum, overlay). `paintEvent` ignores the dirty rect. Playlist / settings / about have no chassis: full `BodyPaint::full`.
5. The host `update`s the old∪new widget rects and fills them transparent in its own `paintEvent`.

`grabPointerIfAllowed` still exists. On `wayland` it returns without `grabMouse`. On other platforms it grabs so a drag can continue if the pointer leaves the widget.

Relevant code: `src/host_shell_window.cpp` (`placePanels`, `applyPunch`), `src/host_window.cpp` (title drag, chassis/live paint), `src/session.cpp` (`windowMoved`, `applyDockToWindows`).

## Cost (why this feels heavy)

ADR 0019 already removed host resize during drag. The remaining tax is **per-move punch + per-move live paint**, not “moving a rectangle.”

| Observation | Number / note |
|---|---|
| Live paint with glow-blurred titles on every analyser tick | ~**104 ms/frame** vs ~**33 ms** with empty titles. Spectrum interval is 33 ms, so the analyser crawled. Fixed in `7ca6b62`. |
| Same live pass after titles left the chassis | ~**0.2 ms** with a long title. **Keep this.** |
| `placePanels` CPU, 20 sibling moves, offscreen | ~**193 µs** with punch every call; ~**73–76 µs** when punch was deferred. The human-visible jank is compositor `setMask` + full panel paint, not this timer. |
| Move test bound | `HostWindowMoveTest::siblingDragDoesNotPayFullClusterPaint` allows 10 ms for those 20 calls and prints `drag-path CPU placePanels`. |

Spectrum ticks and title-bar moves share the live paint path on main. A drag that also rebuilds punch every event is fighting the same frame budget.

## Failed experiment (do not re-land as-is)

Tried in `5b7d0aa` / `f1b166b`, undone in `d9cdc81` after the user saw trails on the canvas.

**Idea:** move widgets during drag; leave the punch put until mouse-up; blit chassis and skip the live pass on main/EQ while `draggingTitle_`; skip `applyHitCursor` and `schedulePersist` during the drag. Escape hatch was `TRAMP_LEGACY_DRAG=1` (removed).

**Why it looked cheap in tests:** `placePanels(..., false)` kept the previous mask. Offscreen CPU dropped. Punch-lag was the point.

**Why it failed on KWin:** `setMask` is not an input-only hint here. The host is a virtual-desktop-sized translucent surface. The punch is the hole the compositor actually shows and hits. When the panel widget moved and the mask stayed on the old rect:

- the vacated rectangle stayed on the canvas (ghost / trail)
- the new location was outside the current hole, so chrome and clear-of-old-pixels did not present as a clean move

`HostShell::paintEvent` filling `dirty` with transparent does not rescue that if the visual/input region is still the old union.

**Second trap (Wayland grab):** `grabMouse()` on a normal (non-popup) window prints `This plugin supports grabbing the mouse only for popup windows` and does nothing. The first cheap-path draft used `QWidget::mouseGrabber() == nullptr` as “pointer is free → punch now.” On Wayland the grab never succeeds, so punch stayed live for the whole drag and the warning spammed. `f1b166b` switched deferral to the panel’s own drag/resize flag and skipped `grabMouse` on Wayland. **That is when deferred punch actually ran on the user’s compositor — and the trails appeared.** “As expected, we took a step back.”

`mouseGrabber()` is not a Wayland-safe busy signal. If a retry needs “pointer is down,” use the panel flag (`draggingTitle_` / `resizingPlaylist_`), not the grabber.

## What to keep from that week

- **Marquee off the live glow path** (`7ca6b62`). Static titles on chassis; live marquee only while scrolling, no Gaussian blur. Reverting that brings the 104 ms tax back.
- **Skip `grabMouse` on Wayland** (still in `HostWindow::grabPointerIfAllowed`). The warning is Qt refusing the grab, not a product feature.
- **`updatePunch` on `placePanels`** remains in the signature. It is unused (always punch). A retry that honors it must prove on KWin that vacated pixels clear. The test `HostShellWindowTest::deferredPunchStillAppliesWhenLayoutAlreadyCaughtUp` currently asserts that `updatePunch=false` **still** follows the panel (flag ignored).

## Seams worth trying next

These were not isolated on KWin after the undo. Prefer one seam per trial; restart `./build/tramp` each time (an already-running binary will not pick up `./build.sh`).

1. **Punch every move; skip only live chrome while `draggingTitle_`.** Trails were blamed on stale punch. Chassis-only during drag may still be a win if punch stays current. Playlist/settings/about still have no chassis.
2. **Punch `old ∪ new` during the drag, snap to `new` on mouse-up.** Keeps the hole large enough to include both rects so the compositor can show the clear + the move. Input in the sliver between is the trade.
3. **Thin `windowMoved` during drag.** Today every move re-runs shade, playlist size, zoom, visibility, then `placePanels` for the whole set. A move-only `setGeometry` on the dragged panel (cluster translate for main) plus punch of the current union may be enough until mouse-up.
4. **Honor `QPaintEvent` / host dirty.** Main/EQ live-paint the full widget; the host already tracks old∪new. Clipping live paint to the well during drag is a smaller change than skipping punch.

`startSystemMove` on this host is not a seam: it would slide a virtual-desktop-sized toplevel (ADR 0019). Extra OS windows per panel are the retired 0017 shape.

## How to time a retry

- Offscreen: `HostWindowMoveTest` `placePanels` fprintf; `main.cpp` already has chassis/live paint loops for dump/bench.
- Device: drag main and a child on KWin with playlist + settings open; watch for trails in the vacated hole and in the gap between panels; confirm desktop clicks still pass through gaps at rest.
- Build with `./build.sh` (Homebrew Qt on this machine). System `cmake` on PATH is not the project build.

## History

| Commit | What |
|---|---|
| `7ca6b62` | Static phosphor titles off the live path. **Still shipped.** |
| `5b7d0aa` | Deferred punch + skip live during title drag; `TRAMP_LEGACY_DRAG`. |
| `f1b166b` | Skip `grabMouse` on Wayland; defer punch from drag flags (grabber was a no-op). |
| `d9cdc81` | Undo punch deferral and live-skip. Wayland grab skip kept. |

Session that produced the experiment: [title-bar drag cheap path](f946af79-c7ed-4e6b-b4ab-ff3cd6c6f626).
