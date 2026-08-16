#!/usr/bin/env bash
# Measure title-bar drag lag: pointer X vs window X during a real OS drag.
#
# Drives the active X11 display (GDK_BACKEND=x11). Does not screenshot.
# Uses a throwaway HOME so it never reads the listener's data.
#
# Usage:
#   tool/measure_drag_latency.sh
#   TRAMP_SOLO_MAIN=1 tool/measure_drag_latency.sh
#   TRAMP_SOLO_MAIN=1 TRAMP_OPAQUE_WINDOWS=1 tool/measure_drag_latency.sh
#   TRAMP_SOLO_MAIN=1 TRAMP_DISABLE_LINUX_DRAG_POLL=1 tool/measure_drag_latency.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${TRAMP_DRAG_BUILD:-release}"
BUNDLE="$ROOT/build/linux/x64/${BUILD}/bundle"
BIN="$BUNDLE/tramp"

if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — build with: flutter build linux --${BUILD}" >&2
  exit 2
fi

if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY is unset; this harness needs X11" >&2
  exit 2
fi
if ! command -v xdotool >/dev/null || ! command -v wmctrl >/dev/null; then
  echo "need xdotool and wmctrl" >&2
  exit 2
fi

HOME_DIR="$(mktemp -d)"
LOG="$(mktemp)"
PID=""
cleanup() {
  if [[ -n "$PID" ]] && kill -0 "$PID" 2>/dev/null; then
    kill -KILL "$PID" 2>/dev/null || true
    wait "$PID" 2>/dev/null || true
  fi
  rm -rf "$HOME_DIR"
  rm -f "$LOG"
}
trap cleanup EXIT

export HOME="$HOME_DIR"
export XDG_CONFIG_HOME="$HOME_DIR/.config"
export XDG_DATA_HOME="$HOME_DIR/.local/share"
export XDG_CACHE_HOME="$HOME_DIR/.cache"
export GDK_BACKEND=x11
unset WAYLAND_DISPLAY WAYLAND_SOCKET
export LD_LIBRARY_PATH="$BUNDLE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBMPV_LIBRARY_PATH="${LIBMPV_LIBRARY_PATH:-$BUNDLE/lib/libmpv.so.2}"

echo "flags: SOLO_MAIN=${TRAMP_SOLO_MAIN:-0} OPAQUE=${TRAMP_OPAQUE_WINDOWS:-0} NO_POLL=${TRAMP_DISABLE_LINUX_DRAG_POLL:-0}"
echo "display: DISPLAY=$DISPLAY GDK_BACKEND=$GDK_BACKEND"

"$BIN" >"$LOG" 2>&1 &
PID=$!

WID=""
for _ in $(seq 1 80); do
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "FAIL: tramp exited before mapping a window" >&2
    tail -n 80 "$LOG" >&2
    exit 1
  fi
  WID="$(xdotool search --pid "$PID" --onlyvisible --name 'Tramp' 2>/dev/null | tail -n 1 || true)"
  if [[ -n "$WID" ]]; then
    break
  fi
  sleep 0.25
done

if [[ -z "$WID" ]]; then
  echo "FAIL: no visible Tramp window (DISPLAY=$DISPLAY)" >&2
  echo "wmctrl:" >&2
  wmctrl -l >&2 || true
  tail -n 40 "$LOG" >&2
  exit 1
fi

echo "window id=$WID"
xdotool windowactivate "$WID" || true
xdotool windowmove "$WID" 200 200 || true
sleep 0.4
echo "xwininfo:"
xwininfo -id "$WID" | grep -E 'Absolute|Relative|Width|Height|Map State' || true

python3 - "$WID" <<'PY'
import statistics, subprocess, sys, time

wid = sys.argv[1]

def xdo(*args, check=True):
    r = subprocess.run(
        ["xdotool", *args],
        capture_output=True,
        text=True,
        timeout=2,
    )
    if check and r.returncode != 0:
        raise RuntimeError(f"xdotool {' '.join(args)}: {r.stderr.strip()}")
    return r.stdout.strip()

def geom():
    out = xdo("getwindowgeometry", "--shell", wid)
    d = {}
    for line in out.splitlines():
        k, _, v = line.partition("=")
        d[k] = int(v)
    return d

def mouse():
    out = xdo("getmouselocation", "--shell")
    d = {}
    for line in out.splitlines():
        k, _, v = line.partition("=")
        if k in ("X", "Y"):
            d[k] = int(v)
    return d["X"], d["Y"]

def xwininfo_abs():
    r = subprocess.run(["xwininfo", "-id", wid], capture_output=True, text=True, check=True)
    ax = ay = None
    for line in r.stdout.splitlines():
        if "Absolute upper-left X" in line:
            ax = int(line.split()[-1])
        if "Absolute upper-left Y" in line:
            ay = int(line.split()[-1])
    return ax, ay

g = geom()
ax, ay = xwininfo_abs()
rel_x = max(40, g["WIDTH"] // 2)
rel_y = 18
print(f"geom X={g['X']} Y={g['Y']} {g['WIDTH']}x{g['HEIGHT']} xwininfo=({ax},{ay}) click_rel=({rel_x},{rel_y})")

xdo("mousemove", "--window", wid, str(rel_x), str(rel_y))
time.sleep(0.08)
xdo("mousedown", "--window", wid, "1")
time.sleep(0.12)

origin = geom()
mx0, my0 = mouse()
ox, oy = xwininfo_abs()
samples = []
t0 = time.perf_counter()
steps = 40
dx = 12
for i in range(steps):
    xdo("mousemove", str((ax or g["X"]) + rel_x + dx * (i + 1)), str((ay or g["Y"]) + rel_y))
    time.sleep(0.016)
    mx, my = mouse()
    w = geom()
    wx, wy = xwininfo_abs()
    mouse_dx = mx - mx0
    win_dx = (wx if wx is not None else w["X"]) - (ox if ox is not None else origin["X"])
    lag = mouse_dx - win_dx
    samples.append((time.perf_counter() - t0, mouse_dx, win_dx, lag))

xdo("mouseup", "1")
time.sleep(0.05)

lags = [s[3] for s in samples]
moved = [s[2] for s in samples]
print(f"samples={len(samples)} window_dx_end={moved[-1]} mouse_dx_end={samples[-1][1]}")
print(f"lag_px p50={statistics.median(lags):.1f} p95={sorted(lags)[int(0.95*(len(lags)-1))]:.1f} max={max(lags)} min={min(lags)}")
print(f"abs_lag_px mean={statistics.mean(abs(x) for x in lags):.1f}")
print("t_s,mouse_dx,win_dx,lag_px")
for t, md, wd, lag in samples:
    print(f"{t:.4f},{md},{wd},{lag}")
if moved[-1] == 0:
    sys.exit(3)
PY
