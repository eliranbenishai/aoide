# Title-bar drag: punch, paint, and a failed cheap path

Read this before changing title-bar drag, playlist resize motion, `placePanels`, `applyPunch`, or `grabMouse`. The cheap path already shipped, felt wrong on KWin, and was undone. The constraints and measurements below are the starting map.

> **Corrected 2026-08.** This page used to say the jank was "compositor `setMask` + full panel paint". Measurement says the `setMask` half is free: dragging main with only main visible runs at **0.55 ms/move (~740 fps)** on KWin/Wayland with punch applied on every move. The whole cost was **CPU paint**, and almost all of that was one function — see [Paint cost is the drag cost](#paint-cost-is-the-drag-cost). Do not spend another week on punch deferral.

Domain words: **host window**, **panel**, **punch** — [`CONTEXT.md`](../../CONTEXT.md). Geometry law: [`.cursor/rules/compositor-geometry.mdc`](../../.cursor/rules/compositor-geometry.mdc).

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

## Paint cost is the drag cost

Measure with `--bench-drag` (below) before theorising. What the numbers say:

- **Punch is not the problem.** Main-only drag, punch applied on every move: **0.55 ms/move**. `QWaylandWindow::setMask` is a `QRegion` compare plus a handful of `wl_surface.set_input_region` requests, and on a toplevel it triggers no repaint at all.
- **Paint is the problem**, and two things made it enormous:
  1. **The build had no optimisation.** `build.sh` passed no `-O` flag and `CMakeLists.txt` set no default `CMAKE_BUILD_TYPE`, so the binary anyone dragged was `-O0`. That alone was ~12× on every paint. Both now default to optimised, and `--bench-chrome` fails the gate when `__OPTIMIZE__` is absent.
  2. **`gaussianBlur` dominated everything else.** A naive `double` separable convolution, `constScanLine()` per tap, radius `3σ` (73 taps at σ=12), two allocations per call. It was **99% of all paint cost**: ablating it took a full main paint from 282 ms to 2.6 ms.
- **Panels that re-rasterise per move pay that cost per move.** Main and EQ cached a chassis and ran zero blurs. Playlist, settings and about called `paintMockupWindow` with the default `BodyPaint::full`, which also sets `glow = true`. They now cache their whole paint, because none of them has per-frame content.

Per-repaint cost, KWin/Wayland, zoom 75%:

| Panel | Before (`-O0`, naive blur) | After (`-O2`, fast blur) |
|---|---|---|
| main | 0.5 ms (0 blurs) | 0.3 ms |
| equalizer | 1.4 ms (0 blurs) | 1.3 ms |
| settings | 27.6 ms (8 blurs, 92% blur) | 2.6 ms |
| playlist | 49.1 ms (18 blurs, 84% blur) | 8.7 ms |
| about | 200.3 ms (18 blurs, 98% blur) | 6.3 ms |

A main-panel drag translates the cluster, so **every visible panel repaints on every move** — the 5-panel drag cost is the sum of the column above. Per-move cost, same conditions, after both the paint fix and per-panel caching:

| Gesture | Before | After |
|---|---|---|
| main, main only | 0.55 ms (740 fps) | 0.51 ms (596 fps) |
| main, all five open | 281 ms (**3.5 fps**) | **4.7 ms (210 fps)** |
| playlist | 49.7 ms (20 fps) | 0.87 ms (475 fps) |
| settings | 27.9 ms (36 fps) | 0.25 ms (763 fps) |
| about | 200 ms (5 fps) | 0.35 ms (419 fps) |
| playlist resize | 157 ms (8.4 fps) | 20.8 ms (53 fps) |

Resize is the one gesture a cache cannot help: the size changes every move, so the panel is genuinely re-rasterised. It is bounded by playlist full-paint cost.

Also still true: `placePanels` CPU for 20 sibling moves is ~**193 µs**, and `HostWindowMoveTest::siblingDragDoesNotPayFullClusterPaint` bounds it at 10 ms. That test measures `placePanels` only — it does **not** exercise `paintEvent`, so it never saw any of the above.

Spectrum ticks (33 ms) and title-bar moves share main's live paint path, which is why `max` in a drag run is ~33 ms even when the median is under 1 ms.

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

Ranked by measured headroom. Prefer one seam per trial; restart `./build/tramp` each time (an already-running binary will not pick up `./build.sh`).

1. **Playlist full-paint cost (~8.7 ms).** The only thing still bounding resize. Non-blur work dominates now: ~15 rows of shaped text, 14–18 button faces, three wells, zebra bands, all recomputed per paint. Caching button faces or the empty list well by size would go straight into resize smoothness.
2. **Skip work that a pure move cannot change.** `applyHitCursor` (with a full `hitTest`) runs on every drag move; `placePanels` calls `show()` on all five panels and re-pushes the mask with no equality check; `fitClusterToHost` walks all five ids. Small next to paint, but free to remove.
3. **Cache what is re-derived per paint.** `QPixmap::fromImage(noiseTile())` allocates per shell paint; fonts, gradients and icon paths are rebuilt per call site. (`loadProximaMark()` used to decode a PNG from disk per about repaint — now cached.)
4. **Quiet the analyser during drag.** Spectrum/marquee ticks repaint main mid-gesture; that is the ~33 ms `max` in a drag run.

Not seams: **punch deferral** (measured free, and it caused the trails below); `startSystemMove` on this host (it would slide a virtual-desktop-sized toplevel); extra OS windows per panel (retired host shape, and Wayland has no `xdg_toplevel` set_position anyway).

## How to time a retry

Everything here is agent-runnable and writes to an isolated support dir, so it never touches the listener's settings.

```bash
# Per-move drag cost, per-panel repaint count/cost/blur share.
./build/tramp --bench-drag playlist --bench-moves 40 --bench-visible playlist
./build/tramp --bench-drag main --bench-moves 40 --bench-visible eq,playlist,settings,about
./build/tramp --bench-resize --bench-moves 40 --bench-visible playlist

tool/bench-drag-matrix.sh          # the whole matrix
TRAMP_BENCH_NO_BLUR=1 …            # ablate blur to see what is left
TRAMP_BLUR=exact …                 # exact kernel instead of the box approximation

QT_QPA_PLATFORM=offscreen ./build/tramp --bench-chrome   # paint budget + -O guard
tool/fidelity-diff.sh --baseline   # capture goldens before a chrome change
tool/fidelity-diff.sh              # RMSE / peak vs those goldens
tool/build-app.sh                  # app only, no test gate — fast measure loop
```

Offscreen runs give deterministic client-side cost; a run on the live session adds surface commit back-pressure. Neither replaces looking at it: drag main and a child on KWin with playlist + settings open, watch for trails in the vacated hole and in the gap between panels, and confirm desktop clicks still pass through the gaps at rest.

Build with `./build.sh` (Homebrew Qt on this machine). System `cmake` on PATH is not the project build.

## History

| Commit | What |
|---|---|
| `7ca6b62` | Static phosphor titles off the live path. **Still shipped.** |
| `5b7d0aa` | Deferred punch + skip live during title drag; `TRAMP_LEGACY_DRAG`. |
| `f1b166b` | Skip `grabMouse` on Wayland; defer punch from drag flags (grabber was a no-op). |
| `d9cdc81` | Undo punch deferral and live-skip. Wayland grab skip kept. |
| `feat/png-compilation` | Precompiled skin chrome to 3× PNG faces. Abandoned: it cached main's chassis, which was already cached and already 0.3 ms, while leaving the blur cost in playlist/settings/about. Marginal by construction. |
| — | Optimised build defaults, fast `gaussianBlur`, `--bench-drag` / `--bench-resize`, fidelity gate. Worst-case drag 281 → 27 ms. |
| — | Per-panel paint cache for playlist / settings / about. Worst-case drag 27 → 4.7 ms; guarded by `HostWindowMoveTest::movingAPanelDoesNotRerasteriseIt`. |

Session that produced the experiment: [title-bar drag cheap path](f946af79-c7ed-4e6b-b4ab-ff3cd6c6f626).
