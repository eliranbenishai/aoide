# Graphite Skin Delivery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship the graphite look as a PNG-first built-in skin, with zoom-only main/EQ windows and a freely resizable playlist well.

**Architecture:** A `GraphiteSkin` asset pack (2× PNGs + manifest rects) paints panel faces and control states. Panels stack invisible hit targets and coded overlays (spectrum, LCD/playlist text, thumbs) on top. `Transform.scale` zoom stays. Equalizer mode locks window size to the zoom step; playlist mode enables free resize around a never-stretched main canvas. Playlist chrome is 9-sliced.

**Tech Stack:** Flutter desktop, `window_manager`, existing `ZoomController` / panels, PNG assets under `assets/skin/graphite/`, optional Python/Pillow for crop scripts in `.scratch/graphite-skin/`.

**Design spec:** [`docs/superpowers/specs/2026-08-02-graphite-skin-delivery-design.md`](../specs/2026-08-02-graphite-skin-delivery-design.md)

## Global Constraints

- Dart SDK `>=3.5.0 <4.0.0`; do not raise the floor.
- Platforms: Windows, Linux, macOS. One window only — no detachable frames.
- Branding is `TRAMP`, all caps. Equalizer title reads `TRAMP EQUALIZER`. No lightning bolt anywhere.
- Logical canvases exact: main `812 × 242`, equalizer `812 × 206`, frame `6`, gutter `6`.
- Zoom steps exact: `[100, 125, 150, 200, 250, 300]` percent. Skin PNGs authored at **2×** and scaled with the step — no per-step exports.
- Main and equalizer canvases **never** freely resize or stretch (permanent).
- Playlist mode: window freely resizable (width + height); main stays fixed logical size × zoom, top-aligned.
- Equalizer mode: no edge resize; window size = stack × zoom step.
- Look source is skin PNGs. Do not substitute clean gradients or flat borders for grain/bevel/emboss.
- Code draws: spectrum, LCD/playlist text, hit targets, asset state selection, slider thumb position.
- Tests that assert text layout / goldens must call `loadTrampFonts()` from `test/support/test_fonts.dart`.
- Panel stack needs a `Material` ancestor (`TrampShell` supplies it).
- Every task ends with relevant tests green and `flutter analyze` clean on touched files before commit.

---

## File Structure

**Created:**

| Path | Responsibility |
|---|---|
| `assets/skin/graphite/**` | 2× PNG panel faces, control sheets, playlist 9-slice regions |
| `lib/ui/skin/graphite_skin.dart` | Asset path constants + logical hit/draw rects (manifest) |
| `lib/ui/skin/skin_image.dart` | `SkinImage` / `NineSliceSkin` widgets (device-pixel aware draw) |
| `lib/ui/skin/skin_button.dart` | Hit target + idle/pressed/active face swap |
| `lib/ui/skin/skin_slider.dart` | Groove (optional face) + positioned thumb + drag |
| `.scratch/graphite-skin/slice_mockup.py` | Crop/rebrand helpers from mockup → `assets/skin/graphite/` |

**Modified:**

| Path | Change |
|---|---|
| `pubspec.yaml` | Register `assets/skin/graphite/` |
| `lib/domain/tramp_settings.dart` | Persist `playlistWindowWidth` / `playlistWindowHeight` |
| `lib/platform/tramp_window.dart` | `setResizable`, playlist-aware sizing helpers |
| `lib/ui/tramp_shell.dart` | Mode-aware `DragToResizeArea`; size restore on region switch |
| `lib/ui/chrome/title_bar.dart` | Unchanged API; callers pass zoom± instead of maximize |
| `lib/ui/main_player/main_player_panel.dart` | Skin faces + skin controls; drop ZOOM dropdown; zoom± in title bar |
| `lib/ui/equalizer/equalizer_panel.dart` | Skin faces + skin controls |
| `lib/ui/playlist_panel.dart` | 9-slice chrome; fills expanded window |
| `lib/app.dart` | Wire playlist size persistence + resizable toggles |
| `docs/architecture.md` | Mark skin path implemented as tasks land |

**Retired as look source (after panels migrate):** painted faces in `MetalPanel` / `ChromeButton` / `ChromeSlider` for main/EQ/playlist chrome. Keep files only if still used by tests during migration; delete or gut when unused.

---

### Task 1: Skin pack scaffolding + main/EQ panel face crops

**Files:**
- Create: `assets/skin/graphite/main_face.png` (1624×484)
- Create: `assets/skin/graphite/equalizer_face.png` (1624×412)
- Create: `.scratch/graphite-skin/slice_mockup.py`
- Modify: `pubspec.yaml` (assets section)
- Test: `test/ui/skin/skin_assets_test.dart`

**Interfaces:**
- Consumes: `docs/mockups/graphite-chrome.png` (and/or `.scratch/mockup_full.png`) as crop source.
- Produces: PNGs at exact 2× logical sizes; `pubspec` lists `assets/skin/graphite/`.

- [ ] **Step 1: Write the failing asset dimension test**

```dart
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;

// If package:image is not desired, use a tiny custom PNG IHDR reader instead.
// Prefer zero new deps: decode IHDR only.

void main() {
  test('main_face is 1624x484', () {
    final bytes = File('assets/skin/graphite/main_face.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, 1624);
    expect(size.$2, 484);
  });

  test('equalizer_face is 1624x412', () {
    final bytes = File('assets/skin/graphite/equalizer_face.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, 1624);
    expect(size.$2, 412);
  });
}
```

Implement `readPngSize` in the same file by reading PNG IHDR (bytes 16–23 big-endian width/height after signature + IHDR chunk).

- [ ] **Step 2: Run test to verify it fails**

Run: `flutter test test/ui/skin/skin_assets_test.dart`

Expected: FAIL (missing files or wrong size)

- [ ] **Step 3: Add assets directory to pubspec**

```yaml
  assets:
    - lib/ui/chrome/logo.svg
    - assets/skin/graphite/
```

- [ ] **Step 4: Produce faces from the mockup**

Write `.scratch/graphite-skin/slice_mockup.py` that:

1. Loads `docs/mockups/graphite-chrome.png`.
2. Measures / uses known panel bounds (prior probes under `.scratch/graphite-chrome/probes/` if helpful).
3. Crops main and EQ panels, scales to 1624×484 and 1624×412 if needed.
4. Inpaints or overlays `TRAMP` / `TRAMP EQUALIZER` where the mockup says WINAMP; removes bolt pixels (clone from nearby metal — do not leave a hole).
5. Punches full alpha in the display-well interior rects that code will own (spectrum + LCD text area on main; leave grooves if thumbs will overlay). Document punched logical rects in comments in `graphite_skin.dart` (Task 2).

Run the script; write PNGs into `assets/skin/graphite/`.

Visual check: open PNGs — must show grain/bevels, not flat fills. If a crop is clean-gradient, redo from mockup.

- [ ] **Step 5: Run tests to verify they pass**

Run: `flutter test test/ui/skin/skin_assets_test.dart`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add pubspec.yaml assets/skin/graphite/main_face.png assets/skin/graphite/equalizer_face.png .scratch/graphite-skin/slice_mockup.py test/ui/skin/skin_assets_test.dart
git commit -m "feat(skin): add 2x main and equalizer panel face PNGs"
```

---

### Task 2: GraphiteSkin manifest + SkinImage

**Files:**
- Create: `lib/ui/skin/graphite_skin.dart`
- Create: `lib/ui/skin/skin_image.dart`
- Test: `test/ui/skin/graphite_skin_test.dart`
- Test: `test/ui/skin/skin_image_test.dart`

**Interfaces:**
- Consumes: asset paths from Task 1.
- Produces:
  - `abstract final class GraphiteSkin` with `static const mainFace = 'assets/skin/graphite/main_face.png';` (and peers), plus `static const mainDisplayWell = Rect.fromLTWH(...)` in **logical** pixels (812-space), matching punched regions.
  - `class SkinImage extends StatelessWidget` with `SkinImage({required String asset, required Size logicalSize, BoxFit fit = BoxFit.fill})` drawing the 2× asset into `logicalSize`.

- [ ] **Step 1: Write failing manifest tests**

```dart
import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';

void main() {
  test('main display well sits inside 812x242', () {
    final r = GraphiteSkin.mainDisplayWell;
    expect(r.left >= 0, isTrue);
    expect(r.top >= 0, isTrue);
    expect(r.right <= 812, isTrue);
    expect(r.bottom <= 242, isTrue);
  });

  test('main face asset path is under graphite skin', () {
    expect(GraphiteSkin.mainFace, startsWith('assets/skin/graphite/'));
  });
}
```

- [ ] **Step 2: Run test — expect FAIL (library missing)**

Run: `flutter test test/ui/skin/graphite_skin_test.dart`

- [ ] **Step 3: Implement GraphiteSkin + SkinImage**

```dart
// lib/ui/skin/graphite_skin.dart
import 'package:flutter/painting.dart';

abstract final class GraphiteSkin {
  static const mainFace = 'assets/skin/graphite/main_face.png';
  static const equalizerFace = 'assets/skin/graphite/equalizer_face.png';

  /// Logical rect cleared in the PNG for spectrum + LCD overlays.
  static const mainDisplayWell = Rect.fromLTWH(41, 41, 527, 137);
  // Adjust to match Task 1 punch; keep in sync with slice script comments.
}
```

```dart
// lib/ui/skin/skin_image.dart
import 'package:flutter/widgets.dart';

class SkinImage extends StatelessWidget {
  const SkinImage({
    super.key,
    required this.asset,
    required this.logicalSize,
    this.fit = BoxFit.fill,
  });

  final String asset;
  final Size logicalSize;
  final BoxFit fit;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: Image.asset(
        asset,
        fit: fit,
        filterQuality: FilterQuality.medium,
        gaplessPlayback: true,
      ),
    );
  }
}
```

Add a widget test that pumps `SkinImage` with `mainFace` inside a `MaterialApp` and expects one `Image`.

- [ ] **Step 4: Run tests — expect PASS**

Run: `flutter test test/ui/skin/`

- [ ] **Step 5: Commit**

```bash
git add lib/ui/skin/graphite_skin.dart lib/ui/skin/skin_image.dart test/ui/skin/graphite_skin_test.dart test/ui/skin/skin_image_test.dart
git commit -m "feat(skin): GraphiteSkin manifest and SkinImage"
```

---

### Task 3: SkinButton + SkinSlider

**Files:**
- Create: `lib/ui/skin/skin_button.dart`
- Create: `lib/ui/skin/skin_slider.dart`
- Create: control PNGs under `assets/skin/graphite/controls/` (transport idle/pressed minimum set for tests; expand in Task 6–7)
- Test: `test/ui/skin/skin_button_test.dart`
- Test: `test/ui/skin/skin_slider_test.dart`

**Interfaces:**
- Consumes: `SkinImage`, asset paths.
- Produces:
  - `SkinButton({required Size size, required String idleAsset, String? pressedAsset, String? activeAsset, bool active = false, VoidCallback? onPressed, required String semanticLabel})`
  - `SkinSlider({required Axis axis, required double value /*0..1*/, ValueChanged<double>? onChanged, ValueChanged<double>? onChangeEnd, required Size trackSize, String? grooveAsset, required String thumbAsset, Size thumbSize = const Size(17, 10)})`

- [ ] **Step 1: Write failing SkinButton test**

```dart
testWidgets('SkinButton calls onPressed', (tester) async {
  var taps = 0;
  await tester.pumpWidget(
    MaterialApp(
      home: SkinButton(
        size: const Size(69, 40),
        idleAsset: GraphiteSkin.transportPlayIdle,
        pressedAsset: GraphiteSkin.transportPlayPressed,
        onPressed: () => taps++,
        semanticLabel: 'Play',
      ),
    ),
  );
  await tester.tap(find.byType(SkinButton));
  expect(taps, 1);
});
```

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement SkinButton**

On pointer down show `pressedAsset` if set; when `active` show `activeAsset` if set; else `idleAsset`. Wrap with `Semantics` + `GestureDetector`. Stack: `SkinImage` then hit target filling `size`.

- [ ] **Step 4: Crop at least play idle/pressed PNGs** into `assets/skin/graphite/controls/` and add constants on `GraphiteSkin`. Dimensions: logical 69×40 → PNG 138×80.

- [ ] **Step 5: Write failing SkinSlider test** — drag changes value (mirror patterns in `test/ui/chrome/` slider tests if present; otherwise:

```dart
testWidgets('SkinSlider drag updates value', (tester) async {
  double? v;
  await tester.pumpWidget(
    MaterialApp(
      home: Center(
        child: SizedBox(
          width: 175,
          height: 12,
          child: SkinSlider(
            axis: Axis.horizontal,
            value: 0.25,
            trackSize: const Size(175, 12),
            thumbAsset: GraphiteSkin.sliderThumb,
            onChanged: (x) => v = x,
          ),
        ),
      ),
    ),
  );
  await tester.drag(find.byType(SkinSlider), const Offset(80, 0));
  expect(v, isNotNull);
  expect(v! > 0.25, isTrue);
});
```

- [ ] **Step 6: Implement SkinSlider** — reuse value mapping ideas from `lib/ui/chrome/chrome_slider.dart` (horizontal: x maps 0..1; vertical: y inverted). Paint thumb with `Positioned` over optional groove `SkinImage` or empty track.

- [ ] **Step 7: Tests PASS + commit**

```bash
git commit -m "feat(skin): SkinButton and SkinSlider with initial control crops"
```

---

### Task 4: Persist playlist window size + window resizable API

**Files:**
- Modify: `lib/domain/tramp_settings.dart`
- Modify: `lib/platform/tramp_window.dart`
- Modify: `test/domain/` or `test/platform/settings_store_test.dart` (extend)
- Test: `test/domain/tramp_settings_test.dart` (create or extend)

**Interfaces:**
- Consumes: existing `TrampSettings` JSON.
- Produces:
  - `TrampSettings.playlistWindowWidth` / `playlistWindowHeight` as `double?` (null = use default).
  - `Future<void> setTrampWindowResizable(bool resizable)` wrapping `windowManager.setResizable(resizable)`.
  - Default playlist size helper: e.g. width = zoomed main width, height = zoomed (frame*2 + main + gutter + 400).

- [ ] **Step 1: Failing round-trip test**

```dart
test('playlist window size round-trips in JSON', () {
  const s = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
    playlistWindowWidth: 900,
    playlistWindowHeight: 700,
  );
  final again = TrampSettings.fromJson(s.toJson());
  expect(again.playlistWindowWidth, 900);
  expect(again.playlistWindowHeight, 700);
});
```

- [ ] **Step 2: Implement fields** — ignore invalid/non-positive numbers in `fromJson`; omit nulls in `toJson`. Update `==` / `hashCode` / `copyWith`.

- [ ] **Step 3: Add `setTrampWindowResizable`**

```dart
Future<void> setTrampWindowResizable(bool resizable) async {
  await windowManager.setResizable(resizable);
}
```

- [ ] **Step 4: Tests PASS + commit**

```bash
git commit -m "feat(settings): persist playlist window size; window resizable API"
```

---

### Task 5: Shell mode-aware resize + region size snap

**Files:**
- Modify: `lib/ui/tramp_shell.dart`
- Modify: `lib/app.dart`
- Test: `test/ui/tramp_shell_window_mode_test.dart` (logic test without real window_manager where possible)

**Interfaces:**
- Consumes: `LowerRegion`, zoom factor, playlist size from settings, `setTrampWindowResizable`, `resizeTrampWindow`.
- Produces: When `lowerRegion == equalizer`, resizable false and size = EQ stack × zoom. When playlist, resizable true; `DragToResizeArea` enabled; stack lays out main at top (fixed width logical × zoom host) and playlist expanded.

Layout rules:

- Outer host width/height follow window.
- Main player child stays `812 * factor` wide visually via existing transform pattern — **do not** stretch main canvas.
- Playlist below gutter expands to remaining height; width fills content area minus frame.

- [ ] **Step 1: Extract pure helper (testable)**

```dart
// e.g. in lib/ui/window_layout.dart
Size eqModeWindowSize(double factor) { /* existing ZoomController.windowSizeFor logic for EQ height */ }

Size playlistModeWindowSize({
  required double factor,
  required double? storedWidth,
  required double? storedHeight,
}) { /* clamp to min: zoomed main width x zoomed (main+gutter+minLower) */ }
```

Write unit tests for mins/clamps.

- [ ] **Step 2: Wire TrampShell**

- Replace always-on `DragToResizeArea` with: wrap only when playlist mode (or `resizeEdgeSize: 0` when EQ).
- On region change callback (from app): call `setTrampWindowResizable` + `resizeTrampWindow`.
- Persist size on resize end in playlist mode (listen via `WindowListener` in `app.dart` or shell).

- [ ] **Step 3: Manual smoke on Windows** — EQ mode: edges don’t resize. PL mode: drag corner grows list; main artwork undistorted.

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(ui): EQ zoom-only window; playlist mode free resize"
```

---

### Task 6: Main player on skin + title-bar zoom±

**Files:**
- Modify: `lib/ui/main_player/main_player_panel.dart`
- Create: remaining control PNGs used by main (transport, shuffle/repeat, EQ/PL, OPEN, window buttons, volume thumb)
- Update: `GraphiteSkin` rects/constants
- Test: update `test/ui/` main player / golden tests

**Interfaces:**
- Consumes: `SkinImage`, `SkinButton`, `SkinSlider`, `GraphiteSkin`, `LcdText`, `SpectrumVisualizer`, `TrampMark`.
- Produces: Main canvas = stack of face + controls + overlays; title trailing = min, zoom− (`zoom.stepDown`), zoom+ (`zoom.stepUp`), close. Remove maximize handler and `_ZoomButton` dropdown.

- [ ] **Step 1: Crop remaining main controls** (same hybrid rules as Task 1). Add paths to `GraphiteSkin`.

- [ ] **Step 2: Rewrite `MainPlayerPanel` build** — `SizedBox(812,242)` → `Stack` with `SkinImage(mainFace)`, positioned `SkinButton`s/`SkinSlider`, display-well overlays for spectrum + texts. Keep existing controller callbacks.

- [ ] **Step 3: Update tests/goldens** — `loadTrampFonts()`; regenerate Windows goldens if present.

- [ ] **Step 4: Visual QA vs mockup at 100% and 200%** — grain/bevel must hold. Use vision subagent if helpful.

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ui): main player uses graphite skin; title-bar zoom controls"
```

---

### Task 7: Equalizer on skin + shade face

**Files:**
- Modify: `lib/ui/equalizer/equalizer_panel.dart`
- Create: `assets/skin/graphite/equalizer_shade_face.png`, EQ control PNGs, slider thumb shared
- Test: EQ widget tests / goldens

**Interfaces:**
- Same skin widgets as Task 6. Windowshade uses `equalizer_shade_face` (title-bar strip) when collapsed.

- [ ] **Step 1: Crop EQ controls + shade face**

- [ ] **Step 2: Rewrite EqualizerPanel stack** onto skin; keep `EqualizerController` wiring; gain labels stay `LcdText` / `Text` in phosphor.

- [ ] **Step 3: Tests + visual QA + commit**

```bash
git commit -m "feat(ui): equalizer panel uses graphite skin"
```

---

### Task 8: Playlist 9-slice skin + expanded layout

**Files:**
- Create: `lib/ui/skin/nine_slice_skin.dart` (if not in Task 2)
- Create: `assets/skin/graphite/playlist/` region PNGs (`nw`, `n`, `ne`, `w`, `e`, `sw`, `s`, `se`, `well`)
- Modify: `lib/ui/playlist_panel.dart`
- Test: `test/ui/skin/nine_slice_skin_test.dart`, playlist tests

**Interfaces:**
- Consumes: playlist size from shell.
- Produces: `NineSliceSkin({required PlaylistSlices slices, required Size size})` painting edges at fixed logical thicknesses and stretching/tiling well; child = scrolling track list.

- [ ] **Step 1: Failing test — nine-slice sizes to parent**

```dart
testWidgets('NineSliceSkin expands', (tester) async {
  await tester.pumpWidget(
    MaterialApp(
      home: SizedBox(
        width: 900,
        height: 500,
        child: NineSliceSkin(
          slices: PlaylistSlices.graphite,
          child: const SizedBox.shrink(),
        ),
      ),
    ),
  );
  expect(tester.getSize(find.byType(NineSliceSkin)), const Size(900, 500));
});
```

- [ ] **Step 2: Author playlist slices** from main/EQ grain samples (invent in-family). Corners fixed px; edges tile; well is dark inset texture that can stretch or tile. **No** flat `#1D2128` rectangle without grain.

- [ ] **Step 3: Implement NineSliceSkin + rewire PlaylistPanel**

- [ ] **Step 4: Stress** — load a large playlist (script or manual); resize window; scroll. Main canvas must stay undistorted.

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ui): 9-slice playlist skin with free-resize well"
```

---

### Task 9: Retire painted chrome look path + docs

**Files:**
- Remove unused painted usages from main/EQ/playlist; delete or narrow `metal_panel.dart` / `chrome_button.dart` / `chrome_slider.dart` if nothing imports them
- Modify: `docs/architecture.md` (skin path implemented; gaps cleared)
- Modify: goldens / README if they still describe painted chrome

- [ ] **Step 1: `dart` analyze imports** — ensure no panel depends on `MetalPanel` for faces.

- [ ] **Step 2: Delete dead code** or leave thin wrappers only if tests still need them (prefer delete).

- [ ] **Step 3: Update architecture module table** to Implemented for skin delivery items.

- [ ] **Step 4: Full test suite**

Run: `flutter test`

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git commit -m "refactor(ui): retire painted chrome look; docs match skin delivery"
```

---

## Spec coverage (self-review)

| Spec requirement | Task |
|---|---|
| PNG-first graphite skin, 2× master | 1–3, 6–8 |
| Hybrid crop + rebrand + no clean substitutes | 1, 6–8 |
| Code: spectrum, text, hits, thumbs | 6–8 |
| Main/EQ never stretch | 5–7 (permanent) |
| EQ mode zoom-only window | 5 |
| Playlist free resize W+H | 5, 8 |
| Persist playlist size | 4–5 |
| Zoom± replace maximize; drop ZOOM dropdown | 6 |
| Playlist invent in-family 9-slice | 8 |
| Side-by-side QA | 6–7 |
| Docs sync | 9 |

No TBD placeholders left in task steps. Types: `GraphiteSkin`, `SkinImage`, `SkinButton`, `SkinSlider`, `NineSliceSkin`, `TrampSettings.playlistWindowWidth/Height`, `setTrampWindowResizable` are consistent across tasks.
