# Assets needed from design

Tramp chrome polish — placeholders ship in-repo so the app stays usable.
Please replace these when you have a chance. Paths are relative to the repo root.

## Priority 1 — replace soon

| # | Asset | Path(s) | Notes |
|---|--------|---------|-------|
| 1 | Mute control sprites | `assets/skin/graphite/controls/mute_idle.png`, `mute_muted.png`, `mute_pressed.png` | Authored at **2×**, must be **pixel-aligned** to the empty well right of OPEN on `main_face.png` (logical ~736,191 size ~64×28). Until then the app draws a code speaker on the face well only — do **not** ship a second bezel that doesn't register to the face (that caused the black-block golden defect). |
| 2 | Playlist toolbar buttons | `assets/skin/graphite/controls/pl_load_idle.png`, `pl_load_pressed.png`, `pl_save_idle.png`, `pl_save_pressed.png`, `pl_add_idle.png`, `pl_add_pressed.png` | Target logical ~54×22 / 48×22 → **2×** ~108×44 / 96×44. Should read like the main **OPEN** recessed label well, not flat Material. Labels: **LOAD**, **SAVE**, **ADD**. |
| 3 | VOL face treatment | `assets/skin/graphite/main_face.png` (region above L/R wells, ~logical 603–722 × 80–96) | Optional bake of **VOL** into the face so it matches L/R engraving. App currently draws VOL as a code overlay; remove the overlay once face art lands. |

## Priority 2 — brand / OS

| # | Asset | Path(s) | Notes |
|---|--------|---------|-------|
| 4 | Windows app icon | `windows/runner/resources/app_icon.ico` | Export from `lib/ui/chrome/logo.svg` (multi-size ICO). |
| 5 | macOS app icons | `macos/Runner/Assets.xcassets/AppIcon.appiconset/app_icon_*.png` | Same source; sizes 16…1024. |

## Priority 3 — nice if you have time

| # | Asset | Path(s) | Notes |
|---|--------|---------|-------|
| 6 | Empty-playlist mark | Optional PNG or confirm code `TrampMark` is enough | Empty state uses compact mark + copy; a soft phosphor illustration could replace it. |
| 7 | Main OPEN↔mute bezel continuity | Region of `main_face.png` at logical ~588–800 × 191–220 | OPEN well and mute well should share one continuous metal language; mute sprites currently overlay the empty slot. |

## How placeholders were made

- Mute / playlist toolbar: `.scratch/graphite-skin/build_polish_chrome.py`
- VOL overlay: Flutter `Text` in `MainPlayerPanel` (not baked)

When you drop replacements, keep filenames and 2× pixel sizes (or update `GraphiteSkin` + hit targets in the same change).
