# UI polish: title chrome, docking rules, EQ fill, taskbar

Date: 2026-08-09  
Status: Implemented  
Branch intent: follow-on to mockup multi-window redesign  
UI authority: [`player-mockup-2.html`](../../../player-mockup-2.html) for button chrome and spectrum gradient; product rules below override mockup where noted. Host is Qt 6 ([ADR 0016](../../adr/0016-qt-for-v1.md)).

Related: [`2026-08-08-mockup-multiwindow-redesign-design.md`](2026-08-08-mockup-multiwindow-redesign-design.md), [ADR 0006](../../adr/0006-multi-window-docking.md), [ADR 0007](../../adr/0007-code-constructed-mockup-chrome.md), [`architecture.md`](../../architecture.md), [`tramp-v1-spec.md`](../../tramp-v1-spec.md).

## Problem

After the multi-window cutover, five product gaps remain:

1. Title-bar window buttons do not match mockup bevel/inset chrome (icons are fine).
2. Docking feel is wrong vs the desired Winamp-like move/snap ownership.
3. EQ band faders lack a value fill; fill should use the spectrum cyan→magenta gradient.
4. EQ and playlist title bars still show logo + app name; only the role title is wanted.
5. Windows taskbar shows three buttons (main + EQ + PL); only main should appear.

Approach: extend existing seams (`DockingCoordinator`, `MockupTitleBar` / `_WinBtnPainter`, `_VTrackPainter`, session window options) — no docking rewrite, no architecture change beyond rule/docs updates.

## 1. Docking / snapping

Change pure layout rules in `DockingCoordinator` (session host/client keep applying frames).

### Move ownership

| Drag source | Behavior |
|-------------|----------|
| **Main** title bar | Translate every **visible** EQ and playlist window by the same delta, whether or not dock edges exist. Hidden windows stay put. Never snap; never create dock edges. |
| **EQ / playlist** title bar | Move only that window. Peel any dock edges for it as soon as drag starts (keep peel-on-drag). Shift-undock remains. |

### Snap (EQ / playlist finalize only)

- Snap runs only when the dragged window is EQ or playlist and `snap: true` (drag end).
- **EQ:** may snap to any side of main or playlist (four sides + existing near-1D / corner alignment within `snapThreshold`).
- **Playlist:** only **top/bottom** contact against other windows. No left/right side docking.
- On a successful top/bottom snap: also flush **left or right** if that edge is already within `snapThreshold`; otherwise keep horizontal offset.
- Prefer recording a second dock edge when both primary and orthogonal edges flush, so restore stays sticky on both axes.
- Dock edges still mean “snapped contact” for persistence and peel; they no longer gate whether main moves satellites — **visibility** does.

### Tests

Extend `test/ui/docking/docking_coordinator_test.dart` for: main always moves visible partners; main never snaps; PL rejects side snap; PL orthogonal flush when near; EQ still side-snaps.

## 2. Title-bar chrome

### Window buttons

Keep button set and icons (main: min / zoom− / zoom+ / close; EQ/PL: collapse + close). Retarget `_WinBtnPainter` to mockup `.wbtn` materials:

- Existing metal / close magenta face gradients
- Stronger **inset** highlight and shadow (top light lip, bottom dark lip) instead of a flat outline that reads as a hollow frame
- Close button uses the same bevel treatment on the magenta face
- Pressed state darkens / inverts as in the mockup

Update title-bar and window goldens after the visual change.

### EQ & playlist title content (product override vs HTML mockup)

Compact title mode on `MockupTitleBar`: for EQ and playlist, omit logo disc and “TRAMP” (+ version). Layout:

`[grip]  EQUALIZER | PLAYLIST EDITOR  [grip]  [buttons]`

Main keeps logo + TRAMP 1.0 + MAIN PLAYER + full button set.

## 3. EQ band fill

Extend `_VTrackPainter` in `mockup_equalizer.dart` (track + thumb remain):

- Fill from the **bottom of the track up to the thumb center** (same idea as horizontal vol/seek).
- Fill uses the **spectrum** multi-stop gradient (cyan → phos → deeper teal → accent/magenta), painted in track space and clipped to filled height so rising values reveal more of the magenta end.
- Thumb draws above the fill; zero notch remains.
- Applies to preamp and all frequency bands.

Update EQ goldens / widget tests with a non-zero band so fill is covered.

## 4. Single Windows taskbar button

- Main window: `skipTaskbar: false` (unchanged).
- EQ and playlist: `skipTaskbar: true` on create/show (session host / client / `WindowOptions` as appropriate).
- **Windows:** call `windowManager.waitUntilReadyToShow()` on each secondary engine **before** `setSkipTaskbar` — the plugin only creates `ITaskbarList` there; skipping it null-derefs and kills the process (`Lost connection to device`).
- Minimize-group behavior unchanged: main minimize still hides/restores visible secondaries; taskbar / Alt-Tab activate only main.
- Verify on Windows. Extras skip the taskbar as `Qt::Dialog` transients of main. Target remains **windows-x64**.
- macOS Dock / Linux taskbar parity: out of scope unless free.

## Documentation updates (same change)

| Doc | Update |
|-----|--------|
| This spec | Source of truth for the polish pass |
| `docs/architecture.md` | Docking move/snap ownership; taskbar; title compact mode; EQ fill |
| `docs/adr/0006-multi-window-docking.md` | Revised docking + taskbar decision |
| `docs/adr/0007-code-constructed-mockup-chrome.md` | Product overrides (compact EQ/PL titles; EQ fill beyond mockup HTML) |
| `docs/tramp-v1-spec.md` | Window/chrome product rules |
| `CONTEXT.md` | Docking / taskbar glossary |

## Out of scope

- Further mockup fidelity items beyond these five
- Rewriting docking as a satellite-offset model
- Changing zoom, shade, or close/quit semantics
- macOS/Linux taskbar/Dock hiding (follow-up)

## Success criteria

1. Title buttons read as recessed mockup `.wbtn` chrome in side-by-side with the HTML mockup.
2. Main title drag moves all visible windows; EQ/PL title drag moves only self; snap only from EQ/PL with PL top/bottom (+ optional orthogonal flush).
3. EQ faders show bottom→thumb spectrum-gradient fill.
4. EQ/PL title bars show role title only (no logo/wordmark).
5. Windows taskbar shows a single Tramp entry for the main player while EQ/PL are open.
6. Docs above reflect the shipped rules (no architecture drift).
