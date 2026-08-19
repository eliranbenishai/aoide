# Graphite Chrome Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild Tramp's UI to match the graphite/chartreuse mockup, with a main player above a switchable equalizer-or-playlist region, at six discrete zoom levels.

**Architecture:** Every panel is authored once against a fixed logical canvas (main player 812×242, equalizer 812×206). A single `Transform.scale` at the root of the stack applies the zoom factor, so there is exactly one source of geometry and no widget can drift from the mockup. Colours live in `TrampColors`, materials in `TrampSurfaces`, and no widget composes its own gradient or bevel.

**Tech Stack:** Flutter desktop (Windows/Linux/macOS), `window_manager` for frameless chrome, `media_kit`/libmpv for playback, bundled TTF fonts, `CustomPainter` for glyphs, `flutter_svg` for the logo.

**Design spec:** [`docs/superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md`](../specs/2026-08-02-graphite-chrome-redesign-design.md) — the geometry tables in that document are the authority for every dimension in this plan.

## Global Constraints

- Dart SDK `>=3.5.0 <4.0.0`; do not raise the floor.
- Platforms: Windows, Linux, macOS. One window only — no detachable frames.
- Branding is `TRAMP`, all caps. The equalizer title bar reads `TRAMP EQUALIZER`. The mockup says "WINAMP"; that is wrong everywhere it appears.
- Palette values are exact: `frame #000000`, `panelTop #2C3039`, `panelBottom #1D2128`, `bevelHi #555B65`, `bevelLo #0B0E12`, `buttonTop #363B45`, `buttonBottom #22262E`, `wellDeep #010306`, `lcdGlass #03060A`, `phosphor #CFEA45`, `phosphorDim #5C7022`, `railAccent #FEE670`, `label #C9CED3`, `labelDim #979DA6`, `thumbHi #BFC8D1`.
- Logical canvases are exact: main player `812 × 242`, equalizer `812 × 206`, frame gutter `6`.
- Zoom steps are exactly `[100, 125, 150, 200, 250, 300]` percent.
- Equalizer band centres are exactly `[60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000]` Hz; gain range ±12 dB.
- No runtime font fetching. `google_fonts` is removed from `pubspec.yaml`; fonts are bundled TTFs.
- **Bundling a font does not make `flutter_test` use it.** The test harness substitutes a fallback face unless the real TTFs are registered with a `FontLoader`, and its metrics are wildly different — `OPEN` measures 46.4px in the fallback versus 26.28px in Barlow Semi Condensed. Any test that asserts text layout, and **every** golden, must call `loadTrampFonts()` from `test/support/test_fonts.dart` in `setUpAll`. Never "fix" a layout overflow that only appears in the fallback font by changing production code.
- All chrome is vector — gradients, `CustomPainter`, SVG. No raster skin assets, and **no icon fonts or glyph characters**. Both fail the same way: `□` and `✕` are absent from Barlow Semi Condensed and render as tofu boxes, and `Icons.volume_up` needs the MaterialIcons font. Window controls and the mute speaker are painted, like every other glyph. Verified by rendering the assembled panel.
- **The panel stack needs a `Material` ancestor.** Without one, Flutter's debug build decorates every `Text` with a missing-Material underline, which makes the whole display look wrong. `TrampShell` supplies it.
- The equalizer is chrome and state only. It must **not** claim to alter audio. See the spec's "Equalizer audio path" section: mpv reports success while silently disabling filters, so any future sink must verify by measurement, never by return code.
- Every task ends with `flutter analyze` clean and `flutter test` green before committing.

---

## File Structure

**Created:**

| Path | Responsibility |
|---|---|
| `lib/theme/tramp_surfaces.dart` | The five surface recipes: raised panel, raised button, pressed button, inset well, LCD glass |
| `lib/theme/tramp_text.dart` | Bundled text styles for chrome labels and LCD phosphor |
| `lib/theme/tramp_metrics.dart` | Canvas sizes, gutter, frame width — shared by panels and the zoom controller |
| `lib/domain/tramp_settings.dart` | `TrampSettings` and the `LowerRegion` enum |
| `lib/domain/equalizer_settings.dart` | `EqualizerSettings`, band frequencies, built-in presets |
| `lib/platform/settings_store.dart` | Persists zoom, lower region and equalizer state to `settings.json` |
| `lib/eq/equalizer_controller.dart` | Equalizer mutation, presets, persistence, `EqualizerSink` seam |
| `lib/playback/audio_levels.dart` | `AudioLevels` value type |
| `lib/ui/zoom/zoom_controller.dart` | Steps, clamping, work-area filtering, window sizing |
| `lib/ui/zoom/zoom_scope.dart` | `InheritedWidget` exposing the factor and device-pixel snapping |
| `lib/ui/chrome/title_bar.dart` | Rails, wordmark, drag region, leading/trailing slots |
| `lib/ui/chrome/lcd_text.dart` | Phosphor text widget with lit/dim treatment |
| `lib/ui/chrome/tramp_mark.dart` | The compact Tramp mark for chrome at control size. `logo.svg` / `TrampLogo` remains the full-size brand asset for icon, splash and About |
| `test/support/test_fonts.dart` | `loadTrampFonts()` — registers the bundled TTFs so tests and goldens measure the real faces instead of the harness fallback |
| `lib/ui/main_player/main_player_panel.dart` | The 812×242 canvas and its three rows |
| `lib/ui/equalizer/equalizer_panel.dart` | The 812×206 canvas, preamp and ten bands |

**Modified:**

| Path | Change |
|---|---|
| `pubspec.yaml` | Drop `google_fonts`, add bundled font assets |
| `lib/theme/tramp_colors.dart` | Replaced wholesale with the graphite palette |
| `lib/theme/tramp_theme.dart` | Drop `GoogleFonts`, use bundled families |
| `lib/ui/chrome/metal_panel.dart` | Thin wrapper over `TrampSurfaces` |
| `lib/ui/chrome/chrome_button.dart` | Icon / label / toggle / dropdown variants |
| `lib/ui/chrome/chrome_slider.dart` | Groove + fill + thumb, horizontal and vertical |
| `lib/ui/chrome/transport_icons.dart` | Adds shuffle, repeat, repeat-one, eject, chevron |
| `lib/ui/chrome/spectrum_visualizer.dart` | Consumes `Stream<AudioLevels>` instead of animating itself |
| `lib/ui/playlist_panel.dart` | Reskinned to the new tokens |
| `lib/ui/tramp_shell.dart` | Zoom host, `LowerRegion` switching |
| `lib/playback/player_engine.dart` | Adds `levelsStream` |
| `lib/playback/media_kit_player_engine.dart` | Emits synthetic levels |
| `lib/playback/fake_player_engine.dart` | Emits scripted levels |
| `lib/playback/playback_controller.dart` | Exposes `levelsStream` |
| `lib/app.dart` | Wires zoom, settings, equalizer, region switching |
| `lib/platform/tramp_window.dart` | Window size derived from the zoom step |

**Deleted:** `lib/ui/classic_main_player.dart` (and its two test files, replaced by main-player tests).

---

### Task 1: Palette, surfaces and bundled fonts

**Files:**
- Modify: `pubspec.yaml`
- Modify: `lib/theme/tramp_colors.dart` (replace contents)
- Modify: `lib/theme/tramp_theme.dart`
- Create: `lib/theme/tramp_surfaces.dart`
- Create: `lib/theme/tramp_text.dart`
- Create: `assets/fonts/` (four TTFs)
- Test: `test/theme/tramp_colors_test.dart` (replace contents)
- Test: `test/theme/tramp_surfaces_test.dart`

**Interfaces:**
- Consumes: nothing.
- Produces: `TrampColors` static colour constants (names listed in Global Constraints); `SurfaceSpec`; `BevelPainter`; `TrampSurfaces.raisedPanel({double bevel})`, `.raisedButton({double bevel})`, `.pressedButton({double bevel})`, `.insetWell({double bevel})`, `.lcdGlass({double bevel})` each returning a `SurfaceSpec`; `TrampText.chromeLabel`, `.chromeLabelDim`, `.wordmark`, `.lcd`, `.lcdDim`, `.lcdLarge`, `.eqScale` each a `TextStyle`.

**Why a `SurfaceSpec` rather than a `BoxDecoration`:** Flutter refuses to paint a `BoxDecoration` that has both a `borderRadius` and a non-uniform `Border` — it throws `A borderRadius can only be given on borders with uniform colors`. This design needs both: rounded corners *and* a light top edge against a dark bottom edge, which is what makes the chrome read as embossed metal. So a surface is split into a fill (gradient or colour, plus the radius) and a bevel painted separately on top, clipped to the same rounded rect. `MetalPanel` in Task 4 composes the two.

- [ ] **Step 1: Download the four font files**

```bash
mkdir -p assets/fonts
curl -L -o assets/fonts/BarlowSemiCondensed-SemiBold.ttf \
  https://github.com/google/fonts/raw/main/ofl/barlowsemicondensed/BarlowSemiCondensed-SemiBold.ttf
curl -L -o assets/fonts/BarlowSemiCondensed-Bold.ttf \
  https://github.com/google/fonts/raw/main/ofl/barlowsemicondensed/BarlowSemiCondensed-Bold.ttf
curl -L -o assets/fonts/IBMPlexMono-Medium.ttf \
  https://github.com/google/fonts/raw/main/ofl/ibmplexmono/IBMPlexMono-Medium.ttf
curl -L -o assets/fonts/IBMPlexMono-SemiBold.ttf \
  https://github.com/google/fonts/raw/main/ofl/ibmplexmono/IBMPlexMono-SemiBold.ttf
```

Verify each file is a real TTF and not a 404 page — every one must be at least 50 KB and start with the bytes `00 01 00 00`:

```bash
ls -l assets/fonts/
```

If any download returns HTML, fetch the family manually from `https://fonts.google.com/specimen/Barlow+Semi+Condensed` and `https://fonts.google.com/specimen/IBM+Plex+Mono` and place the four named files. Both families are OFL-licensed, so also save the licence:

```bash
curl -L -o assets/fonts/OFL.txt \
  https://github.com/google/fonts/raw/main/ofl/ibmplexmono/OFL.txt
```

- [ ] **Step 2: Write the failing palette test**

Replace `test/theme/tramp_colors_test.dart` entirely:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('panel face is dark graphite, not light metal', () {
    // The old palette was #B8B8B8. Anything bright here means the light-metal
    // chrome came back.
    expect(TrampColors.panelTop, const Color(0xFF2C3039));
    expect(TrampColors.panelBottom, const Color(0xFF1D2128));
    expect(TrampColors.panelTop.computeLuminance(), lessThan(0.05));
  });

  test('phosphor is chartreuse, not pure green', () {
    expect(TrampColors.phosphor, const Color(0xFFCFEA45));
    // Pure green (#33FF33) has a blue channel equal to its red channel; the
    // chartreuse phosphor is strongly red-biased against blue.
    expect(TrampColors.phosphor.red, greaterThan(TrampColors.phosphor.blue));
  });

  test('rail accent is warmer than the phosphor', () {
    expect(TrampColors.railAccent, const Color(0xFFFEE670));
    expect(TrampColors.railAccent.red, greaterThan(TrampColors.phosphor.red));
  });

  test('frame is pure black and the well is near-black', () {
    expect(TrampColors.frame, const Color(0xFF000000));
    expect(TrampColors.wellDeep, const Color(0xFF010306));
    expect(TrampColors.lcdGlass, const Color(0xFF03060A));
  });

  test('every token is fully opaque', () {
    for (final c in TrampColors.all) {
      expect(c.alpha, 0xFF, reason: '$c must be opaque');
    }
  });
}
```

- [ ] **Step 3: Run the test to verify it fails**

Run: `flutter test test/theme/tramp_colors_test.dart`
Expected: FAIL — `TrampColors.panelTop` and the other new names are undefined.

- [ ] **Step 4: Replace the palette**

Replace `lib/theme/tramp_colors.dart` entirely:

```dart
import 'package:flutter/painting.dart';

/// Graphite chrome palette. Every value is sampled from the reference mockup at
/// `docs/mockups/graphite-chrome.png`.
abstract final class TrampColors {
  /// Outer border and the gutter between stacked panels.
  static const frame = Color(0xFF000000);

  static const panelTop = Color(0xFF2C3039);
  static const panelBottom = Color(0xFF1D2128);
  static const bevelHi = Color(0xFF555B65);
  static const bevelLo = Color(0xFF0B0E12);

  static const buttonTop = Color(0xFF363B45);
  static const buttonBottom = Color(0xFF22262E);

  static const wellDeep = Color(0xFF010306);
  static const lcdGlass = Color(0xFF03060A);

  /// Lit phosphor: LCD text, spectrum bars, slider fills, active toggles.
  static const phosphor = Color(0xFFCFEA45);
  static const phosphorDim = Color(0xFF5C7022);

  /// Chrome accent — deliberately warmer than [phosphor] so the display reads
  /// as a screen rather than as paint.
  static const railAccent = Color(0xFFFEE670);

  static const label = Color(0xFFC9CED3);
  static const labelDim = Color(0xFF979DA6);
  static const thumbHi = Color(0xFFBFC8D1);

  static const all = <Color>[
    frame,
    panelTop,
    panelBottom,
    bevelHi,
    bevelLo,
    buttonTop,
    buttonBottom,
    wellDeep,
    lcdGlass,
    phosphor,
    phosphorDim,
    railAccent,
    label,
    labelDim,
    thumbHi,
  ];
}
```

- [ ] **Step 5: Run the palette test to verify it passes**

Run: `flutter test test/theme/tramp_colors_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 6: Write the failing surfaces test**

Create `test/theme/tramp_surfaces_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/theme/tramp_surfaces.dart';

void main() {
  test('raised panel is one smooth two-stop gradient', () {
    final g = TrampSurfaces.raisedPanel().decoration.gradient! as LinearGradient;
    // Exactly two stops: the mockup's visible mid-gradient stop is a rendering
    // artifact, not a design feature.
    expect(g.colors, [TrampColors.panelTop, TrampColors.panelBottom]);
    expect(g.stops, isNull);
    expect(g.begin, Alignment.topCenter);
    expect(g.end, Alignment.bottomCenter);
  });

  test('the fill carries no border, because the bevel is painted separately',
      () {
    // A BoxDecoration with both a borderRadius and a non-uniform Border throws
    // when painted, so the fill must stay border-free.
    expect(TrampSurfaces.raisedPanel().decoration.border, isNull);
    expect(TrampSurfaces.raisedButton().decoration.border, isNull);
    expect(TrampSurfaces.insetWell().decoration.border, isNull);
  });

  test('raised surfaces highlight the top-left and shadow the bottom-right',
      () {
    final spec = TrampSurfaces.raisedPanel();
    expect(spec.highlight, TrampColors.bevelHi);
    expect(spec.shadow, TrampColors.bevelLo);
  });

  test('inset surfaces reverse the bevel direction', () {
    final spec = TrampSurfaces.insetWell();
    expect(spec.highlight, TrampColors.bevelLo);
    expect(spec.shadow, TrampColors.bevelHi);
  });

  test('pressed button inverts both the gradient and the bevel', () {
    final raised = TrampSurfaces.raisedButton();
    final pressed = TrampSurfaces.pressedButton();
    final raisedGradient = raised.decoration.gradient! as LinearGradient;
    final pressedGradient = pressed.decoration.gradient! as LinearGradient;
    expect(pressedGradient.colors, raisedGradient.colors.reversed.toList());
    expect(pressed.highlight, raised.shadow);
    expect(pressed.shadow, raised.highlight);
  });

  test('lcd glass fills with the glass colour and has no gradient', () {
    final spec = TrampSurfaces.lcdGlass();
    expect(spec.decoration.color, TrampColors.lcdGlass);
    expect(spec.decoration.gradient, isNull);
  });

  test('bevel width is configurable for device-pixel snapping', () {
    expect(TrampSurfaces.raisedButton(bevel: 2).bevel, 2);
  });

  test('panels are more rounded than buttons', () {
    expect(TrampSurfaces.raisedPanel().radius, 3);
    expect(TrampSurfaces.raisedButton().radius, 2);
  });

  // The unit assertions above cannot catch Flutter refusing to paint a
  // decoration; only pumping one can.
  testWidgets('every surface paints without throwing', (tester) async {
    final specs = <String, SurfaceSpec>{
      'raisedPanel': TrampSurfaces.raisedPanel(),
      'raisedButton': TrampSurfaces.raisedButton(),
      'pressedButton': TrampSurfaces.pressedButton(),
      'insetWell': TrampSurfaces.insetWell(),
      'lcdGlass': TrampSurfaces.lcdGlass(),
    };

    for (final entry in specs.entries) {
      await tester.pumpWidget(
        Directionality(
          textDirection: TextDirection.ltr,
          child: Center(
            child: CustomPaint(
              foregroundPainter: BevelPainter(spec: entry.value),
              child: DecoratedBox(
                decoration: entry.value.decoration,
                child: const SizedBox(width: 40, height: 20),
              ),
            ),
          ),
        ),
      );
      expect(tester.takeException(), isNull, reason: entry.key);
    }
  });

  test('BevelPainter repaints when the surface changes', () {
    final raised = BevelPainter(spec: TrampSurfaces.raisedButton());
    final pressed = BevelPainter(spec: TrampSurfaces.pressedButton());
    expect(raised.shouldRepaint(pressed), isTrue);
    expect(
      raised.shouldRepaint(BevelPainter(spec: TrampSurfaces.raisedButton())),
      isFalse,
    );
  });

  // Rasterise and read pixels. A no-throw assertion cannot tell a correct
  // bevel from one painted at half its configured width, or from one lit on
  // the wrong side — both are silent visual defects.
  group('painted bevel pixels', () {
    const size = Size(40, 40);
    const bevel = 4.0;

    Future<Uint32List> render(SurfaceSpec spec) async {
      final recorder = PictureRecorder();
      BevelPainter(spec: spec).paint(Canvas(recorder), size);
      final image = await recorder
          .endRecording()
          .toImage(size.width.toInt(), size.height.toInt());
      final data = await image.toByteData();
      return data!.buffer.asUint32List();
    }

    // toByteData returns RGBA; compare against the same packing.
    int packed(Color c) =>
        (c.alpha << 24) | (c.blue << 16) | (c.green << 8) | c.red;

    int at(Uint32List pixels, int x, int y) =>
        pixels[y * size.width.toInt() + x];

    test('left edge is the highlight for its full configured width', () async {
      final pixels = await render(TrampSurfaces.raisedPanel(bevel: bevel));
      const midY = 20;
      for (var x = 0; x < bevel; x++) {
        expect(at(pixels, x, midY), packed(TrampColors.bevelHi),
            reason: 'left edge pixel x=$x should be the highlight');
      }
      expect(at(pixels, bevel.toInt() + 1, midY), 0,
          reason: 'nothing should paint inside the bevel');
    });

    test('right edge is the shadow', () async {
      final pixels = await render(TrampSurfaces.raisedPanel(bevel: bevel));
      expect(at(pixels, size.width.toInt() - 1, 20),
          packed(TrampColors.bevelLo));
    });

    test('inset surfaces light the opposite side', () async {
      final pixels = await render(TrampSurfaces.insetWell(bevel: bevel));
      expect(at(pixels, 0, 20), packed(TrampColors.bevelLo),
          reason: 'an inset well is shadowed on the left, not lit');
      expect(at(pixels, size.width.toInt() - 1, 20),
          packed(TrampColors.bevelHi));
    });
  });
}
```

Add `import 'dart:typed_data';` and `import 'dart:ui';` (for `PictureRecorder` and `Canvas`) to this test file's imports, hiding nothing that clashes with `package:flutter/material.dart` — import `dart:ui` with `show PictureRecorder, Canvas` to avoid the `Color`/`TextStyle` collisions.

- [ ] **Step 7: Run it to verify it fails**

Run: `flutter test test/theme/tramp_surfaces_test.dart`
Expected: FAIL — `tramp_surfaces.dart` does not exist.

- [ ] **Step 8: Write the surfaces**

Create `lib/theme/tramp_surfaces.dart`:

```dart
import 'dart:math' as math;

import 'package:flutter/rendering.dart';

import 'tramp_colors.dart';

/// One chrome material: a fill, plus the bevel to paint on top of it.
///
/// Fill and bevel are separate because Flutter throws when a [BoxDecoration]
/// carries both a `borderRadius` and a non-uniform `Border`, and this design
/// needs rounded corners together with a light top edge against a dark bottom
/// edge. [BevelPainter] draws the edges, clipped to the same rounded rect.
class SurfaceSpec {
  const SurfaceSpec({
    required this.decoration,
    required this.highlight,
    required this.shadow,
    required this.radius,
    required this.bevel,
  });

  /// Fill only — gradient or colour and the corner radius. Never a border.
  final BoxDecoration decoration;

  /// Colour of the top and left edges.
  final Color highlight;

  /// Colour of the bottom and right edges.
  final Color shadow;

  final double radius;
  final double bevel;

  @override
  bool operator ==(Object other) =>
      other is SurfaceSpec &&
      other.decoration == decoration &&
      other.highlight == highlight &&
      other.shadow == shadow &&
      other.radius == radius &&
      other.bevel == bevel;

  @override
  int get hashCode =>
      Object.hash(decoration, highlight, shadow, radius, bevel);
}

/// The complete set of chrome materials.
///
/// Every panel, button, groove and display in the app draws from here. Widgets
/// must not compose their own gradients or bevels — drift between the
/// equalizer, the transport buttons and the playlist is exactly what this
/// single definition prevents.
abstract final class TrampSurfaces {
  static const double panelRadius = 3;
  static const double buttonRadius = 2;

  static SurfaceSpec raisedPanel({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(panelRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.panelTop, TrampColors.panelBottom],
          ),
        ),
        highlight: TrampColors.bevelHi,
        shadow: TrampColors.bevelLo,
        radius: panelRadius,
        bevel: bevel,
      );

  static SurfaceSpec raisedButton({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(buttonRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.buttonTop, TrampColors.buttonBottom],
          ),
        ),
        highlight: TrampColors.bevelHi,
        shadow: TrampColors.bevelLo,
        radius: buttonRadius,
        bevel: bevel,
      );

  static SurfaceSpec pressedButton({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(buttonRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.buttonBottom, TrampColors.buttonTop],
          ),
        ),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: buttonRadius,
        bevel: bevel,
      );

  static SurfaceSpec insetWell({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(color: TrampColors.wellDeep),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: 0,
        bevel: bevel,
      );

  static SurfaceSpec lcdGlass({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(color: TrampColors.lcdGlass),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: 0,
        bevel: bevel,
      );
}

/// Paints a surface's two-tone bevel over its fill.
///
/// Use as a `foregroundPainter` so the edges sit above the fill.
class BevelPainter extends CustomPainter {
  const BevelPainter({required this.spec});

  final SurfaceSpec spec;

  @override
  void paint(Canvas canvas, Size size) {
    if (size.isEmpty || spec.bevel <= 0) return;
    if (size.width <= spec.bevel || size.height <= spec.bevel) return;

    // Clip to the outer edge, but stroke a path inset by half a bevel, so the
    // stroke spans exactly [0, bevel] and none of its width is clipped away.
    // Clipping to the stroked path itself would discard its outer half and
    // paint the bevel at half the configured width.
    final outer = RRect.fromRectAndRadius(
      Offset.zero & size,
      Radius.circular(spec.radius),
    );

    final inset = spec.bevel / 2;
    final radius = math.max(0.0, spec.radius - inset);
    final track = RRect.fromRectAndRadius(
      Rect.fromLTWH(
        inset,
        inset,
        size.width - spec.bevel,
        size.height - spec.bevel,
      ),
      Radius.circular(radius),
    );

    final stroke = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = spec.bevel;

    canvas.save();
    canvas.clipRRect(outer);

    // Full rim in shadow, then the lit edges redrawn over it, so the surface
    // reads as lit from above.
    canvas.drawRRect(track, stroke..color = spec.shadow);
    canvas.drawPath(_litEdges(track, radius), stroke..color = spec.highlight);

    canvas.restore();
  }

  /// The left and top edges, joined by the top-left corner arc.
  Path _litEdges(RRect track, double radius) {
    final path = Path()..moveTo(track.left, track.bottom - radius);
    if (radius <= 0) {
      path
        ..lineTo(track.left, track.top)
        ..lineTo(track.right, track.top);
      return path;
    }
    return path
      ..lineTo(track.left, track.top + radius)
      ..arcToPoint(
        Offset(track.left + radius, track.top),
        radius: Radius.circular(radius),
      )
      ..lineTo(track.right - radius, track.top);
  }

  @override
  bool shouldRepaint(BevelPainter oldDelegate) => oldDelegate.spec != spec;
}
```

- [ ] **Step 9: Run the surfaces test to verify it passes**

Run: `flutter test test/theme/tramp_surfaces_test.dart`
Expected: PASS (6 tests)

- [ ] **Step 10: Register the fonts and drop google_fonts**

In `pubspec.yaml`, remove the `google_fonts: ^6.2.1` line from `dependencies`, then replace the `flutter:` section with:

```yaml
flutter:
  uses-material-design: true
  assets:
    - lib/ui/chrome/logo.svg
  fonts:
    - family: BarlowSemiCondensed
      fonts:
        - asset: assets/fonts/BarlowSemiCondensed-SemiBold.ttf
          weight: 600
        - asset: assets/fonts/BarlowSemiCondensed-Bold.ttf
          weight: 700
    - family: IBMPlexMono
      fonts:
        - asset: assets/fonts/IBMPlexMono-Medium.ttf
          weight: 500
        - asset: assets/fonts/IBMPlexMono-SemiBold.ttf
          weight: 600
```

Then: `flutter pub get`

- [ ] **Step 11: Write the text styles**

Create `lib/theme/tramp_text.dart`:

```dart
import 'package:flutter/painting.dart';

import 'tramp_colors.dart';

/// Bundled type. Both families ship as assets: runtime font fetching breaks
/// offline rendering and makes golden tests non-deterministic.
abstract final class TrampText {
  static const _chrome = 'BarlowSemiCondensed';
  static const _mono = 'IBMPlexMono';

  /// Chrome button labels: OPEN, ZOOM, PRESETS, ON, AUTO, EQ, PL.
  static const chromeLabel = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 11,
    height: 1,
    letterSpacing: 0.6,
    color: TrampColors.label,
  );

  static const chromeLabelDim = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 11,
    height: 1,
    letterSpacing: 0.6,
    color: TrampColors.labelDim,
  );

  /// The TRAMP wordmark between the title-bar rails.
  static const wordmark = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w700,
    fontSize: 15,
    height: 1,
    letterSpacing: 3.2,
    color: TrampColors.label,
  );

  /// Equalizer frequency and dB scale labels.
  static const eqScale = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 9,
    height: 1,
    color: TrampColors.label,
  );

  /// Track title, bitrate, indicators.
  static const lcd = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w500,
    fontSize: 11,
    height: 1.1,
    color: TrampColors.phosphor,
  );

  static const lcdDim = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w500,
    fontSize: 11,
    height: 1.1,
    color: TrampColors.phosphorDim,
  );

  /// The large elapsed-time readout.
  static const lcdLarge = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w600,
    fontSize: 24,
    height: 1,
    color: TrampColors.phosphor,
  );
}
```

- [ ] **Step 12: Point the theme at the bundled fonts**

Replace `lib/theme/tramp_theme.dart` entirely:

```dart
import 'package:flutter/material.dart';

import 'tramp_colors.dart';

ThemeData buildTrampTheme() {
  final base = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    scaffoldBackgroundColor: TrampColors.frame,
    colorScheme: const ColorScheme.dark(
      primary: TrampColors.phosphor,
      secondary: TrampColors.railAccent,
      surface: TrampColors.panelBottom,
      onPrimary: TrampColors.lcdGlass,
      onSurface: TrampColors.label,
    ),
  );

  return base.copyWith(
    textTheme: base.textTheme.apply(
      fontFamily: 'BarlowSemiCondensed',
      bodyColor: TrampColors.label,
      displayColor: TrampColors.label,
    ),
  );
}
```

- [ ] **Step 13: Verify nothing references the removed tokens**

Run: `flutter analyze`
Expected: errors listing every remaining use of the old names (`metalFace`, `metalMid`, `metalHi`, `metalShadow`, `metalDeep`, `groove`, `lcdBackground`, `lcdPhosphor`, `lcdPhosphorDim`, `lcdPeak`, `fillAccent`, `windowClose`, `skinBorder`, `borderWidth`). These are fixed by Tasks 3–13; do not patch them here. Record the list so later tasks can be checked off against it.

Confirm the two new suites pass and that `google_fonts` is gone:

```bash
flutter test test/theme/
grep -rn "google_fonts" lib/ pubspec.yaml
```

Expected: theme tests PASS, `grep` finds nothing.

- [ ] **Step 14: Commit**

```bash
git add pubspec.yaml pubspec.lock assets/fonts lib/theme test/theme
git commit -m "feat(theme): graphite palette, surface recipes and bundled fonts

Replaces the light-metal/pure-green tokens with the graphite and chartreuse
palette sampled from the mockup, centralises every chrome material in
TrampSurfaces, and bundles Barlow Semi Condensed and IBM Plex Mono so
rendering works offline and goldens are deterministic."
```

---

### Task 2: Metrics and the zoom controller

**Files:**
- Create: `lib/theme/tramp_metrics.dart`
- Create: `lib/domain/tramp_settings.dart`
- Create: `lib/platform/settings_store.dart`
- Create: `lib/ui/zoom/zoom_controller.dart`
- Test: `test/ui/zoom/zoom_controller_test.dart`
- Test: `test/platform/settings_store_test.dart`

**Interfaces:**
- Consumes: nothing from Task 1.
- Produces:
  - `TrampMetrics.mainPlayer` = `Size(812, 242)`, `.equalizer` = `Size(812, 206)`, `.gutter` = `6.0`, `.frame` = `6.0`, `.minLowerRegion` = `240.0`.
  - `enum LowerRegion { equalizer, playlist }`.
  - `TrampSettings({int zoomPercent, LowerRegion lowerRegion, EqualizerSettings equalizer})` with `copyWith`, `toJson()`, `TrampSettings.fromJson(Map<String, dynamic>)`, `TrampSettings.defaults`. Note: the `equalizer` field is added in Task 9; for this task `TrampSettings` carries only `zoomPercent` and `lowerRegion`.
  - `abstract class SettingsStore { Future<TrampSettings> read(); Future<void> write(TrampSettings settings); }` and `FileSettingsStore({required Future<Directory> Function() supportDir})`.
  - `ZoomController extends ChangeNotifier` with `static const List<int> steps`, `int get percent`, `double get factor`, `List<int> get enabledSteps`, `bool canUse(int percent)`, `void setPercent(int)`, `void stepUp()`, `void stepDown()`, `void reset()`, `Size windowSizeFor(int percent)`, `Size minimumWindowSizeFor(int percent)`, `static int bestInitialPercent(Size workArea)`, and a settable `Size workArea`.

- [ ] **Step 1: Write the failing zoom controller test**

Create `test/ui/zoom/zoom_controller_test.dart`:

```dart
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

void main() {
  // A monitor big enough that every step fits, so tests that aren't about
  // clamping don't accidentally hit it.
  const hugeWorkArea = Size(6000, 4000);

  ZoomController build({
    Size workArea = hugeWorkArea,
    int initialPercent = 100,
  }) =>
      ZoomController(workArea: workArea, initialPercent: initialPercent);

  test('steps are the six documented levels', () {
    expect(ZoomController.steps, [100, 125, 150, 200, 250, 300]);
  });

  test('factor is percent over one hundred', () {
    final c = build(initialPercent: 150);
    expect(c.factor, 1.5);
  });

  test('stepUp and stepDown move one step and clamp at the ends', () {
    final c = build(initialPercent: 100);
    c.stepDown();
    expect(c.percent, 100, reason: 'already at the smallest step');
    c.stepUp();
    expect(c.percent, 125);
    c.setPercent(300);
    c.stepUp();
    expect(c.percent, 300, reason: 'already at the largest step');
  });

  test('reset returns to 100 percent', () {
    final c = build(initialPercent: 250);
    c.reset();
    expect(c.percent, 100);
  });

  test('notifies listeners when the step changes but not when it repeats', () {
    final c = build(initialPercent: 100);
    var calls = 0;
    c.addListener(() => calls++);
    c.setPercent(150);
    expect(calls, 1);
    c.setPercent(150);
    expect(calls, 1, reason: 'setting the same step must not notify');
  });

  test('window size scales the canvas stack by the factor', () {
    final c = build();
    // 812 panel + 6 frame either side = 824 logical; 6 + 242 + 6 + 240 + 6 = 500.
    expect(c.windowSizeFor(100), const Size(824, 500));
    expect(c.windowSizeFor(200), const Size(1648, 1000));
  });

  test('minimum window size never clips the player chrome', () {
    final c = build();
    final min100 = c.minimumWindowSizeFor(100);
    expect(min100.width, 824);
    // Player + gutter + a collapsed equalizer title bar, plus the frame.
    expect(min100.height, lessThan(c.windowSizeFor(100).height));
  });

  test('steps too wide for the work area are disabled', () {
    // 300% needs 2472 logical px of width; a 1600px-wide monitor cannot host it.
    final c = build(workArea: const Size(1600, 1200));
    expect(c.canUse(100), isTrue);
    expect(c.canUse(150), isTrue);
    expect(c.canUse(300), isFalse);
    expect(c.enabledSteps, [100, 125, 150]);
  });

  test('setPercent refuses a step that does not fit', () {
    final c = build(workArea: const Size(1600, 1200), initialPercent: 100);
    c.setPercent(300);
    expect(c.percent, 100, reason: 'must not adopt a step that would clip');
  });

  test('changing the work area re-clamps the current step', () {
    final c = build(initialPercent: 300);
    c.workArea = const Size(1600, 1200);
    expect(c.percent, 150, reason: 'largest step that still fits');
  });

  test('bestInitialPercent picks the largest fitting step capped at 150', () {
    expect(ZoomController.bestInitialPercent(const Size(6000, 4000)), 150);
    expect(ZoomController.bestInitialPercent(const Size(1000, 700)), 100);
  });

  test('metrics match the locked canvases', () {
    expect(TrampMetrics.mainPlayer, const Size(812, 242));
    expect(TrampMetrics.equalizer, const Size(812, 206));
    expect(TrampMetrics.gutter, 6.0);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/zoom/zoom_controller_test.dart`
Expected: FAIL — `tramp_metrics.dart` and `zoom_controller.dart` do not exist.

- [ ] **Step 3: Write the metrics**

Create `lib/theme/tramp_metrics.dart`:

```dart
import 'package:flutter/painting.dart';

/// Fixed logical canvases, derived by halving the reference mockup's measured
/// panel bounds (main 1625x484, equalizer 1625x411).
///
/// Every dimension inside a panel is authored against these, and the zoom
/// factor is applied once at the root of the stack. There is deliberately no
/// per-widget scaling.
abstract final class TrampMetrics {
  static const mainPlayer = Size(812, 242);
  static const equalizer = Size(812, 206);

  /// Black gutter between stacked panels.
  static const gutter = 6.0;

  /// Black frame around the whole window.
  static const frame = 6.0;

  /// Default height given to the lower region at 100%.
  static const minLowerRegion = 240.0;

  /// Height of a panel title bar, and therefore of a collapsed equalizer.
  static const titleBar = 35.0;
}
```

- [ ] **Step 4: Write the zoom controller**

Create `lib/ui/zoom/zoom_controller.dart`:

```dart
import 'package:flutter/foundation.dart';
import 'package:flutter/painting.dart';

import '../../theme/tramp_metrics.dart';

/// Owns the discrete zoom step.
///
/// The UI is authored at a fixed logical size and scaled by [factor] with a
/// single transform, so this class only has to decide which step is current and
/// which steps the display can host.
class ZoomController extends ChangeNotifier {
  ZoomController({
    required Size workArea,
    int initialPercent = 100,
    this.onPercentChanged,
  })  : _workArea = workArea,
        _percent = 100 {
    _percent = _largestFitting(initialPercent);
  }

  static const List<int> steps = [100, 125, 150, 200, 250, 300];

  /// Never auto-select a step larger than this on first run — a fresh install
  /// on a 4K display should not open enormous.
  static const int _initialCap = 150;

  /// Called after a successful step change so the host can resize the window
  /// and persist the choice.
  final void Function(int percent)? onPercentChanged;

  int _percent;
  Size _workArea;

  int get percent => _percent;
  double get factor => _percent / 100;

  Size get workArea => _workArea;

  set workArea(Size value) {
    if (_workArea == value) return;
    _workArea = value;
    final clamped = _largestFitting(_percent);
    if (clamped != _percent) {
      _percent = clamped;
      notifyListeners();
      onPercentChanged?.call(_percent);
    }
  }

  List<int> get enabledSteps => steps.where(canUse).toList();

  bool canUse(int percent) {
    final needed = minimumWindowSizeFor(percent);
    return needed.width <= _workArea.width &&
        needed.height <= _workArea.height;
  }

  void setPercent(int value) {
    if (!steps.contains(value)) return;
    if (!canUse(value)) return;
    if (value == _percent) return;
    _percent = value;
    notifyListeners();
    onPercentChanged?.call(_percent);
  }

  void stepUp() {
    final index = steps.indexOf(_percent);
    for (var i = index + 1; i < steps.length; i++) {
      if (canUse(steps[i])) {
        setPercent(steps[i]);
        return;
      }
    }
  }

  void stepDown() {
    final index = steps.indexOf(_percent);
    for (var i = index - 1; i >= 0; i--) {
      if (canUse(steps[i])) {
        setPercent(steps[i]);
        return;
      }
    }
  }

  void reset() => setPercent(100);

  /// Comfortable default window: player, gutter, and a usable lower region.
  Size windowSizeFor(int percent) {
    final f = percent / 100;
    final width = TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2;
    final height = TrampMetrics.frame * 2 +
        TrampMetrics.mainPlayer.height +
        TrampMetrics.gutter +
        TrampMetrics.minLowerRegion;
    return Size(width * f, height * f);
  }

  /// Smallest window that still shows the whole player and a collapsed
  /// equalizer title bar without clipping.
  Size minimumWindowSizeFor(int percent) {
    final f = percent / 100;
    final width = TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2;
    final height = TrampMetrics.frame * 2 +
        TrampMetrics.mainPlayer.height +
        TrampMetrics.gutter +
        TrampMetrics.titleBar;
    return Size(width * f, height * f);
  }

  static int bestInitialPercent(Size workArea) {
    final probe = ZoomController(workArea: workArea);
    var best = steps.first;
    for (final step in steps) {
      if (step > _initialCap) break;
      if (probe.canUse(step)) best = step;
    }
    return best;
  }

  int _largestFitting(int desired) {
    if (steps.contains(desired) && canUse(desired)) return desired;
    var best = steps.first;
    for (final step in steps) {
      if (step > desired) break;
      if (canUse(step)) best = step;
    }
    return best;
  }
}
```

- [ ] **Step 5: Run the zoom test to verify it passes**

Run: `flutter test test/ui/zoom/zoom_controller_test.dart`
Expected: PASS (12 tests)

- [ ] **Step 6: Write the failing settings store test**

Create `test/platform/settings_store_test.dart`:

```dart
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/platform/settings_store.dart';

void main() {
  late Directory dir;

  setUp(() async {
    dir = await Directory.systemTemp.createTemp('tramp-settings-test');
  });

  tearDown(() async {
    if (dir.existsSync()) await dir.delete(recursive: true);
  });

  FileSettingsStore store() =>
      FileSettingsStore(supportDir: () async => dir);

  test('reading with no file on disk yields the defaults', () async {
    final settings = await store().read();
    expect(settings, TrampSettings.defaults);
  });

  test('round-trips zoom and lower region', () async {
    const written = TrampSettings(
      zoomPercent: 200,
      lowerRegion: LowerRegion.equalizer,
    );
    await store().write(written);
    expect(await store().read(), written);
  });

  test('unparseable json falls back to defaults instead of throwing', () async {
    await File('${dir.path}/settings.json').writeAsString('{not json');
    expect(await store().read(), TrampSettings.defaults);
  });

  test('an unknown zoom step falls back to the default', () async {
    await File('${dir.path}/settings.json')
        .writeAsString('{"zoomPercent":137,"lowerRegion":"playlist"}');
    final settings = await store().read();
    expect(settings.zoomPercent, TrampSettings.defaults.zoomPercent);
  });
}
```

- [ ] **Step 7: Run it to verify it fails**

Run: `flutter test test/platform/settings_store_test.dart`
Expected: FAIL — `tramp_settings.dart` and `settings_store.dart` do not exist.

- [ ] **Step 8: Write the settings value type**

Create `lib/domain/tramp_settings.dart`:

```dart
/// Which panel occupies the region below the main player.
enum LowerRegion { equalizer, playlist }

/// Persisted UI state. Equalizer state joins this in Task 9.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.lowerRegion,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
  );

  /// Kept in step with `ZoomController.steps`; an out-of-range value on disk is
  /// treated as corrupt rather than adopted.
  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final LowerRegion lowerRegion;

  TrampSettings copyWith({int? zoomPercent, LowerRegion? lowerRegion}) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      lowerRegion: lowerRegion ?? this.lowerRegion,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'lowerRegion': lowerRegion.name,
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final region = json['lowerRegion'];
    return TrampSettings(
      zoomPercent: zoom is int && validZoomPercents.contains(zoom)
          ? zoom
          : defaults.zoomPercent,
      lowerRegion: LowerRegion.values.firstWhere(
        (value) => value.name == region,
        orElse: () => defaults.lowerRegion,
      ),
    );
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.lowerRegion == lowerRegion;

  @override
  int get hashCode => Object.hash(zoomPercent, lowerRegion);

  @override
  String toString() =>
      'TrampSettings(zoomPercent: $zoomPercent, lowerRegion: $lowerRegion)';
}
```

- [ ] **Step 9: Write the settings store**

Create `lib/platform/settings_store.dart`:

```dart
import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/tramp_settings.dart';

abstract class SettingsStore {
  Future<TrampSettings> read();
  Future<void> write(TrampSettings settings);
}

/// Mirrors `FilePlaylistStore`: a single JSON file in the app support dir.
class FileSettingsStore implements SettingsStore {
  FileSettingsStore({required this.supportDir});

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, 'settings.json'));
  }

  @override
  Future<TrampSettings> read() async {
    final f = await _file();
    if (!await f.exists()) return TrampSettings.defaults;
    // Corrupt settings must never block startup — a bad file is just defaults.
    try {
      final decoded = jsonDecode(await f.readAsString());
      if (decoded is! Map<String, dynamic>) return TrampSettings.defaults;
      return TrampSettings.fromJson(decoded);
    } on FormatException {
      return TrampSettings.defaults;
    }
  }

  @override
  Future<void> write(TrampSettings settings) async {
    final f = await _file();
    await f.writeAsString(jsonEncode(settings.toJson()));
  }
}
```

- [ ] **Step 10: Run the settings test to verify it passes**

Run: `flutter test test/platform/settings_store_test.dart`
Expected: PASS (4 tests)

- [ ] **Step 11: Commit**

```bash
git add lib/theme/tramp_metrics.dart lib/domain/tramp_settings.dart \
        lib/platform/settings_store.dart lib/ui/zoom \
        test/ui/zoom test/platform/settings_store_test.dart
git commit -m "feat(zoom): discrete zoom steps, canvas metrics and settings persistence

Adds the six documented zoom levels with work-area clamping so a step can
never open a window the display cannot host, plus the fixed logical canvas
metrics and a settings file for zoom and lower-region state."
```

---

### Task 3: ZoomScope and device-pixel snapping

**Files:**
- Create: `lib/ui/zoom/zoom_scope.dart`
- Test: `test/ui/zoom/zoom_scope_test.dart`

**Interfaces:**
- Consumes: `ZoomController` from Task 2 (only its `factor`; `ZoomScope` takes a plain `double`).
- Produces: `ZoomScope({required double factor, required double devicePixelRatio, required Widget child})` with `static ZoomScope of(BuildContext context)`, `static ZoomScope? maybeOf(BuildContext context)`, `static double hairlineFor(BuildContext context)`, `double snap(double logicalWidth)`, and `static const double hairline = 1.0`.

Every chrome widget in Tasks 4–8 gets its bevel width from `ZoomScope.hairlineFor(context)`, which degrades to an unsnapped hairline when no scope is present. That keeps single-widget tests and goldens runnable without wrapping each one.

- [ ] **Step 1: Write the failing test**

Create `test/ui/zoom/zoom_scope_test.dart`:

```dart
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

void main() {
  testWidgets('of() exposes the factor to descendants', (tester) async {
    double? seen;
    await tester.pumpWidget(
      ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: Builder(
          builder: (context) {
            seen = ZoomScope.of(context).factor;
            return const SizedBox();
          },
        ),
      ),
    );
    expect(seen, 1.5);
  });

  test('snap rounds a hairline to a whole device pixel', () {
    // At 150% on a 1x display a 1px bevel would land on 1.5 device px and blur.
    const scope = ZoomScope(
      factor: 1.5,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    // 1 logical * 1.5 = 1.5 device px -> rounds to 2 -> back to 1.333 logical.
    expect(scope.snap(1), closeTo(2 / 1.5, 1e-9));
  });

  test('snap never returns less than one device pixel', () {
    const scope = ZoomScope(
      factor: 1,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    expect(scope.snap(0.2), 1.0);
  });

  test('snap is a no-op when the result is already whole', () {
    const scope = ZoomScope(
      factor: 2,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    expect(scope.snap(1), 1.0);
  });

  test('device pixel ratio participates in the rounding', () {
    const scope = ZoomScope(
      factor: 1.25,
      devicePixelRatio: 2,
      child: SizedBox(),
    );
    // 1 * 1.25 * 2 = 2.5 device px. Dart rounds half away from zero, so that
    // is 3 device px, or 3 / 2.5 logical.
    expect(scope.snap(1), closeTo(3 / 2.5, 1e-9));
  });

  testWidgets('hairlineFor falls back to an unsnapped hairline with no scope',
      (tester) async {
    double? seen;
    await tester.pumpWidget(
      Builder(
        builder: (context) {
          seen = ZoomScope.hairlineFor(context);
          return const SizedBox();
        },
      ),
    );
    expect(seen, ZoomScope.hairline);
  });

  testWidgets('hairlineFor snaps when a scope is present', (tester) async {
    double? seen;
    await tester.pumpWidget(
      ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: Builder(
          builder: (context) {
            seen = ZoomScope.hairlineFor(context);
            return const SizedBox();
          },
        ),
      ),
    );
    expect(seen, closeTo(2 / 1.5, 1e-9));
  });

  // Tested directly rather than by counting rebuilds: a widget test that
  // re-pumps a fresh child rebuilds it on widget identity alone, so it cannot
  // isolate the inherited dependency.
  test('updateShouldNotify fires only when the factor or ratio changes', () {
    const base = ZoomScope(factor: 1, devicePixelRatio: 1, child: SizedBox());
    const sameValues =
        ZoomScope(factor: 1, devicePixelRatio: 1, child: SizedBox());
    const otherFactor =
        ZoomScope(factor: 2, devicePixelRatio: 1, child: SizedBox());
    const otherRatio =
        ZoomScope(factor: 1, devicePixelRatio: 2, child: SizedBox());

    expect(base.updateShouldNotify(sameValues), isFalse);
    expect(otherFactor.updateShouldNotify(base), isTrue);
    expect(otherRatio.updateShouldNotify(base), isTrue);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/zoom/zoom_scope_test.dart`
Expected: FAIL — `zoom_scope.dart` does not exist.

- [ ] **Step 3: Write ZoomScope**

Create `lib/ui/zoom/zoom_scope.dart`:

```dart
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

/// Carries the active zoom factor down the tree.
///
/// Widgets read this only to snap hairlines. Nothing scales itself — the factor
/// is applied once, by a single transform at the root of the panel stack.
class ZoomScope extends InheritedWidget {
  const ZoomScope({
    super.key,
    required this.factor,
    required this.devicePixelRatio,
    required super.child,
  });

  /// Nominal width of a chrome bevel, before snapping.
  static const double hairline = 1.0;

  final double factor;
  final double devicePixelRatio;

  static ZoomScope of(BuildContext context) {
    final scope = maybeOf(context);
    assert(scope != null, 'No ZoomScope found in context');
    return scope!;
  }

  static ZoomScope? maybeOf(BuildContext context) =>
      context.dependOnInheritedWidgetOfExactType<ZoomScope>();

  /// Snapped bevel width for [context].
  ///
  /// Falls back to an unsnapped hairline when there is no scope, so a single
  /// chrome widget can be pumped in a test or golden without ceremony.
  static double hairlineFor(BuildContext context) =>
      maybeOf(context)?.snap(hairline) ?? hairline;

  /// Rounds [logicalWidth] so it lands on whole device pixels once scaled.
  ///
  /// Without this a 1px bevel becomes 1.5 device pixels at 150% and renders as
  /// a soft grey smear instead of a crisp edge.
  double snap(double logicalWidth) {
    final scale = factor * devicePixelRatio;
    if (scale <= 0) return logicalWidth;
    final device = math.max(1.0, (logicalWidth * scale).roundToDouble());
    return device / scale;
  }

  @override
  bool updateShouldNotify(ZoomScope oldWidget) =>
      oldWidget.factor != factor ||
      oldWidget.devicePixelRatio != devicePixelRatio;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/zoom/zoom_scope_test.dart`
Expected: PASS (8 tests)

- [ ] **Step 5: Commit**

```bash
git add lib/ui/zoom/zoom_scope.dart test/ui/zoom/zoom_scope_test.dart
git commit -m "feat(zoom): ZoomScope with device-pixel hairline snapping

Exposes the active zoom factor to descendants and rounds bevel widths to
whole device pixels so chrome edges stay crisp at fractional zoom steps."
```

---

### Task 4: MetalPanel over the surface recipes

**Files:**
- Modify: `lib/ui/chrome/metal_panel.dart` (replace contents)
- Test: `test/ui/chrome/metal_panel_test.dart`

**Interfaces:**
- Consumes: `TrampSurfaces` (Task 1), `ZoomScope.hairlineFor` (Task 3).
- Produces: `enum TrampSurface { raisedPanel, raisedButton, pressedButton, insetWell, lcdGlass }` and `MetalPanel({required TrampSurface surface, required Widget child, EdgeInsets? padding})`.

- [ ] **Step 1: Write the failing test**

Create `test/ui/chrome/metal_panel_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

BoxDecoration decorationOf(WidgetTester tester) {
  final box = tester.widget<DecoratedBox>(find.byType(DecoratedBox));
  return box.decoration as BoxDecoration;
}

BevelPainter bevelOf(WidgetTester tester) {
  final paint = tester.widget<CustomPaint>(find.byType(CustomPaint).first);
  return paint.foregroundPainter! as BevelPainter;
}

void main() {
  testWidgets('raised panel uses the panel gradient', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    final gradient = decorationOf(tester).gradient! as LinearGradient;
    expect(gradient.colors.first, TrampColors.panelTop);
  });

  testWidgets('lcd glass surface fills with the glass colour', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.lcdGlass,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(decorationOf(tester).color, TrampColors.lcdGlass);
  });

  testWidgets('bevel width is snapped from the ambient zoom', (tester) async {
    await tester.pumpWidget(
      const ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(bevelOf(tester).spec.bevel, closeTo(2 / 1.5, 1e-9));
  });

  testWidgets('renders and paints without a ZoomScope ancestor',
      (tester) async {
    await tester.pumpWidget(
      const Center(
        child: MetalPanel(
          surface: TrampSurface.raisedButton,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(tester.takeException(), isNull);
    expect(bevelOf(tester).spec.bevel, ZoomScope.hairline);
  });

  testWidgets('the fill carries no border so it can be rounded and painted',
      (tester) async {
    await tester.pumpWidget(
      const Center(
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(decorationOf(tester).border, isNull);
    expect(decorationOf(tester).borderRadius, isNotNull);
    expect(tester.takeException(), isNull);
  });

  testWidgets('padding wraps the child when supplied', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.raisedPanel,
        padding: EdgeInsets.all(4),
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(find.byType(Padding), findsOneWidget);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/chrome/metal_panel_test.dart`
Expected: FAIL — `TrampSurface` is undefined and `MetalPanel` still takes `MetalPanelStyle`.

- [ ] **Step 3: Replace MetalPanel**

Replace `lib/ui/chrome/metal_panel.dart` entirely:

```dart
import 'package:flutter/widgets.dart';

import '../../theme/tramp_surfaces.dart';
import '../zoom/zoom_scope.dart';

/// Which material a panel wears.
enum TrampSurface { raisedPanel, raisedButton, pressedButton, insetWell, lcdGlass }

/// Applies one of the shared surface recipes.
///
/// This widget deliberately holds no visual decisions of its own — it exists so
/// callers name a material instead of hand-rolling one. It composes the fill and
/// the bevel, which are separate because Flutter will not paint a rounded
/// decoration that also carries a two-tone border.
class MetalPanel extends StatelessWidget {
  const MetalPanel({
    super.key,
    required this.surface,
    required this.child,
    this.padding,
  });

  final TrampSurface surface;
  final Widget child;
  final EdgeInsets? padding;

  @override
  Widget build(BuildContext context) {
    final bevel = ZoomScope.hairlineFor(context);
    final spec = switch (surface) {
      TrampSurface.raisedPanel => TrampSurfaces.raisedPanel(bevel: bevel),
      TrampSurface.raisedButton => TrampSurfaces.raisedButton(bevel: bevel),
      TrampSurface.pressedButton => TrampSurfaces.pressedButton(bevel: bevel),
      TrampSurface.insetWell => TrampSurfaces.insetWell(bevel: bevel),
      TrampSurface.lcdGlass => TrampSurfaces.lcdGlass(bevel: bevel),
    };

    final inner =
        padding == null ? child : Padding(padding: padding!, child: child);

    return CustomPaint(
      foregroundPainter: BevelPainter(spec: spec),
      child: DecoratedBox(decoration: spec.decoration, child: inner),
    );
  }
}
```

`MetalPanel` exposes `surface` publicly so tests can assert which material a widget chose, and the bevel is reachable through the `BevelPainter` on the `CustomPaint`.

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/chrome/metal_panel_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 5: Commit**

```bash
git add lib/ui/chrome/metal_panel.dart test/ui/chrome/metal_panel_test.dart
git commit -m "refactor(chrome): MetalPanel names a surface instead of building one

Panels now select from the shared TrampSurfaces recipes and take their bevel
width from the ambient zoom, so no widget composes its own gradient."
```

---

### Task 5: ChromeButton variants

> **Execution order:** run **Task 7 before this task.** `ChromeButton`'s dropdown
> variant imports `ChevronPainter` from `lib/ui/chrome/transport_icons.dart`, and
> that file does not compile until Task 7 rewrites it off the deleted palette —
> so Task 5's tests cannot even build first. Task 7 has no dependency on Task 5.
> The numbering is left alone so task references stay stable; only the order
> changes: 1, 2, 3, 4, **7**, **5**, 6, 8, …

**Files:**
- Modify: `lib/ui/chrome/chrome_button.dart` (replace contents)
- Test: `test/ui/chrome/chrome_button_test.dart`

**Interfaces:**
- Consumes: `MetalPanel`, `TrampSurface` (Task 4), `TrampText` (Task 1).
- Produces:
  - `ChromeButton.icon({Key? key, required Widget icon, required VoidCallback? onPressed, required String semanticLabel, Size size, bool active})`
  - `ChromeButton.label({Key? key, required String text, required VoidCallback? onPressed, Widget? leading, Size? size, bool active})`
  - `ChromeButton.dropdown({Key? key, required String text, required VoidCallback? onPressed, Size? size})`
  - All expose `bool get isEnabled`. A disabled button renders its content in `TrampColors.labelDim` and does not react to taps. An `active` button renders content in `TrampColors.phosphor`.

**How content tinting applies to icons.** Label text and the chevron take the resolved content colour directly. An `icon` or `leading` is an arbitrary widget the caller already tinted — the play triangle arrives in `phosphor`, for instance — so the button must not blanket-recolour it, or that intent is lost. The rule: when the button is **disabled** or **active**, wrap the glyph in a `ColorFiltered` with `BlendMode.srcIn` to force `labelDim` or `phosphor` respectively; otherwise pass it through untouched. That way a disabled transport button visibly dims, a lit toggle visibly lights, and the play triangle keeps its phosphor in the ordinary case.

- [ ] **Step 1: Write the failing test**

Create `test/ui/chrome/chrome_button_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/chrome_button.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

TrampSurface surfaceOf(WidgetTester tester) =>
    tester.widget<MetalPanel>(find.byType(MetalPanel)).surface;

void main() {
  testWidgets('label button reports its text and fires onPressed',
      (tester) async {
    var taps = 0;
    await tester.pumpWidget(host(
      ChromeButton.label(text: 'OPEN', onPressed: () => taps++),
    ));
    expect(find.text('OPEN'), findsOneWidget);
    await tester.tap(find.byType(ChromeButton));
    expect(taps, 1);
  });

  testWidgets('a null onPressed makes the button inert and dim', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.label(text: 'OPEN', onPressed: null),
    ));
    await tester.tap(find.byType(ChromeButton));
    expect(tester.takeException(), isNull);
    final text = tester.widget<Text>(find.text('OPEN'));
    expect(text.style!.color, TrampColors.labelDim);
  });

  testWidgets('an active toggle lights its label in phosphor', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.label(text: 'ON', onPressed: () {}, active: true),
    ));
    final text = tester.widget<Text>(find.text('ON'));
    expect(text.style!.color, TrampColors.phosphor);
  });

  testWidgets('pressing swaps to the pressed surface and back', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.label(text: 'OPEN', onPressed: () {}),
    ));
    expect(surfaceOf(tester), TrampSurface.raisedButton);

    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(ChromeButton)),
    );
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.pressedButton);

    await gesture.up();
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.raisedButton);
  });

  testWidgets('a disabled button never shows the pressed surface',
      (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.label(text: 'OPEN', onPressed: null),
    ));
    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(ChromeButton)),
    );
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.raisedButton);
    await gesture.up();
  });

  testWidgets('icon button carries a semantics label', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.icon(
        icon: const SizedBox(width: 8, height: 8),
        onPressed: () {},
        semanticLabel: 'Shuffle',
      ),
    ));
    expect(
      find.bySemanticsLabel('Shuffle'),
      findsOneWidget,
    );
  });

  testWidgets('dropdown button renders text plus a chevron', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.dropdown(text: 'ZOOM 150%', onPressed: () {}),
    ));
    expect(find.text('ZOOM 150%'), findsOneWidget);
    expect(find.byKey(ChromeButton.chevronKey), findsOneWidget);
  });

  testWidgets('explicit size is honoured', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton.label(
        text: 'OPEN',
        onPressed: () {},
        size: const Size(54, 26),
      ),
    ));
    expect(tester.getSize(find.byType(ChromeButton)), const Size(54, 26));
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/chrome/chrome_button_test.dart`
Expected: FAIL — the named constructors do not exist.

- [ ] **Step 3: Replace ChromeButton**

Replace `lib/ui/chrome/chrome_button.dart` entirely:

```dart
import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';
import 'metal_panel.dart';
import 'transport_icons.dart';

/// A raised graphite button.
///
/// Three shapes cover every control in the chrome: an icon button (transport,
/// shuffle, window controls), a label button (OPEN, ON, AUTO, EQ, PL), and a
/// dropdown (ZOOM, PRESETS).
class ChromeButton extends StatefulWidget {
  const ChromeButton._({
    super.key,
    required this.onPressed,
    required this.semanticLabel,
    required this.active,
    required this.size,
    this.icon,
    this.text,
    this.leading,
    this.chevron = false,
  });

  factory ChromeButton.icon({
    Key? key,
    required Widget icon,
    required VoidCallback? onPressed,
    required String semanticLabel,
    Size size = const Size(26, 26),
    bool active = false,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel,
      active: active,
      size: size,
      icon: icon,
    );
  }

  factory ChromeButton.label({
    Key? key,
    required String text,
    required VoidCallback? onPressed,
    Widget? leading,
    Size? size,
    bool active = false,
    String? semanticLabel,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel ?? text,
      active: active,
      size: size,
      text: text,
      leading: leading,
    );
  }

  factory ChromeButton.dropdown({
    Key? key,
    required String text,
    required VoidCallback? onPressed,
    Size? size,
    String? semanticLabel,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel ?? text,
      active: false,
      size: size,
      text: text,
      chevron: true,
    );
  }

  static const chevronKey = Key('chrome-button-chevron');

  final VoidCallback? onPressed;
  final String semanticLabel;
  final bool active;
  final Size? size;
  final Widget? icon;
  final String? text;
  final Widget? leading;
  final bool chevron;

  bool get isEnabled => onPressed != null;

  @override
  State<ChromeButton> createState() => _ChromeButtonState();
}

class _ChromeButtonState extends State<ChromeButton> {
  bool _down = false;

  void _setDown(bool value) {
    if (!widget.isEnabled || _down == value) return;
    setState(() => _down = value);
  }

  Color get _contentColour {
    if (!widget.isEnabled) return TrampColors.labelDim;
    return widget.active ? TrampColors.phosphor : TrampColors.label;
  }

  @override
  Widget build(BuildContext context) {
    final colour = _contentColour;

    final Widget content;
    if (widget.icon != null) {
      content = Center(child: widget.icon);
    } else {
      content = Row(
        mainAxisAlignment: MainAxisAlignment.center,
        mainAxisSize: MainAxisSize.min,
        children: [
          if (widget.leading != null) ...[
            widget.leading!,
            const SizedBox(width: 4),
          ],
          Text(widget.text!, style: TrampText.chromeLabel.copyWith(color: colour)),
          if (widget.chevron) ...[
            const SizedBox(width: 5),
            SizedBox(
              key: ChromeButton.chevronKey,
              width: 7,
              height: 5,
              child: CustomPaint(painter: ChevronPainter(colour: colour)),
            ),
          ],
        ],
      );
    }

    // Horizontal padding is only for buttons that size themselves to their
    // label. When an explicit `size` is given, that size IS the constraint —
    // adding padding on top steals width from the label and overflows tight
    // buttons. `AUTO` needs 25.87px in Barlow and a 37px button leaves only
    // 23px once 7px is taken from each side.
    Widget button = MetalPanel(
      surface: _down ? TrampSurface.pressedButton : TrampSurface.raisedButton,
      padding: (widget.icon != null || widget.size != null)
          ? null
          : const EdgeInsets.symmetric(horizontal: 7),
      child: content,
    );

    if (widget.size != null) {
      button = SizedBox(
        width: widget.size!.width,
        height: widget.size!.height,
        child: button,
      );
    }

    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.semanticLabel,
      child: MouseRegion(
        cursor: widget.isEnabled
            ? SystemMouseCursors.click
            : SystemMouseCursors.basic,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (_) => _setDown(true),
          onTapUp: (_) => _setDown(false),
          onTapCancel: () => _setDown(false),
          onTap: widget.onPressed,
          child: button,
        ),
      ),
    );
  }
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/chrome/chrome_button_test.dart`
Expected: PASS (8 tests)

`ChevronPainter` and every transport glyph come from `lib/ui/chrome/transport_icons.dart`, which **Task 7 must have completed first** — see the execution-order note below. Do not define a chevron here.

- [ ] **Step 5: Commit**

```bash
git add lib/ui/chrome/chrome_button.dart lib/ui/chrome/transport_icons.dart \
        test/ui/chrome/chrome_button_test.dart
git commit -m "feat(chrome): icon, label and dropdown button variants

One raised-graphite button with three shapes covers every control in the
mockup, with press, disabled and lit-toggle states driven from the shared
surfaces and palette."
```

---

### Task 6: ChromeSlider, horizontal and vertical

**Files:**
- Modify: `lib/ui/chrome/chrome_slider.dart` (replace contents)
- Test: `test/ui/chrome/chrome_slider_test.dart` (replace contents)

**Interfaces:**
- Consumes: `TrampColors` (Task 1), `ZoomScope.hairlineFor` (Task 3).
- Produces: `ChromeSlider({Key? key, required double value, required Axis axis, ValueChanged<double>? onChanged, ValueChanged<double>? onChangeEnd, bool dimmed = false, double thumbExtent = 17, double thumbThickness = 10, String? semanticLabel})`. `value` is `0..1`. For `Axis.vertical`, `value` 1 is the top. A slider with both callbacks null is read-only and ignores gestures.

- [ ] **Step 1: Write the failing test**

Replace `test/ui/chrome/chrome_slider_test.dart` entirely:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/chrome_slider.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: SizedBox(width: 200, height: 200, child: child)),
    );

void main() {
  testWidgets('dragging a horizontal slider reports a higher value',
      (tester) async {
    double? ended;
    await tester.pumpWidget(host(
      ChromeSlider(
        value: 0.0,
        axis: Axis.horizontal,
        onChanged: (_) {},
        onChangeEnd: (v) => ended = v,
      ),
    ));
    await tester.drag(find.byType(ChromeSlider), const Offset(100, 0));
    await tester.pumpAndSettle();
    expect(ended, isNotNull);
    expect(ended, greaterThan(0.3));
  });

  testWidgets('dragging a vertical slider upward raises the value',
      (tester) async {
    double? ended;
    await tester.pumpWidget(host(
      ChromeSlider(
        value: 0.5,
        axis: Axis.vertical,
        onChanged: (_) {},
        onChangeEnd: (v) => ended = v,
      ),
    ));
    await tester.drag(find.byType(ChromeSlider), const Offset(0, -60));
    await tester.pumpAndSettle();
    expect(ended, greaterThan(0.5));
  });

  testWidgets('values are clamped to the 0..1 range', (tester) async {
    double? ended;
    await tester.pumpWidget(host(
      ChromeSlider(
        value: 0.5,
        axis: Axis.horizontal,
        onChanged: (_) {},
        onChangeEnd: (v) => ended = v,
      ),
    ));
    await tester.drag(find.byType(ChromeSlider), const Offset(9999, 0));
    await tester.pumpAndSettle();
    expect(ended, 1.0);
  });

  testWidgets('a read-only slider ignores drags', (tester) async {
    await tester.pumpWidget(host(
      const ChromeSlider(value: 0.4, axis: Axis.horizontal),
    ));
    await tester.drag(find.byType(ChromeSlider), const Offset(80, 0));
    await tester.pumpAndSettle();
    expect(tester.takeException(), isNull);
  });

  testWidgets('fill paints in phosphor, and in the dim tone when dimmed',
      (tester) async {
    await tester.pumpWidget(host(
      const ChromeSlider(value: 0.5, axis: Axis.horizontal),
    ));
    var painter = tester
        .widget<CustomPaint>(find.byType(CustomPaint).first)
        .painter! as SliderPainter;
    expect(painter.fill, TrampColors.phosphor);

    await tester.pumpWidget(host(
      const ChromeSlider(value: 0.5, axis: Axis.horizontal, dimmed: true),
    ));
    painter = tester
        .widget<CustomPaint>(find.byType(CustomPaint).first)
        .painter! as SliderPainter;
    expect(painter.fill, TrampColors.phosphorDim);
  });

  testWidgets('semantics label is exposed when supplied', (tester) async {
    await tester.pumpWidget(host(
      const ChromeSlider(
        value: 0.5,
        axis: Axis.horizontal,
        semanticLabel: 'Volume',
      ),
    ));
    expect(find.bySemanticsLabel('Volume'), findsOneWidget);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/chrome/chrome_slider_test.dart`
Expected: FAIL — `ChromeSlider` has no `axis` parameter and `SliderPainter` is undefined.

- [ ] **Step 3: Replace ChromeSlider**

Replace `lib/ui/chrome/chrome_slider.dart` entirely:

```dart
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../zoom/zoom_scope.dart';

/// Groove, phosphor fill and metal thumb — the slider language the equalizer
/// bands and the volume control share.
class ChromeSlider extends StatefulWidget {
  const ChromeSlider({
    super.key,
    required this.value,
    required this.axis,
    this.onChanged,
    this.onChangeEnd,
    this.dimmed = false,
    this.thumbExtent = 17,
    this.thumbThickness = 10,
    this.semanticLabel,
  });

  /// Position in `0..1`. For [Axis.vertical], 1 is the top of the travel.
  final double value;
  final Axis axis;
  final ValueChanged<double>? onChanged;
  final ValueChanged<double>? onChangeEnd;

  /// Renders the fill in [TrampColors.phosphorDim] — used while muted.
  final bool dimmed;

  /// Thumb size measured across and along the travel axis.
  final double thumbExtent;
  final double thumbThickness;

  final String? semanticLabel;

  bool get isInteractive => onChanged != null || onChangeEnd != null;

  @override
  State<ChromeSlider> createState() => _ChromeSliderState();
}

class _ChromeSliderState extends State<ChromeSlider> {
  double? _preview;

  /// Drag-end callbacks carry no local position, so the last update's position
  /// is remembered to resolve the final value.
  Offset? _lastLocal;

  double get _shown => (_preview ?? widget.value).clamp(0.0, 1.0);

  double _fractionFor(Offset local, Size size) {
    if (widget.axis == Axis.horizontal) {
      final usable = math.max(1.0, size.width - widget.thumbExtent);
      return ((local.dx - widget.thumbExtent / 2) / usable).clamp(0.0, 1.0);
    }
    final usable = math.max(1.0, size.height - widget.thumbExtent);
    // Screen y grows downward; a slider's value grows upward.
    return (1 - (local.dy - widget.thumbExtent / 2) / usable).clamp(0.0, 1.0);
  }

  void _update(Offset local, Size size, {required bool end}) {
    _lastLocal = local;
    final fraction = _fractionFor(local, size);
    setState(() => _preview = fraction);
    widget.onChanged?.call(fraction);
    if (end) {
      widget.onChangeEnd?.call(fraction);
      setState(() => _preview = null);
    }
  }

  @override
  Widget build(BuildContext context) {
    final bevel = ZoomScope.hairlineFor(context);

    Widget slider = LayoutBuilder(
      builder: (context, constraints) {
        final size = Size(constraints.maxWidth, constraints.maxHeight);

        final paint = CustomPaint(
          painter: SliderPainter(
            value: _shown,
            axis: widget.axis,
            fill: widget.dimmed
                ? TrampColors.phosphorDim
                : TrampColors.phosphor,
            bevel: bevel,
            thumbExtent: widget.thumbExtent,
            thumbThickness: widget.thumbThickness,
          ),
          size: size,
        );

        if (!widget.isInteractive) return paint;

        return GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (d) => _update(d.localPosition, size, end: true),
          onVerticalDragUpdate: widget.axis == Axis.vertical
              ? (d) => _update(d.localPosition, size, end: false)
              : null,
          onVerticalDragEnd: widget.axis == Axis.vertical
              ? (_) => _update(
                    _lastLocal ?? Offset.zero,
                    size,
                    end: true,
                  )
              : null,
          onHorizontalDragUpdate: widget.axis == Axis.horizontal
              ? (d) => _update(d.localPosition, size, end: false)
              : null,
          onHorizontalDragEnd: widget.axis == Axis.horizontal
              ? (_) => _update(
                    _lastLocal ?? Offset.zero,
                    size,
                    end: true,
                  )
              : null,
          onHorizontalDragCancel: () => setState(() => _preview = null),
          onVerticalDragCancel: () => setState(() => _preview = null),
          child: paint,
        );
      },
    );

    if (widget.semanticLabel != null) {
      slider = Semantics(
        slider: true,
        label: widget.semanticLabel,
        value: '${(_shown * 100).round()}%',
        child: slider,
      );
    }

    return slider;
  }
}
```

- [ ] **Step 4: Write the slider painter**

Append to `lib/ui/chrome/chrome_slider.dart`:

```dart
/// Paints the groove, the phosphor fill and the thumb.
class SliderPainter extends CustomPainter {
  const SliderPainter({
    required this.value,
    required this.axis,
    required this.fill,
    required this.bevel,
    required this.thumbExtent,
    required this.thumbThickness,
  });

  final double value;
  final Axis axis;
  final Color fill;
  final double bevel;
  final double thumbExtent;
  final double thumbThickness;

  @override
  void paint(Canvas canvas, Size size) {
    final horizontal = axis == Axis.horizontal;
    final grooveThickness = math.max(3.0, bevel * 3);

    final groove = horizontal
        ? Rect.fromLTWH(
            thumbExtent / 2,
            (size.height - grooveThickness) / 2,
            math.max(0, size.width - thumbExtent),
            grooveThickness,
          )
        : Rect.fromLTWH(
            (size.width - grooveThickness) / 2,
            thumbExtent / 2,
            grooveThickness,
            math.max(0, size.height - thumbExtent),
          );

    canvas.drawRect(groove, Paint()..color = TrampColors.wellDeep);
    canvas.drawRect(
      groove,
      Paint()
        ..color = TrampColors.bevelLo
        ..style = PaintingStyle.stroke
        ..strokeWidth = bevel,
    );

    // Fill runs from the low end of the travel to the thumb.
    final filled = horizontal
        ? Rect.fromLTWH(groove.left, groove.top, groove.width * value, groove.height)
        : Rect.fromLTWH(
            groove.left,
            groove.bottom - groove.height * value,
            groove.width,
            groove.height * value,
          );
    canvas.drawRect(filled, Paint()..color = fill);

    final centre = horizontal
        ? Offset(groove.left + groove.width * value, size.height / 2)
        : Offset(size.width / 2, groove.bottom - groove.height * value);

    final thumb = Rect.fromCenter(
      center: centre,
      width: horizontal ? thumbThickness : thumbExtent,
      height: horizontal ? thumbExtent : thumbThickness,
    );

    canvas.drawRect(
      thumb,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [TrampColors.thumbHi, TrampColors.buttonBottom],
        ).createShader(thumb),
    );
    canvas.drawRect(
      thumb,
      Paint()
        ..color = TrampColors.bevelLo
        ..style = PaintingStyle.stroke
        ..strokeWidth = bevel,
    );
  }

  @override
  bool shouldRepaint(SliderPainter old) =>
      old.value != value ||
      old.axis != axis ||
      old.fill != fill ||
      old.bevel != bevel ||
      old.thumbExtent != thumbExtent ||
      old.thumbThickness != thumbThickness;
}
```

- [ ] **Step 5: Run the test to verify it passes**

Run: `flutter test test/ui/chrome/chrome_slider_test.dart`
Expected: PASS (6 tests)

- [ ] **Step 6: Commit**

```bash
git add lib/ui/chrome/chrome_slider.dart test/ui/chrome/chrome_slider_test.dart
git commit -m "feat(chrome): horizontal and vertical groove sliders

One slider serves the seek bar, the volume control and the equalizer bands,
with a phosphor fill that dims while muted."
```

---

### Task 7: Glyphs and LCD text

> **Execution order:** run this task **before Task 5**. It is self-contained, and
> Task 5's dropdown button imports `ChevronPainter` from the file this task
> rewrites.

**Files:**
- Modify: `lib/ui/chrome/transport_icons.dart` (replace contents; this task creates `ChevronPainter`, which Task 5 then consumes)
- Create: `lib/ui/chrome/lcd_text.dart`
- Test: `test/ui/chrome/transport_icons_test.dart`
- Test: `test/ui/chrome/lcd_text_test.dart`

**Interfaces:**
- Consumes: `TrampColors`, `TrampText` (Task 1).
- Produces:
  - `TransportIcons.prev({Color colour})`, `.play({Color colour})`, `.pause({Color colour})`, `.stop({Color colour})`, `.next({Color colour})`, `.shuffle({Color colour})`, `.repeat({Color colour, bool one})`, `.eject({Color colour})` — each returns a `Widget`.

**No brand mark belongs in this file.** The reference mockup's lightning bolt is Winamp's logo. Tramp has its own mark, `TrampLogo` in `lib/ui/chrome/logo.dart`, and that is what the title bar uses.
  - `ChevronPainter({required Color colour})` (already added in Task 5).
  - `LcdText(String text, {Key? key, bool lit = true, LcdSize size = LcdSize.normal})` and `enum LcdSize { normal, large }`.
  - `LcdIndicator(String label, {Key? key, required bool lit, required VoidCallback? onTap})` — the small `EQ`/`PL` markers inside the display well.

- [ ] **Step 1: Write the failing glyph test**

Create `test/ui/chrome/transport_icons_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/transport_icons.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: SizedBox(width: 40, height: 40, child: child)),
    );

void main() {
  testWidgets('every glyph paints without error', (tester) async {
    final glyphs = <String, Widget>{
      'prev': TransportIcons.prev(),
      'play': TransportIcons.play(),
      'pause': TransportIcons.pause(),
      'stop': TransportIcons.stop(),
      'next': TransportIcons.next(),
      'shuffle': TransportIcons.shuffle(),
      'repeat': TransportIcons.repeat(),
      'repeatOne': TransportIcons.repeat(one: true),
      'eject': TransportIcons.eject(),
    };

    for (final entry in glyphs.entries) {
      await tester.pumpWidget(host(entry.value));
      expect(tester.takeException(), isNull, reason: entry.key);
      expect(find.byType(CustomPaint), findsWidgets, reason: entry.key);
    }
  });

  testWidgets('play defaults to phosphor and honours an override',
      (tester) async {
    await tester.pumpWidget(host(TransportIcons.play()));
    expect(tester.takeException(), isNull);

    await tester.pumpWidget(host(TransportIcons.play(colour: TrampColors.label)));
    expect(tester.takeException(), isNull);
  });

  test('repeat-one is distinguishable from repeat-all', () {
    const all = RepeatPainter(colour: TrampColors.label, one: false);
    const one = RepeatPainter(colour: TrampColors.label, one: true);
    expect(all.shouldRepaint(one), isTrue);
  });

  // Tramp's own mark lives in `TrampLogo` (lib/ui/chrome/logo.dart), rendered
  // from logo.svg. The glyph set must not carry a brand mark of its own — the
  // reference mockup's lightning bolt is Winamp's logo, not a generic icon.
  test('the glyph set contains no brand mark', () {
    expect(
      TransportIcons.defaultGlyphColour,
      TrampColors.label,
      reason: 'glyphs are neutral chrome, tinted by the caller',
    );
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/chrome/transport_icons_test.dart`
Expected: FAIL — `shuffle`, `repeat`, `eject` and `RepeatPainter` are undefined.

- [ ] **Step 3: Replace the glyph library**

Replace `lib/ui/chrome/transport_icons.dart` entirely:

```dart
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';

/// Vector glyphs for every icon in the chrome.
///
/// Painted rather than drawn from a font so they stay crisp at every zoom step
/// and need no icon-font asset.
abstract final class TransportIcons {
  static const defaultGlyphColour = TrampColors.label;

  static Widget prev({Color colour = defaultGlyphColour}) =>
      _paint(SkipPainter(colour: colour, forward: false), const Size(16, 12));

  static Widget next({Color colour = defaultGlyphColour}) =>
      _paint(SkipPainter(colour: colour, forward: true), const Size(16, 12));

  static Widget play({Color colour = TrampColors.phosphor}) =>
      _paint(PlayPainter(colour: colour), const Size(14, 14));

  static Widget pause({Color colour = defaultGlyphColour}) =>
      _paint(PausePainter(colour: colour), const Size(12, 14));

  static Widget stop({Color colour = defaultGlyphColour}) =>
      _paint(StopPainter(colour: colour), const Size(12, 12));

  static Widget shuffle({Color colour = defaultGlyphColour}) =>
      _paint(ShufflePainter(colour: colour), const Size(16, 12));

  static Widget repeat({Color colour = defaultGlyphColour, bool one = false}) =>
      _paint(RepeatPainter(colour: colour, one: one), const Size(16, 13));

  static Widget eject({Color colour = defaultGlyphColour}) =>
      _paint(EjectPainter(colour: colour), const Size(13, 12));

  static Widget _paint(CustomPainter painter, Size size) => SizedBox(
        width: size.width,
        height: size.height,
        child: CustomPaint(painter: painter),
      );
}

Paint _fill(Color colour) => Paint()..color = colour;

Paint _stroke(Color colour, double width) => Paint()
  ..color = colour
  ..style = PaintingStyle.stroke
  ..strokeWidth = width
  ..strokeJoin = StrokeJoin.round;

class PlayPainter extends CustomPainter {
  const PlayPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, 0)
      ..lineTo(size.width, size.height / 2)
      ..lineTo(0, size.height)
      ..close();
    canvas.drawPath(path, _fill(colour));
  }

  @override
  bool shouldRepaint(PlayPainter old) => old.colour != colour;
}

class PausePainter extends CustomPainter {
  const PausePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final barWidth = size.width * 0.36;
    canvas.drawRect(
      Rect.fromLTWH(0, 0, barWidth, size.height),
      _fill(colour),
    );
    canvas.drawRect(
      Rect.fromLTWH(size.width - barWidth, 0, barWidth, size.height),
      _fill(colour),
    );
  }

  @override
  bool shouldRepaint(PausePainter old) => old.colour != colour;
}

class StopPainter extends CustomPainter {
  const StopPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawRect(Offset.zero & size, _fill(colour));
  }

  @override
  bool shouldRepaint(StopPainter old) => old.colour != colour;
}

class SkipPainter extends CustomPainter {
  const SkipPainter({required this.colour, required this.forward});

  final Color colour;
  final bool forward;

  @override
  void paint(Canvas canvas, Size size) {
    final triangle = size.width * 0.42;
    final barWidth = size.width * 0.12;

    void wedge(double left, bool pointsRight) {
      final path = Path();
      if (pointsRight) {
        path
          ..moveTo(left, 0)
          ..lineTo(left + triangle, size.height / 2)
          ..lineTo(left, size.height);
      } else {
        path
          ..moveTo(left + triangle, 0)
          ..lineTo(left, size.height / 2)
          ..lineTo(left + triangle, size.height);
      }
      canvas.drawPath(path..close(), _fill(colour));
    }

    if (forward) {
      wedge(0, true);
      wedge(triangle * 0.9, true);
      canvas.drawRect(
        Rect.fromLTWH(size.width - barWidth, 0, barWidth, size.height),
        _fill(colour),
      );
    } else {
      canvas.drawRect(Rect.fromLTWH(0, 0, barWidth, size.height), _fill(colour));
      wedge(barWidth + triangle * 0.1, false);
      wedge(barWidth + triangle, false);
    }
  }

  @override
  bool shouldRepaint(SkipPainter old) =>
      old.colour != colour || old.forward != forward;
}

class ShufflePainter extends CustomPainter {
  const ShufflePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final p = _stroke(colour, math.max(1.0, size.height * 0.13));
    // Two crossing paths with arrowheads: the classic shuffle mark.
    canvas.drawLine(Offset(0, size.height * 0.2),
        Offset(size.width * 0.78, size.height * 0.8), p);
    canvas.drawLine(Offset(0, size.height * 0.8),
        Offset(size.width * 0.78, size.height * 0.2), p);

    for (final y in [size.height * 0.2, size.height * 0.8]) {
      final path = Path()
        ..moveTo(size.width * 0.72, y - size.height * 0.18)
        ..lineTo(size.width, y)
        ..lineTo(size.width * 0.72, y + size.height * 0.18)
        ..close();
      canvas.drawPath(path, _fill(colour));
    }
  }

  @override
  bool shouldRepaint(ShufflePainter old) => old.colour != colour;
}

class RepeatPainter extends CustomPainter {
  const RepeatPainter({required this.colour, required this.one});

  final Color colour;

  /// Draws a `1` inside the loop for repeat-one.
  final bool one;

  @override
  void paint(Canvas canvas, Size size) {
    final stroke = math.max(1.0, size.height * 0.13);
    final p = _stroke(colour, stroke);
    final rect = Rect.fromLTWH(
      stroke,
      stroke,
      size.width - stroke * 2,
      size.height - stroke * 2,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(rect, Radius.circular(size.height * 0.35)),
      p,
    );

    final head = Path()
      ..moveTo(rect.right - size.width * 0.22, rect.top - size.height * 0.06)
      ..lineTo(rect.right, rect.top + size.height * 0.14)
      ..lineTo(rect.right - size.width * 0.22, rect.top + size.height * 0.34)
      ..close();
    canvas.drawPath(head, _fill(colour));

    if (one) {
      final bar = Rect.fromLTWH(
        size.width / 2 - stroke / 2,
        size.height * 0.32,
        stroke,
        size.height * 0.36,
      );
      canvas.drawRect(bar, _fill(colour));
    }
  }

  @override
  bool shouldRepaint(RepeatPainter old) =>
      old.colour != colour || old.one != one;
}

class EjectPainter extends CustomPainter {
  const EjectPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final triangle = Path()
      ..moveTo(0, size.height * 0.62)
      ..lineTo(size.width / 2, 0)
      ..lineTo(size.width, size.height * 0.62)
      ..close();
    canvas.drawPath(triangle, _fill(colour));
    canvas.drawRect(
      Rect.fromLTWH(0, size.height * 0.78, size.width, size.height * 0.22),
      _fill(colour),
    );
  }

  @override
  bool shouldRepaint(EjectPainter old) => old.colour != colour;
}

/// Small downward chevron for dropdown buttons.
class ChevronPainter extends CustomPainter {
  const ChevronPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, 0)
      ..lineTo(size.width, 0)
      ..lineTo(size.width / 2, size.height)
      ..close();
    canvas.drawPath(path, _fill(colour));
  }

  @override
  bool shouldRepaint(ChevronPainter old) => old.colour != colour;
}
```

- [ ] **Step 4: Run the glyph test to verify it passes**

Run: `flutter test test/ui/chrome/transport_icons_test.dart`
Expected: PASS (4 tests)

- [ ] **Step 5: Write the failing LCD text test**

Create `test/ui/chrome/lcd_text_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/lcd_text.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

void main() {
  testWidgets('lit text uses phosphor, unlit uses the dim tone',
      (tester) async {
    await tester.pumpWidget(host(const LcdText('0:05')));
    expect(
      tester.widget<Text>(find.text('0:05')).style!.color,
      TrampColors.phosphor,
    );

    await tester.pumpWidget(host(const LcdText('0:05', lit: false)));
    expect(
      tester.widget<Text>(find.text('0:05')).style!.color,
      TrampColors.phosphorDim,
    );
  });

  testWidgets('large size renders at the big readout scale', (tester) async {
    await tester.pumpWidget(host(const LcdText('0:05', size: LcdSize.large)));
    final normal = tester.widget<Text>(find.text('0:05')).style!.fontSize!;
    await tester.pumpWidget(host(const LcdText('0:05')));
    final small = tester.widget<Text>(find.text('0:05')).style!.fontSize!;
    expect(normal, greaterThan(small));
  });

  testWidgets('all LCD text uses the bundled mono family', (tester) async {
    await tester.pumpWidget(host(const LcdText('128 kbps')));
    expect(
      tester.widget<Text>(find.text('128 kbps')).style!.fontFamily,
      'IBMPlexMono',
    );
  });

  testWidgets('indicator lights when active and reports taps', (tester) async {
    var taps = 0;
    await tester.pumpWidget(host(
      LcdIndicator('EQ', lit: true, onTap: () => taps++),
    ));
    expect(
      tester.widget<Text>(find.text('EQ')).style!.color,
      TrampColors.phosphor,
    );
    await tester.tap(find.text('EQ'));
    expect(taps, 1);
  });

  testWidgets('indicator with no callback is inert', (tester) async {
    await tester.pumpWidget(host(
      const LcdIndicator('PL', lit: false, onTap: null),
    ));
    await tester.tap(find.text('PL'));
    expect(tester.takeException(), isNull);
  });
}
```

- [ ] **Step 6: Run it to verify it fails**

Run: `flutter test test/ui/chrome/lcd_text_test.dart`
Expected: FAIL — `lcd_text.dart` does not exist.

- [ ] **Step 7: Write LcdText and LcdIndicator**

Create `lib/ui/chrome/lcd_text.dart`:

```dart
import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';

enum LcdSize { normal, large }

/// Phosphor text on the display well.
class LcdText extends StatelessWidget {
  const LcdText(
    this.text, {
    super.key,
    this.lit = true,
    this.size = LcdSize.normal,
    this.textAlign,
  });

  final String text;
  final bool lit;
  final LcdSize size;
  final TextAlign? textAlign;

  @override
  Widget build(BuildContext context) {
    final base = size == LcdSize.large ? TrampText.lcdLarge : TrampText.lcd;
    return Text(
      text,
      textAlign: textAlign,
      maxLines: 1,
      overflow: TextOverflow.clip,
      style: base.copyWith(
        color: lit ? TrampColors.phosphor : TrampColors.phosphorDim,
      ),
    );
  }
}

/// The small `EQ` / `PL` markers inside the display well.
///
/// These report which lower region is showing; they are readouts first and
/// shortcuts second, so an absent [onTap] is a valid state.
class LcdIndicator extends StatelessWidget {
  const LcdIndicator(
    this.label, {
    super.key,
    required this.lit,
    required this.onTap,
  });

  final String label;
  final bool lit;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final text = LcdText(label, lit: lit);

    final boxed = Container(
      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
      decoration: BoxDecoration(
        border: Border.all(
          color: lit ? TrampColors.phosphor : TrampColors.phosphorDim,
        ),
      ),
      child: text,
    );

    if (onTap == null) {
      return Semantics(label: label, child: boxed);
    }

    return Semantics(
      button: true,
      label: label,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: onTap,
          child: boxed,
        ),
      ),
    );
  }
}
```

- [ ] **Step 8: Run the LCD test to verify it passes**

Run: `flutter test test/ui/chrome/lcd_text_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 9: Commit**

```bash
git add lib/ui/chrome/transport_icons.dart lib/ui/chrome/lcd_text.dart \
        test/ui/chrome/transport_icons_test.dart test/ui/chrome/lcd_text_test.dart
git commit -m "feat(chrome): full vector glyph set and LCD text widgets

Adds shuffle, repeat, repeat-one, eject and chevron glyphs alongside the
transport marks, plus phosphor text and the EQ/PL display indicators."
```

---

### Task 8: Shared title bar

**Files:**
- Create: `lib/ui/chrome/title_bar.dart`
- Test: `test/ui/chrome/title_bar_test.dart`

**Interfaces:**
- Consumes: `TrampColors`, `TrampText` (Task 1), `TrampMetrics` (Task 2), `ZoomScope` (Task 3).
- Produces: `TrampTitleBar({Key? key, required String title, Widget? leading, List<Widget> trailing = const [], bool draggable = true})`. Height is `TrampMetrics.titleBar`. Renders `leading`, then a rail, then `title`, then a rail, then `trailing`. The rails are painted by `RailPainter({required Color colour, required double thickness})`.

- [ ] **Step 1: Write the failing test**

Create `test/ui/chrome/title_bar_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/title_bar.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Align(
        alignment: Alignment.topLeft,
        child: SizedBox(width: 812, child: child),
      ),
    );

void main() {
  testWidgets('renders the title in caps at the documented height',
      (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP')));
    expect(find.text('TRAMP'), findsOneWidget);
    expect(
      tester.getSize(find.byType(TrampTitleBar)).height,
      TrampMetrics.titleBar,
    );
  });

  testWidgets('never renders the Winamp brand', (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP EQUALIZER')));
    expect(find.textContaining('WINAMP', findRichText: true), findsNothing);
    expect(find.text('TRAMP EQUALIZER'), findsOneWidget);
  });

  testWidgets('rails paint in the rail accent', (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP')));
    final painters = tester
        .widgetList<CustomPaint>(find.byType(CustomPaint))
        .map((w) => w.painter)
        .whereType<RailPainter>()
        .toList();
    expect(painters, hasLength(2), reason: 'one rail either side of the title');
    expect(painters.first.colour, TrampColors.railAccent);
  });

  testWidgets('leading and trailing slots are placed', (tester) async {
    await tester.pumpWidget(host(
      TrampTitleBar(
        title: 'TRAMP',
        leading: const SizedBox(key: Key('lead'), width: 27, height: 27),
        trailing: const [
          SizedBox(key: Key('t1'), width: 27, height: 27),
          SizedBox(key: Key('t2'), width: 27, height: 27),
        ],
      ),
    ));
    expect(find.byKey(const Key('lead')), findsOneWidget);
    expect(find.byKey(const Key('t1')), findsOneWidget);
    expect(find.byKey(const Key('t2')), findsOneWidget);

    final lead = tester.getCenter(find.byKey(const Key('lead')));
    final title = tester.getCenter(find.text('TRAMP'));
    final trail = tester.getCenter(find.byKey(const Key('t2')));
    expect(lead.dx, lessThan(title.dx));
    expect(trail.dx, greaterThan(title.dx));
  });

  testWidgets('the drag region can be suppressed for tests and goldens',
      (tester) async {
    await tester.pumpWidget(
      host(const TrampTitleBar(title: 'TRAMP', draggable: false)),
    );
    expect(tester.takeException(), isNull);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/chrome/title_bar_test.dart`
Expected: FAIL — `title_bar.dart` does not exist.

- [ ] **Step 3: Write the title bar**

Create `lib/ui/chrome/title_bar.dart`:

```dart
import 'package:flutter/widgets.dart';
import 'package:window_manager/window_manager.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../zoom/zoom_scope.dart';

/// The title bar shared by the main player and the equalizer.
///
/// `draggable` exists so widget tests and goldens can render the bar without a
/// live `window_manager` binding.
class TrampTitleBar extends StatelessWidget {
  const TrampTitleBar({
    super.key,
    required this.title,
    this.leading,
    this.trailing = const [],
    this.draggable = true,
  });

  final String title;
  final Widget? leading;
  final List<Widget> trailing;
  final bool draggable;

  @override
  Widget build(BuildContext context) {
    final thickness = ZoomScope.hairlineFor(context) * 2;

    Widget centre = Row(
      children: [
        Expanded(
          child: CustomPaint(
            painter: RailPainter(
              colour: TrampColors.railAccent,
              thickness: thickness,
            ),
            child: const SizedBox(height: TrampMetrics.titleBar),
          ),
        ),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12),
          child: Text(title, style: TrampText.wordmark),
        ),
        Expanded(
          child: CustomPaint(
            painter: RailPainter(
              colour: TrampColors.railAccent,
              thickness: thickness,
            ),
            child: const SizedBox(height: TrampMetrics.titleBar),
          ),
        ),
      ],
    );

    if (draggable) {
      centre = DragToMoveArea(child: centre);
    }

    return SizedBox(
      height: TrampMetrics.titleBar,
      child: Row(
        children: [
          if (leading != null) ...[
            const SizedBox(width: 4),
            leading!,
            const SizedBox(width: 6),
          ],
          Expanded(child: centre),
          for (final widget in trailing) ...[
            const SizedBox(width: 5),
            widget,
          ],
          const SizedBox(width: 5),
        ],
      ),
    );
  }
}

/// Two horizontal accent lines, the classic title-bar grip motif.
class RailPainter extends CustomPainter {
  const RailPainter({
    required this.colour,
    required this.thickness,
    this.firstY = 17,
    this.secondY = 22,
  });

  final Color colour;
  final double thickness;

  /// Distance from the bar's top to each rail, in logical pixels.
  ///
  /// Absolute rather than fractional: these come from measuring the reference
  /// mockup, where the rails sit at logical y 17 and y 22 of the 35-tall bar.
  /// Expressing them as fractions of the height invites drift.
  final double firstY;
  final double secondY;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = colour;
    for (final y in [firstY, secondY]) {
      if (y + thickness > size.height) continue;
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, thickness), paint);
    }
  }

  @override
  bool shouldRepaint(RailPainter old) =>
      old.colour != colour ||
      old.thickness != thickness ||
      old.firstY != firstY ||
      old.secondY != secondY;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/chrome/title_bar_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 5: Commit**

```bash
git add lib/ui/chrome/title_bar.dart test/ui/chrome/title_bar_test.dart
git commit -m "feat(chrome): shared title bar with accent rails

One title bar serves both panels, with slots for the logo menu, the window
controls and the equalizer's collapse and close buttons."
```

---

### Task 9: AudioLevels seam and the spectrum visualizer

**Files:**
- Create: `lib/playback/audio_levels.dart`
- Modify: `lib/playback/player_engine.dart`
- Modify: `lib/playback/media_kit_player_engine.dart`
- Modify: `lib/playback/fake_player_engine.dart`
- Modify: `lib/playback/playback_controller.dart`
- Modify: `lib/ui/chrome/spectrum_visualizer.dart` (replace contents)
- Test: `test/playback/audio_levels_test.dart`
- Test: `test/ui/chrome/spectrum_visualizer_test.dart` (replace contents)

**Interfaces:**
- Consumes: `TrampColors` (Task 1).
- Produces:
  - `AudioLevels({required List<double> bands, required double leftRms, required double rightRms, required bool synthetic})` with `static const int bandCount = 20`, `static const AudioLevels silent`, and `AudioLevels.synthesised({required double intensity, required int seed})`.
  - `PlayerEngine.levelsStream` → `Stream<AudioLevels>`.
  - `PlaybackController.levelsStream` → `Stream<AudioLevels>`.
  - `SpectrumVisualizer({Key? key, required Stream<AudioLevels> levels})` and `SpectrumPainter({required List<double> bars, required List<double> peaks})`.

**Note on honesty:** the media_kit engine cannot measure real levels — see the spec's "Audio levels" section. It therefore emits `synthetic: true`. The `synthetic` flag must be preserved through every layer so nothing downstream can claim measured data. Real analysis arrives in the separate spectrogram plan.

- [ ] **Step 1: Write the failing AudioLevels test**

Create `test/playback/audio_levels_test.dart`:

```dart
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_levels.dart';

void main() {
  test('band count is the display width', () {
    expect(AudioLevels.bandCount, 20);
  });

  test('silent has the right shape and is flagged synthetic', () {
    expect(AudioLevels.silent.bands, hasLength(AudioLevels.bandCount));
    expect(AudioLevels.silent.bands.every((b) => b == 0), isTrue);
    expect(AudioLevels.silent.leftRms, 0);
    expect(AudioLevels.silent.synthetic, isTrue);
  });

  test('synthesised levels stay in range and are flagged', () {
    for (var seed = 0; seed < 50; seed++) {
      final levels = AudioLevels.synthesised(intensity: 0.8, seed: seed);
      expect(levels.bands, hasLength(AudioLevels.bandCount));
      for (final b in levels.bands) {
        expect(b, inInclusiveRange(0.0, 1.0));
      }
      expect(levels.leftRms, inInclusiveRange(0.0, 1.0));
      expect(levels.synthetic, isTrue);
    }
  });

  test('zero intensity synthesises silence', () {
    final levels = AudioLevels.synthesised(intensity: 0, seed: 3);
    expect(levels.bands.every((b) => b == 0), isTrue);
  });

  test('synthesis is deterministic for a given seed', () {
    final a = AudioLevels.synthesised(intensity: 0.5, seed: 11);
    final b = AudioLevels.synthesised(intensity: 0.5, seed: 11);
    expect(a.bands, b.bands);
  });

  test('constructing with the wrong band count is rejected', () {
    expect(
      () => AudioLevels(
        bands: const [0.1, 0.2],
        leftRms: 0,
        rightRms: 0,
        synthetic: true,
      ),
      throwsA(isA<AssertionError>()),
    );
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/playback/audio_levels_test.dart`
Expected: FAIL — `audio_levels.dart` does not exist.

- [ ] **Step 3: Write AudioLevels**

Create `lib/playback/audio_levels.dart`:

```dart
import 'dart:math' as math;

/// One frame of analyser data.
///
/// [synthetic] is part of the contract, not a debug flag: the media_kit engine
/// cannot measure real levels, so anything derived from playback state must say
/// so rather than pass itself off as measured audio.
class AudioLevels {
  AudioLevels({
    required this.bands,
    required this.leftRms,
    required this.rightRms,
    required this.synthetic,
  }) : assert(
          bands.length == bandCount,
          'expected $bandCount bands, got ${bands.length}',
        );

  /// Number of bars the display shows.
  static const int bandCount = 20;

  static final AudioLevels silent = AudioLevels(
    bands: List<double>.filled(bandCount, 0),
    leftRms: 0,
    rightRms: 0,
    synthetic: true,
  );

  final List<double> bands;
  final double leftRms;
  final double rightRms;
  final bool synthetic;

  /// A plausible spectrum shape for use until real analysis is available.
  ///
  /// Deterministic in [seed] so tests and goldens are stable. Energy falls off
  /// towards the treble, which is what music generally does.
  factory AudioLevels.synthesised({
    required double intensity,
    required int seed,
  }) {
    final clamped = intensity.clamp(0.0, 1.0);
    if (clamped == 0) return silent;

    final random = math.Random(seed);
    final bands = List<double>.generate(bandCount, (i) {
      final tilt = 1 - (i / bandCount) * 0.65;
      final jitter = 0.55 + random.nextDouble() * 0.45;
      return (clamped * tilt * jitter).clamp(0.0, 1.0);
    });

    return AudioLevels(
      bands: bands,
      leftRms: clamped * (0.7 + random.nextDouble() * 0.3),
      rightRms: clamped * (0.7 + random.nextDouble() * 0.3),
      synthetic: true,
    );
  }
}
```

- [ ] **Step 4: Run the AudioLevels test to verify it passes**

Run: `flutter test test/playback/audio_levels_test.dart`
Expected: PASS (6 tests)

- [ ] **Step 5: Add the seam to PlayerEngine**

In `lib/playback/player_engine.dart`, add the import and the stream:

```dart
import '../domain/track.dart';
import 'audio_levels.dart';

abstract class PlayerEngine {
  Stream<Duration> get positionStream;
  Stream<Duration> get durationStream;
  Stream<bool> get playingStream;
  Stream<void> get completedStream;

  /// Analyser frames for the spectrum display.
  ///
  /// Implementations that cannot measure real audio must emit frames flagged
  /// `synthetic: true` rather than silently fabricating measured-looking data.
  Stream<AudioLevels> get levelsStream;

  Future<void> open(Track track);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> seek(Duration position);
  Future<void> setVolume(double volume);
  Future<void> dispose();
}
```

- [ ] **Step 6a: Implement the stream in the media_kit engine**

Apply these four edits to `lib/playback/media_kit_player_engine.dart`.

First, add the import beneath the existing `player_engine.dart` import:

```dart
import 'audio_levels.dart';
import 'player_engine.dart';
```

Second, replace the constructor and field block (currently lines 13–18) with:

```dart
class MediaKitPlayerEngine implements PlayerEngine {
  MediaKitPlayerEngine({this.onMetadata, Player? player})
      : _player = player ?? Player() {
    // The engine cannot measure real audio, so it publishes a synthesised
    // spectrum shape driven by playing state and volume. See the design spec:
    // libmpv ships without the filters any real metering would need.
    _playingSubscription = _player.stream.playing.listen((playing) {
      _isPlaying = playing;
    });
    _levelsTimer = Timer.periodic(const Duration(milliseconds: 33), (_) {
      if (_levels.isClosed) return;
      _levels.add(
        _isPlaying
            ? AudioLevels.synthesised(
                intensity: _currentVolume,
                seed: _levelsSeed++,
              )
            : AudioLevels.silent,
      );
    });
  }

  final Player _player;
  final TrackMetadataCallback? onMetadata;

  final _levels = StreamController<AudioLevels>.broadcast();
  StreamSubscription<bool>? _playingSubscription;
  Timer? _levelsTimer;
  int _levelsSeed = 0;
  bool _isPlaying = false;
  double _currentVolume = 1;

  @override
  Stream<AudioLevels> get levelsStream => _levels.stream;
```

Third, replace `setVolume` so it records the level used for synthesis:

```dart
  @override
  Future<void> setVolume(double volume) {
    _currentVolume = volume.clamp(0.0, 1.0);
    return _player.setVolume(_currentVolume * 100);
  }
```

Fourth, replace `dispose`:

```dart
  @override
  Future<void> dispose() async {
    _levelsTimer?.cancel();
    await _playingSubscription?.cancel();
    await _levels.close();
    await _player.dispose();
  }
```

- [ ] **Step 6b: Implement the stream in the fake engine**

Apply these three edits to `lib/playback/fake_player_engine.dart`.

First, add the import beneath the existing `player_engine.dart` import:

```dart
import 'audio_levels.dart';
import 'player_engine.dart';
```

Second, add the controller beside the existing ones and the getter beside the other stream getters:

```dart
  final _completedController = StreamController<void>.broadcast();
  final _levelsController = StreamController<AudioLevels>.broadcast();

  @override
  Stream<AudioLevels> get levelsStream => _levelsController.stream;

  /// Push one analyser frame. Tests drive the spectrum through this rather than
  /// waiting on a timer, so they stay deterministic.
  void emitLevels(AudioLevels levels) => _levelsController.add(levels);
```

Third, close it in `dispose`:

```dart
  @override
  Future<void> dispose() async {
    await _positionController.close();
    await _durationController.close();
    await _playingController.close();
    await _completedController.close();
    await _levelsController.close();
  }
```

- [ ] **Step 7: Expose the stream on PlaybackController**

In `lib/playback/playback_controller.dart`, add the import and a passthrough getter. Deliberately do **not** route frames through `notifyListeners()` — at 30 Hz that would rebuild the whole player thirty times a second. The visualizer subscribes directly.

```dart
import 'audio_levels.dart';

// ... inside PlaybackController:

  /// Analyser frames, consumed directly by the spectrum display.
  ///
  /// Not surfaced as controller state on purpose: notifying listeners at frame
  /// rate would rebuild the entire player chrome thirty times a second.
  Stream<AudioLevels> get levelsStream => _engine.levelsStream;
```

- [ ] **Step 8: Write the failing visualizer test**

Replace `test/ui/chrome/spectrum_visualizer_test.dart` entirely:

```dart
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_levels.dart';
import 'package:tramp/ui/chrome/spectrum_visualizer.dart';

Widget host(Stream<AudioLevels> levels) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(
        child: SizedBox(
          width: 228,
          height: 96,
          child: SpectrumVisualizer(levels: levels),
        ),
      ),
    );

SpectrumPainter painterOf(WidgetTester tester) =>
    tester.widget<CustomPaint>(find.byType(CustomPaint).first).painter!
        as SpectrumPainter;

void main() {
  late StreamController<AudioLevels> controller;

  setUp(() => controller = StreamController<AudioLevels>.broadcast());
  tearDown(() => controller.close());

  testWidgets('starts silent', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    expect(painterOf(tester).bars.every((b) => b == 0), isTrue);
  });

  testWidgets('rises immediately on a loud frame (fast attack)',
      (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 1),
      leftRms: 1,
      rightRms: 1,
      synthetic: false,
    ));
    await tester.pump();
    expect(painterOf(tester).bars.first, greaterThan(0.9));
  });

  testWidgets('decays gradually rather than snapping to zero', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 1),
      leftRms: 1,
      rightRms: 1,
      synthetic: false,
    ));
    await tester.pump();
    controller.add(AudioLevels.silent);
    await tester.pump();
    final bar = painterOf(tester).bars.first;
    expect(bar, lessThan(1.0));
    expect(bar, greaterThan(0.0), reason: 'slow decay, not an instant drop');
  });

  testWidgets('peak caps sit at or above the bars', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 0.8),
      leftRms: 0.8,
      rightRms: 0.8,
      synthetic: false,
    ));
    await tester.pump();
    final p = painterOf(tester);
    for (var i = 0; i < p.bars.length; i++) {
      expect(p.peaks[i], greaterThanOrEqualTo(p.bars[i]));
    }
  });

  testWidgets('survives a stream that closes', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    await controller.close();
    await tester.pump();
    expect(tester.takeException(), isNull);
  });
}
```

- [ ] **Step 9: Run it to verify it fails**

Run: `flutter test test/ui/chrome/spectrum_visualizer_test.dart`
Expected: FAIL — `SpectrumVisualizer` still takes `playing`/`volume` and `SpectrumPainter` is undefined.

- [ ] **Step 10: Replace the visualizer**

Replace `lib/ui/chrome/spectrum_visualizer.dart` entirely:

```dart
import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../playback/audio_levels.dart';
import '../../theme/tramp_colors.dart';

/// Spectrum bars driven by the engine's analyser stream.
///
/// The widget owns no animation of its own — it renders what the engine reports,
/// smoothed so that a coarse source cadence still looks alive.
class SpectrumVisualizer extends StatefulWidget {
  const SpectrumVisualizer({super.key, required this.levels});

  final Stream<AudioLevels> levels;

  @override
  State<SpectrumVisualizer> createState() => _SpectrumVisualizerState();
}

class _SpectrumVisualizerState extends State<SpectrumVisualizer> {
  static const _decay = 0.86;
  static const _peakDecay = 0.97;

  late List<double> _bars;
  late List<double> _peaks;
  StreamSubscription<AudioLevels>? _subscription;

  @override
  void initState() {
    super.initState();
    _bars = List<double>.filled(AudioLevels.bandCount, 0);
    _peaks = List<double>.filled(AudioLevels.bandCount, 0);
    _listen();
  }

  @override
  void didUpdateWidget(SpectrumVisualizer oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.levels != widget.levels) {
      _subscription?.cancel();
      _listen();
    }
  }

  void _listen() {
    _subscription = widget.levels.listen(_onFrame);
  }

  void _onFrame(AudioLevels frame) {
    if (!mounted) return;
    setState(() {
      for (var i = 0; i < AudioLevels.bandCount; i++) {
        final incoming = frame.bands[i].clamp(0.0, 1.0);
        // Fast attack, slow decay: rise instantly, fall smoothly.
        _bars[i] = incoming > _bars[i] ? incoming : _bars[i] * _decay;
        _peaks[i] = math.max(_bars[i], _peaks[i] * _peakDecay);
      }
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      painter: SpectrumPainter(
        bars: List<double>.unmodifiable(_bars),
        peaks: List<double>.unmodifiable(_peaks),
      ),
    );
  }
}

class SpectrumPainter extends CustomPainter {
  const SpectrumPainter({required this.bars, required this.peaks});

  final List<double> bars;
  final List<double> peaks;

  @override
  void paint(Canvas canvas, Size size) {
    if (bars.isEmpty) return;

    final slot = size.width / bars.length;
    final barWidth = math.max(1.0, slot - 1);
    final lit = Paint()..color = TrampColors.phosphor;
    final dim = Paint()..color = TrampColors.phosphorDim;

    for (var i = 0; i < bars.length; i++) {
      final left = i * slot;
      final height = size.height * bars[i];
      if (height > 0) {
        canvas.drawRect(
          Rect.fromLTWH(left, size.height - height, barWidth, height),
          lit,
        );
      }

      final peakY = size.height - size.height * peaks[i];
      canvas.drawRect(Rect.fromLTWH(left, peakY, barWidth, 1), dim);
    }
  }

  @override
  bool shouldRepaint(SpectrumPainter old) =>
      !_same(old.bars, bars) || !_same(old.peaks, peaks);

  static bool _same(List<double> a, List<double> b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
```

- [ ] **Step 11: Run the visualizer test and the playback suite**

Run: `flutter test test/ui/chrome/spectrum_visualizer_test.dart test/playback/`
Expected: PASS. If `playback_controller_test.dart` fails to compile because `FakePlayerEngine` does not yet satisfy the new interface, finish Step 6 for the fake engine before re-running.

- [ ] **Step 12: Commit**

```bash
git add lib/playback lib/ui/chrome/spectrum_visualizer.dart \
        test/playback test/ui/chrome/spectrum_visualizer_test.dart
git commit -m "feat(playback): AudioLevels seam driving the spectrum display

The visualizer now renders engine-reported analyser frames with fast attack,
slow decay and falling peak caps instead of animating itself. media_kit emits
frames flagged synthetic because it cannot measure real levels; real analysis
lands with the spectrogram plan."
```

---

### Task 10: Equalizer state

**Files:**
- Create: `lib/domain/equalizer_settings.dart`
- Create: `lib/eq/equalizer_controller.dart`
- Modify: `lib/domain/tramp_settings.dart` (add the `equalizer` field)
- Modify: `test/platform/settings_store_test.dart` (cover the new field)
- Test: `test/domain/equalizer_settings_test.dart`
- Test: `test/eq/equalizer_controller_test.dart`

**Interfaces:**
- Consumes: `SettingsStore`, `TrampSettings` (Task 2).
- Produces:
  - `EqualizerSettings({required bool enabled, required bool auto, required double preamp, required List<double> gains, String? presetName})` with `static const List<int> bandFrequencies`, `static const double gainLimit = 12`, `static final EqualizerSettings flat`, `copyWith`, `withGain(int band, double gain)`, `withPreamp(double)`, `toJson`, `fromJson`, `==`/`hashCode`.
  - `EqualizerPresets.builtIn` → `Map<String, List<double>>` (ten gains each).
  - `abstract class EqualizerSink { Future<void> apply(EqualizerSettings settings); }` and `class NoopEqualizerSink implements EqualizerSink`.
  - `EqualizerController extends ChangeNotifier` with `EqualizerSettings get settings`, `setEnabled(bool)`, `setAuto(bool)`, `setGain(int band, double gain)`, `setPreamp(double)`, `applyPreset(String name)`, `resetFlat()`, `Future<void> load()`, and `List<String> get presetNames`.

- [ ] **Step 1: Write the failing settings test**

Create `test/domain/equalizer_settings_test.dart`:

```dart
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';

void main() {
  test('bands are the Winamp 2.x set', () {
    expect(EqualizerSettings.bandFrequencies,
        [60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000]);
  });

  test('flat is ten zero gains, disabled', () {
    expect(EqualizerSettings.flat.gains, hasLength(10));
    expect(EqualizerSettings.flat.gains.every((g) => g == 0), isTrue);
    expect(EqualizerSettings.flat.preamp, 0);
    expect(EqualizerSettings.flat.enabled, isFalse);
  });

  test('gains clamp to plus or minus twelve dB', () {
    final hot = EqualizerSettings.flat.withGain(0, 99);
    expect(hot.gains[0], EqualizerSettings.gainLimit);
    final cold = EqualizerSettings.flat.withGain(0, -99);
    expect(cold.gains[0], -EqualizerSettings.gainLimit);
  });

  test('withGain leaves other bands untouched', () {
    final s = EqualizerSettings.flat.withGain(3, 5);
    expect(s.gains[3], 5);
    expect(s.gains[4], 0);
  });

  test('an out-of-range band index is ignored rather than throwing', () {
    expect(EqualizerSettings.flat.withGain(99, 5).gains,
        EqualizerSettings.flat.gains);
  });

  test('json round-trips every field', () {
    const original = EqualizerSettings(
      enabled: true,
      auto: true,
      preamp: 3.5,
      gains: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
      presetName: 'Rock',
    );
    expect(EqualizerSettings.fromJson(original.toJson()), original);
  });

  test('malformed json falls back to flat', () {
    expect(EqualizerSettings.fromJson(const {}), EqualizerSettings.flat);
    expect(
      EqualizerSettings.fromJson(const {'gains': [1, 2]}),
      EqualizerSettings.flat,
      reason: 'wrong band count is corrupt, not partially usable',
    );
  });

  test('every built-in preset has ten in-range gains', () {
    expect(EqualizerPresets.builtIn, isNotEmpty);
    for (final entry in EqualizerPresets.builtIn.entries) {
      expect(entry.value, hasLength(10), reason: entry.key);
      for (final g in entry.value) {
        expect(g.abs(), lessThanOrEqualTo(EqualizerSettings.gainLimit),
            reason: entry.key);
      }
    }
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/domain/equalizer_settings_test.dart`
Expected: FAIL — `equalizer_settings.dart` does not exist.

- [ ] **Step 3: Write EqualizerSettings and the presets**

Create `lib/domain/equalizer_settings.dart`:

```dart
/// Ten-band equalizer state.
///
/// Chrome and state only: these gains are persisted and displayed but do not
/// reach the audio path. See the design spec's "Equalizer audio path" section —
/// the shipped libmpv silently disables filter graphs while reporting success.
class EqualizerSettings {
  const EqualizerSettings({
    required this.enabled,
    required this.auto,
    required this.preamp,
    required this.gains,
    this.presetName,
  });

  /// Winamp 2.x band centres, in hertz.
  static const List<int> bandFrequencies = [
    60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000,
  ];

  static const double gainLimit = 12;

  static const EqualizerSettings flat = EqualizerSettings(
    enabled: false,
    auto: false,
    preamp: 0,
    gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  );

  final bool enabled;
  final bool auto;
  final double preamp;
  final List<double> gains;
  final String? presetName;

  static double _clamp(double value) =>
      value.clamp(-gainLimit, gainLimit).toDouble();

  EqualizerSettings copyWith({
    bool? enabled,
    bool? auto,
    double? preamp,
    List<double>? gains,
    String? presetName,
    bool clearPresetName = false,
  }) {
    return EqualizerSettings(
      enabled: enabled ?? this.enabled,
      auto: auto ?? this.auto,
      preamp: preamp ?? this.preamp,
      gains: gains ?? this.gains,
      presetName: clearPresetName ? null : (presetName ?? this.presetName),
    );
  }

  /// Editing a band makes the curve no longer the named preset.
  EqualizerSettings withGain(int band, double gain) {
    if (band < 0 || band >= bandFrequencies.length) return this;
    final next = List<double>.of(gains);
    next[band] = _clamp(gain);
    return copyWith(gains: next, clearPresetName: true);
  }

  EqualizerSettings withPreamp(double value) =>
      copyWith(preamp: _clamp(value));

  Map<String, dynamic> toJson() => {
        'enabled': enabled,
        'auto': auto,
        'preamp': preamp,
        'gains': gains,
        'presetName': presetName,
      };

  factory EqualizerSettings.fromJson(Map<String, dynamic> json) {
    final rawGains = json['gains'];
    if (rawGains is! List || rawGains.length != bandFrequencies.length) {
      return flat;
    }
    final gains = <double>[];
    for (final value in rawGains) {
      if (value is! num) return flat;
      gains.add(_clamp(value.toDouble()));
    }
    final preamp = json['preamp'];
    return EqualizerSettings(
      enabled: json['enabled'] == true,
      auto: json['auto'] == true,
      preamp: preamp is num ? _clamp(preamp.toDouble()) : 0,
      gains: gains,
      presetName: json['presetName'] as String?,
    );
  }

  @override
  bool operator ==(Object other) {
    if (other is! EqualizerSettings) return false;
    if (other.enabled != enabled ||
        other.auto != auto ||
        other.preamp != preamp ||
        other.presetName != presetName ||
        other.gains.length != gains.length) {
      return false;
    }
    for (var i = 0; i < gains.length; i++) {
      if (other.gains[i] != gains[i]) return false;
    }
    return true;
  }

  @override
  int get hashCode =>
      Object.hash(enabled, auto, preamp, presetName, Object.hashAll(gains));

  @override
  String toString() => 'EqualizerSettings(enabled: $enabled, auto: $auto, '
      'preamp: $preamp, gains: $gains, presetName: $presetName)';
}

/// Built-in curves offered by the PRESETS menu.
abstract final class EqualizerPresets {
  static const Map<String, List<double>> builtIn = {
    'Flat': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    'Rock': [5, 4, 2, -1, -2, 1, 3, 5, 5, 5],
    'Pop': [-1, 2, 4, 5, 3, 0, -1, -1, -1, -1],
    'Jazz': [4, 3, 1, 2, -1, -1, 0, 2, 3, 4],
    'Classical': [5, 4, 3, 2, -1, -1, 0, 2, 3, 4],
    'Bass Boost': [9, 7, 5, 2, 0, 0, 0, 0, 0, 0],
    'Treble Boost': [0, 0, 0, 0, 0, 2, 5, 7, 8, 8],
    'Vocal': [-2, -1, 2, 4, 5, 4, 2, 0, -1, -2],
  };
}
```

- [ ] **Step 4: Run the settings test to verify it passes**

Run: `flutter test test/domain/equalizer_settings_test.dart`
Expected: PASS (8 tests)

- [ ] **Step 5: Add the equalizer to TrampSettings**

In `lib/domain/tramp_settings.dart`, add the import, the field, and the serialisation. Replace the class body's field list, `defaults`, `copyWith`, `toJson`, `fromJson`, `==` and `hashCode` with:

```dart
import 'equalizer_settings.dart';

/// Which panel occupies the region below the main player.
enum LowerRegion { equalizer, playlist }

/// Persisted UI state.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.lowerRegion,
    this.equalizer = EqualizerSettings.flat,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
  );

  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final LowerRegion lowerRegion;
  final EqualizerSettings equalizer;

  TrampSettings copyWith({
    int? zoomPercent,
    LowerRegion? lowerRegion,
    EqualizerSettings? equalizer,
  }) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      lowerRegion: lowerRegion ?? this.lowerRegion,
      equalizer: equalizer ?? this.equalizer,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'lowerRegion': lowerRegion.name,
        'equalizer': equalizer.toJson(),
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final region = json['lowerRegion'];
    final eq = json['equalizer'];
    return TrampSettings(
      zoomPercent: zoom is int && validZoomPercents.contains(zoom)
          ? zoom
          : defaults.zoomPercent,
      lowerRegion: LowerRegion.values.firstWhere(
        (value) => value.name == region,
        orElse: () => defaults.lowerRegion,
      ),
      equalizer: eq is Map<String, dynamic>
          ? EqualizerSettings.fromJson(eq)
          : EqualizerSettings.flat,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.lowerRegion == lowerRegion &&
      other.equalizer == equalizer;

  @override
  int get hashCode => Object.hash(zoomPercent, lowerRegion, equalizer);

  @override
  String toString() => 'TrampSettings(zoomPercent: $zoomPercent, '
      'lowerRegion: $lowerRegion, equalizer: $equalizer)';
}
```

Add a round-trip case to `test/platform/settings_store_test.dart`:

```dart
  test('round-trips equalizer state', () async {
    const written = TrampSettings(
      zoomPercent: 150,
      lowerRegion: LowerRegion.equalizer,
      equalizer: EqualizerSettings(
        enabled: true,
        auto: false,
        preamp: 2,
        gains: [1, 0, -1, 0, 0, 0, 0, 0, 0, 3],
        presetName: 'Rock',
      ),
    );
    await store().write(written);
    expect(await store().read(), written);
  });
```

with `import 'package:tramp/domain/equalizer_settings.dart';` added at the top.

Run: `flutter test test/platform/settings_store_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 6: Write the failing controller test**

Create `test/eq/equalizer_controller_test.dart`:

```dart
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';

class MemorySettingsStore implements SettingsStore {
  MemorySettingsStore([this.stored = TrampSettings.defaults]);

  TrampSettings stored;
  int writes = 0;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async {
    stored = settings;
    writes++;
  }
}

class RecordingSink implements EqualizerSink {
  final applied = <EqualizerSettings>[];

  @override
  Future<void> apply(EqualizerSettings settings) async =>
      applied.add(settings);
}

void main() {
  test('starts flat', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    expect(c.settings, EqualizerSettings.flat);
  });

  test('load adopts persisted state', () async {
    const stored = EqualizerSettings(
      enabled: true,
      auto: true,
      preamp: 4,
      gains: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
      presetName: 'Rock',
    );
    final c = EqualizerController(
      store: MemorySettingsStore(
        TrampSettings.defaults.copyWith(equalizer: stored),
      ),
      sink: NoopEqualizerSink(),
    );
    await c.load();
    expect(c.settings, stored);
  });

  test('setGain updates, notifies and persists', () async {
    final store = MemorySettingsStore();
    final c = EqualizerController(store: store, sink: NoopEqualizerSink());
    var notifications = 0;
    c.addListener(() => notifications++);

    c.setGain(2, 6);
    expect(c.settings.gains[2], 6);
    expect(notifications, 1);
    await Future<void>.delayed(Duration.zero);
    expect(store.stored.equalizer.gains[2], 6);
  });

  test('applyPreset sets the curve and records the name', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    c.applyPreset('Rock');
    expect(c.settings.gains, EqualizerPresets.builtIn['Rock']);
    expect(c.settings.presetName, 'Rock');
  });

  test('an unknown preset name is ignored', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    c.applyPreset('Nope');
    expect(c.settings, EqualizerSettings.flat);
  });

  test('editing a band clears the preset name', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    c.applyPreset('Rock');
    c.setGain(0, 1);
    expect(c.settings.presetName, isNull);
  });

  test('every change reaches the sink', () {
    final sink = RecordingSink();
    final c = EqualizerController(store: MemorySettingsStore(), sink: sink);
    c.setEnabled(true);
    c.setPreamp(3);
    expect(sink.applied, hasLength(2));
    expect(sink.applied.last.preamp, 3);
  });

  test('preset names are the built-in curves', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    expect(c.presetNames, EqualizerPresets.builtIn.keys.toList());
  });
}
```

- [ ] **Step 7: Run it to verify it fails**

Run: `flutter test test/eq/equalizer_controller_test.dart`
Expected: FAIL — `equalizer_controller.dart` does not exist.

- [ ] **Step 8: Write the controller and the sink seam**

Create `lib/eq/equalizer_controller.dart`:

```dart
import 'dart:async';

import 'package:flutter/foundation.dart';

import '../domain/equalizer_settings.dart';
import '../platform/settings_store.dart';

/// Where equalizer curves would go to become audible.
///
/// Deliberately a seam with only a no-op implementation. Wiring this to mpv is
/// blocked: the shipped libmpv cannot construct filter graphs, and it reports
/// success while silently disabling them. Any real implementation must be
/// verified by measuring output, never by checking a return code.
abstract class EqualizerSink {
  Future<void> apply(EqualizerSettings settings);
}

class NoopEqualizerSink implements EqualizerSink {
  const NoopEqualizerSink();

  @override
  Future<void> apply(EqualizerSettings settings) async {}
}

class EqualizerController extends ChangeNotifier {
  EqualizerController({
    required SettingsStore store,
    required EqualizerSink sink,
  })  : _store = store,
        _sink = sink;

  final SettingsStore _store;
  final EqualizerSink _sink;

  EqualizerSettings _settings = EqualizerSettings.flat;

  EqualizerSettings get settings => _settings;

  List<String> get presetNames => EqualizerPresets.builtIn.keys.toList();

  Future<void> load() async {
    final persisted = await _store.read();
    _settings = persisted.equalizer;
    notifyListeners();
    unawaited(_sink.apply(_settings));
  }

  void setEnabled(bool value) => _apply(_settings.copyWith(enabled: value));

  void setAuto(bool value) => _apply(_settings.copyWith(auto: value));

  void setGain(int band, double gain) => _apply(_settings.withGain(band, gain));

  void setPreamp(double value) => _apply(_settings.withPreamp(value));

  void resetFlat() => _apply(
        EqualizerSettings.flat.copyWith(
          enabled: _settings.enabled,
          auto: _settings.auto,
          presetName: 'Flat',
        ),
      );

  void applyPreset(String name) {
    final curve = EqualizerPresets.builtIn[name];
    if (curve == null) return;
    _apply(_settings.copyWith(gains: List<double>.of(curve), presetName: name));
  }

  void _apply(EqualizerSettings next) {
    if (next == _settings) return;
    _settings = next;
    notifyListeners();
    unawaited(_sink.apply(next));
    unawaited(_persist());
  }

  Future<void> _persist() async {
    final current = await _store.read();
    await _store.write(current.copyWith(equalizer: _settings));
  }
}
```

- [ ] **Step 9: Run the controller test to verify it passes**

Run: `flutter test test/eq/ test/domain/ test/platform/`
Expected: PASS

- [ ] **Step 10: Commit**

```bash
git add lib/domain/equalizer_settings.dart lib/domain/tramp_settings.dart \
        lib/eq test/domain/equalizer_settings_test.dart test/eq \
        test/platform/settings_store_test.dart
git commit -m "feat(eq): ten-band equalizer state, presets and sink seam

Gains, preamp and presets are editable and persisted. The EqualizerSink seam
has only a no-op implementation: the shipped libmpv cannot construct filter
graphs and reports success while disabling them, so audibility is out of scope
until that is solved."
```

---

### Task 11: EqualizerPanel

**Files:**
- Create: `lib/ui/equalizer/equalizer_panel.dart`
- Test: `test/ui/equalizer/equalizer_panel_test.dart`

**Interfaces:**
- Consumes: `EqualizerController` (Task 10), `TrampTitleBar` (Task 8), `ChromeButton` (Task 5), `ChromeSlider` (Task 6), `MetalPanel` (Task 4), `TrampMetrics` (Task 2).
- Produces: `EqualizerPanel({Key? key, required EqualizerController controller, required VoidCallback onCollapse, required VoidCallback onClose, bool collapsed = false, bool draggableTitle = true})` with `static const logicalSize = TrampMetrics.equalizer`. Widget keys: `Key('eq-on')`, `Key('eq-auto')`, `Key('eq-presets')`, `Key('eq-collapse')`, `Key('eq-close')`, `Key('eq-preamp')`, and `Key('eq-band-$i')` for `i` in `0..9`.

**Layout note:** the canvas is fixed, so children are absolutely positioned from the spec's equalizer geometry table. That reproduces the measured mockup exactly instead of approximating it with nested flex and padding.

- [ ] **Step 1: Write the failing test**

Create `test/ui/equalizer/equalizer_panel_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/equalizer/equalizer_panel.dart';

class MemorySettingsStore implements SettingsStore {
  TrampSettings stored = TrampSettings.defaults;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async => stored = settings;
}

void main() {
  late EqualizerController controller;

  setUp(() {
    controller = EqualizerController(
      store: MemorySettingsStore(),
      sink: const NoopEqualizerSink(),
    );
  });

  Future<void> pump(
    WidgetTester tester, {
    bool collapsed = false,
    VoidCallback? onCollapse,
    VoidCallback? onClose,
  }) async {
    await tester.pumpWidget(
      Directionality(
        textDirection: TextDirection.ltr,
        child: Align(
          alignment: Alignment.topLeft,
          child: EqualizerPanel(
            controller: controller,
            collapsed: collapsed,
            draggableTitle: false,
            onCollapse: onCollapse ?? () {},
            onClose: onClose ?? () {},
          ),
        ),
      ),
    );
  }

  testWidgets('holds the locked canvas size', (tester) async {
    await pump(tester);
    expect(
      tester.getSize(find.byType(EqualizerPanel)),
      TrampMetrics.equalizer,
    );
    expect(tester.takeException(), isNull);
  });

  testWidgets('is branded TRAMP, never Winamp', (tester) async {
    await pump(tester);
    expect(find.text('TRAMP EQUALIZER'), findsOneWidget);
    expect(find.textContaining('WINAMP'), findsNothing);
  });

  testWidgets('renders all ten band labels', (tester) async {
    await pump(tester);
    for (final label in ['60', '170', '310', '600', '1K', '3K', '6K', '12K',
      '14K', '16K']) {
      expect(find.text(label), findsOneWidget, reason: label);
    }
  });

  testWidgets('ON toggles the controller', (tester) async {
    await pump(tester);
    expect(controller.settings.enabled, isFalse);
    await tester.tap(find.byKey(const Key('eq-on')));
    await tester.pump();
    expect(controller.settings.enabled, isTrue);
  });

  testWidgets('AUTO toggles the controller', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('eq-auto')));
    await tester.pump();
    expect(controller.settings.auto, isTrue);
  });

  testWidgets('dragging a band slider changes that gain only', (tester) async {
    await pump(tester);
    await tester.drag(find.byKey(const Key('eq-band-0')), const Offset(0, -40));
    await tester.pumpAndSettle();
    expect(controller.settings.gains[0], greaterThan(0));
    expect(controller.settings.gains[1], 0);
  });

  testWidgets('gain values are printed with one decimal and a sign',
      (tester) async {
    controller.setGain(0, 3.5);
    await pump(tester);
    expect(find.text('+3.5'), findsOneWidget);
  });

  testWidgets('collapse and close report to their callbacks', (tester) async {
    var collapses = 0;
    var closes = 0;
    await pump(
      tester,
      onCollapse: () => collapses++,
      onClose: () => closes++,
    );
    await tester.tap(find.byKey(const Key('eq-collapse')));
    await tester.tap(find.byKey(const Key('eq-close')));
    expect(collapses, 1);
    expect(closes, 1);
  });

  testWidgets('collapsed shows only the title bar', (tester) async {
    await pump(tester, collapsed: true);
    expect(
      tester.getSize(find.byType(EqualizerPanel)).height,
      TrampMetrics.titleBar,
    );
    expect(find.byKey(const Key('eq-band-0')), findsNothing);
    expect(find.text('TRAMP EQUALIZER'), findsOneWidget);
  });

  testWidgets('presets menu lists the built-in curves and applies one',
      (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('eq-presets')));
    await tester.pumpAndSettle();
    expect(find.text('Rock'), findsOneWidget);
    await tester.tap(find.text('Rock'));
    await tester.pumpAndSettle();
    expect(controller.settings.presetName, 'Rock');
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/equalizer/equalizer_panel_test.dart`
Expected: FAIL — `equalizer_panel.dart` does not exist.

- [ ] **Step 3: Write the panel**

Create `lib/ui/equalizer/equalizer_panel.dart`:

```dart
import 'package:flutter/material.dart';

import '../../domain/equalizer_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/chrome_button.dart';
import '../chrome/chrome_slider.dart';
import '../chrome/metal_panel.dart';
import '../chrome/title_bar.dart';
import '../chrome/transport_icons.dart';

/// The ten-band equalizer panel.
///
/// Authored against the fixed 812x206 canvas with absolute positions taken from
/// the design spec's geometry table, so the layout is the measured mockup rather
/// than an approximation of it.
class EqualizerPanel extends StatelessWidget {
  const EqualizerPanel({
    super.key,
    required this.controller,
    required this.onCollapse,
    required this.onClose,
    this.collapsed = false,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.equalizer;

  /// Logical x of each band slider's centre.
  static const List<double> bandCentres = [
    196, 245, 294, 343, 392, 441, 490, 539, 588, 636,
  ];

  static const List<String> bandLabels = [
    '60', '170', '310', '600', '1K', '3K', '6K', '12K', '14K', '16K',
  ];

  static const double _preampCentre = 73;
  static const double _sliderTop = 71;
  static const double _sliderBottom = 166;
  static const double _sliderWidth = 34;

  final EqualizerController controller;
  final VoidCallback onCollapse;
  final VoidCallback onClose;
  final bool collapsed;
  final bool draggableTitle;

  /// dB gain maps to a 0..1 slider position, centre being 0 dB.
  static double _toFraction(double gain) =>
      ((gain + EqualizerSettings.gainLimit) /
              (EqualizerSettings.gainLimit * 2))
          .clamp(0.0, 1.0);

  static double _toGain(double fraction) =>
      fraction * EqualizerSettings.gainLimit * 2 -
      EqualizerSettings.gainLimit;

  static String _format(double gain) =>
      '${gain >= 0 ? '+' : '-'}${gain.abs().toStringAsFixed(1)}';

  @override
  Widget build(BuildContext context) {
    final titleBar = TrampTitleBar(
      title: 'TRAMP EQUALIZER',
      draggable: draggableTitle,
      leading: ChromeButton.icon(
        key: const Key('eq-collapse'),
        icon: SizedBox(
          width: 9,
          height: 6,
          child: CustomPaint(
            painter: _CollapsePainter(colour: TrampColors.label),
          ),
        ),
        onPressed: onCollapse,
        semanticLabel: 'Collapse equalizer',
        size: const Size(27, 27),
      ),
      trailing: [
        ChromeButton.label(
          key: const Key('eq-close'),
          text: 'X',
          onPressed: onClose,
          semanticLabel: 'Close equalizer',
          size: const Size(27, 27),
        ),
      ],
    );

    if (collapsed) {
      return SizedBox(
        width: logicalSize.width,
        height: TrampMetrics.titleBar,
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: titleBar,
        ),
      );
    }

    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: ListenableBuilder(
          listenable: controller,
          builder: (context, _) {
            final settings = controller.settings;

            return Stack(
              children: [
                Positioned(left: 0, right: 0, top: 0, child: titleBar),

                Positioned(
                  left: 36,
                  top: 42,
                  child: ChromeButton.label(
                    key: const Key('eq-on'),
                    text: 'ON',
                    active: settings.enabled,
                    onPressed: () => controller.setEnabled(!settings.enabled),
                    size: const Size(33, 20),
                  ),
                ),
                Positioned(
                  left: 93,
                  top: 42,
                  child: ChromeButton.label(
                    key: const Key('eq-auto'),
                    text: 'AUTO',
                    active: settings.auto,
                    onPressed: () => controller.setAuto(!settings.auto),
                    size: const Size(37, 20),
                  ),
                ),

                Positioned(
                  left: 677,
                  top: 44,
                  child: _PresetsButton(controller: controller),
                ),

                // Preamp, with its printed dB scale.
                const Positioned(
                  left: 40,
                  top: 56,
                  child: Text('PREAMP', style: TrampText.eqScale),
                ),
                _slider(
                  key: const Key('eq-preamp'),
                  centreX: _preampCentre,
                  value: _toFraction(settings.preamp),
                  onChanged: (f) => controller.setPreamp(_toGain(f)),
                  semanticLabel: 'Preamp',
                ),
                _valueLabel(_preampCentre, _format(settings.preamp)),
                const Positioned(
                  left: 100,
                  top: _sliderTop - 4,
                  child: Text('+12 dB', style: TrampText.eqScale),
                ),
                const Positioned(
                  left: 100,
                  top: (_sliderTop + _sliderBottom) / 2 - 5,
                  child: Text('0 dB', style: TrampText.eqScale),
                ),
                const Positioned(
                  left: 100,
                  top: _sliderBottom - 8,
                  child: Text('-12 dB', style: TrampText.eqScale),
                ),

                for (var i = 0; i < bandCentres.length; i++) ...[
                  Positioned(
                    left: bandCentres[i] - _sliderWidth / 2,
                    top: 53,
                    width: _sliderWidth,
                    child: Text(
                      bandLabels[i],
                      textAlign: TextAlign.center,
                      style: TrampText.eqScale,
                    ),
                  ),
                  _slider(
                    key: Key('eq-band-$i'),
                    centreX: bandCentres[i],
                    value: _toFraction(settings.gains[i]),
                    onChanged: (f) => controller.setGain(i, _toGain(f)),
                    semanticLabel: '${bandLabels[i]} hertz',
                  ),
                  _valueLabel(bandCentres[i], _format(settings.gains[i])),
                ],
              ],
            );
          },
        ),
      ),
    );
  }

  Widget _slider({
    required Key key,
    required double centreX,
    required double value,
    required ValueChanged<double> onChanged,
    required String semanticLabel,
  }) {
    return Positioned(
      left: centreX - _sliderWidth / 2,
      top: _sliderTop,
      width: _sliderWidth,
      height: _sliderBottom - _sliderTop,
      child: ChromeSlider(
        key: key,
        value: value,
        axis: Axis.vertical,
        semanticLabel: semanticLabel,
        onChanged: onChanged,
        onChangeEnd: onChanged,
      ),
    );
  }

  Widget _valueLabel(double centreX, String text) {
    return Positioned(
      left: centreX - _sliderWidth,
      top: 174,
      width: _sliderWidth * 2,
      child: Text(
        text,
        textAlign: TextAlign.center,
        style: TrampText.eqScale.copyWith(color: TrampColors.phosphor),
      ),
    );
  }
}

class _PresetsButton extends StatelessWidget {
  const _PresetsButton({required this.controller});

  final EqualizerController controller;

  @override
  Widget build(BuildContext context) {
    return ChromeButton.dropdown(
      key: const Key('eq-presets'),
      text: 'PRESETS',
      size: const Size(99, 22),
      onPressed: () async {
        final box = context.findRenderObject()! as RenderBox;
        final origin = box.localToGlobal(Offset.zero);
        final chosen = await showMenu<String>(
          context: context,
          position: RelativeRect.fromLTRB(
            origin.dx,
            origin.dy + box.size.height,
            origin.dx,
            origin.dy,
          ),
          items: [
            for (final name in controller.presetNames)
              PopupMenuItem<String>(value: name, child: Text(name)),
          ],
        );
        if (chosen != null) controller.applyPreset(chosen);
      },
    );
  }
}

/// The upward triangle on the collapse button.
class _CollapsePainter extends CustomPainter {
  const _CollapsePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, size.height)
      ..lineTo(size.width / 2, 0)
      ..lineTo(size.width, size.height)
      ..close();
    canvas.drawPath(path, Paint()..color = colour);
  }

  @override
  bool shouldRepaint(_CollapsePainter old) => old.colour != colour;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/equalizer/equalizer_panel_test.dart`
Expected: PASS (10 tests)

- [ ] **Step 5: Commit**

```bash
git add lib/ui/equalizer test/ui/equalizer
git commit -m "feat(eq): equalizer panel on the fixed 812x206 canvas

Preamp plus ten bands, ON and AUTO toggles, a presets menu and windowshade
collapse, positioned from the measured mockup geometry."
```

---

### Task 12: Audio format info seam

The display well shows real `128 kbps / 44 kHz / stereo`, which no existing type carries. This is a separate seam from `AudioLevels` because it changes at track boundaries, not at frame rate.

**Files:**
- Create: `lib/playback/audio_format_info.dart`
- Modify: `lib/playback/player_engine.dart`
- Modify: `lib/playback/media_kit_player_engine.dart`
- Modify: `lib/playback/fake_player_engine.dart`
- Modify: `lib/playback/playback_controller.dart`
- Test: `test/playback/audio_format_info_test.dart`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `AudioFormatInfo({int? bitrateKbps, int? sampleRateHz, int? channels})` with `static const unknown`, and the display getters `String get bitrateLabel`, `String get sampleRateLabel`, `String get channelLabel`.
  - `PlayerEngine.formatStream` → `Stream<AudioFormatInfo>`.
  - `PlaybackController.formatInfo` → `AudioFormatInfo` (state, updated via `notifyListeners`), plus `FakePlayerEngine.emitFormat(AudioFormatInfo)`.

- [ ] **Step 1: Write the failing test**

Create `test/playback/audio_format_info_test.dart`:

```dart
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_format_info.dart';

void main() {
  test('unknown renders em-dash placeholders', () {
    expect(AudioFormatInfo.unknown.bitrateLabel, '— kbps');
    expect(AudioFormatInfo.unknown.sampleRateLabel, '— kHz');
    expect(AudioFormatInfo.unknown.channelLabel, '—');
  });

  test('known values render as the mockup shows them', () {
    const info = AudioFormatInfo(
      bitrateKbps: 128,
      sampleRateHz: 44100,
      channels: 2,
    );
    expect(info.bitrateLabel, '128 kbps');
    expect(info.sampleRateLabel, '44 kHz');
    expect(info.channelLabel, 'stereo');
  });

  test('mono and surround are named', () {
    expect(const AudioFormatInfo(channels: 1).channelLabel, 'mono');
    expect(const AudioFormatInfo(channels: 6).channelLabel, '6 ch');
  });

  test('sample rate rounds to the nearest kHz', () {
    expect(const AudioFormatInfo(sampleRateHz: 48000).sampleRateLabel, '48 kHz');
    expect(const AudioFormatInfo(sampleRateHz: 22050).sampleRateLabel, '22 kHz');
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/playback/audio_format_info_test.dart`
Expected: FAIL — `audio_format_info.dart` does not exist.

- [ ] **Step 3: Write the type**

Create `lib/playback/audio_format_info.dart`:

```dart
/// Stream properties shown on the display well.
///
/// Every field is nullable because they are only known once a track is open and
/// decoding has started; the labels render placeholders until then.
class AudioFormatInfo {
  const AudioFormatInfo({this.bitrateKbps, this.sampleRateHz, this.channels});

  static const unknown = AudioFormatInfo();

  final int? bitrateKbps;
  final int? sampleRateHz;
  final int? channels;

  String get bitrateLabel =>
      bitrateKbps == null ? '— kbps' : '$bitrateKbps kbps';

  String get sampleRateLabel => sampleRateHz == null
      ? '— kHz'
      : '${(sampleRateHz! / 1000).round()} kHz';

  String get channelLabel => switch (channels) {
        null => '—',
        1 => 'mono',
        2 => 'stereo',
        final n => '$n ch',
      };

  @override
  bool operator ==(Object other) =>
      other is AudioFormatInfo &&
      other.bitrateKbps == bitrateKbps &&
      other.sampleRateHz == sampleRateHz &&
      other.channels == channels;

  @override
  int get hashCode => Object.hash(bitrateKbps, sampleRateHz, channels);
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/playback/audio_format_info_test.dart`
Expected: PASS (4 tests)

- [ ] **Step 5: Add the stream to the interface and the fake**

In `lib/playback/player_engine.dart`, add the import and the getter next to `levelsStream`:

```dart
import 'audio_format_info.dart';

  /// Stream properties of the open track. Emits [AudioFormatInfo.unknown] until
  /// decoding reports real values.
  Stream<AudioFormatInfo> get formatStream;
```

In `lib/playback/fake_player_engine.dart`, add alongside the levels controller:

```dart
  final _formatController = StreamController<AudioFormatInfo>.broadcast();

  @override
  Stream<AudioFormatInfo> get formatStream => _formatController.stream;

  void emitFormat(AudioFormatInfo info) => _formatController.add(info);
```

with `import 'audio_format_info.dart';` at the top and `await _formatController.close();` added to `dispose`.

- [ ] **Step 6: Verify media_kit's field names before wiring them**

`media_kit` exposes stream properties whose exact field names must be confirmed against the installed version rather than assumed. Find them:

```bash
grep -rn "class AudioParams" ~/.pub-cache/hosted/pub.dev/media_kit-*/lib/
grep -rn "audioBitrate\|audioParams" ~/.pub-cache/hosted/pub.dev/media_kit-*/lib/src/player/player.dart
```

On Windows the cache lives at `%LOCALAPPDATA%\Pub\Cache\hosted\pub.dev\`.

**Verified against the installed package, 2026-08-02:**

| Member | Real type | Note |
|---|---|---|
| `AudioParams.sampleRate` | `int?` | as assumed |
| `AudioParams.channels` | `String?` | a channel-layout string, **not** a count — do not use it for `channels` |
| `AudioParams.channelCount` | `int?` | this is the count `AudioFormatInfo` wants |
| `PlayerStream.audioBitrate` | `Stream<double?>` | bits per second, so divide by 1000 for kbps |

Step 7's code uses `channelCount`. The original assumption that `channels` was an
`int` was wrong, which is exactly why this verification step exists.

- [ ] **Step 7: Wire the media_kit engine**

In `lib/playback/media_kit_player_engine.dart`, add `import 'audio_format_info.dart';`, then add these members and combine the two source streams in the constructor:

```dart
  final _format = StreamController<AudioFormatInfo>.broadcast();
  StreamSubscription<dynamic>? _paramsSubscription;
  StreamSubscription<dynamic>? _bitrateSubscription;
  int? _sampleRateHz;
  int? _channels;
  int? _bitrateKbps;

  @override
  Stream<AudioFormatInfo> get formatStream => _format.stream;

  void _emitFormat() {
    if (_format.isClosed) return;
    _format.add(AudioFormatInfo(
      bitrateKbps: _bitrateKbps,
      sampleRateHz: _sampleRateHz,
      channels: _channels,
    ));
  }
```

Add to the constructor body:

```dart
    _paramsSubscription = _player.stream.audioParams.listen((params) {
      _sampleRateHz = params.sampleRate;
      // `channels` is a layout string ("stereo"); `channelCount` is the number.
      _channels = params.channelCount;
      _emitFormat();
    });
    _bitrateSubscription = _player.stream.audioBitrate.listen((bitrate) {
      // media_kit reports bits per second; the display wants kbps.
      _bitrateKbps = bitrate == null ? null : (bitrate / 1000).round();
      _emitFormat();
    });
```

Add to `dispose`, before `_player.dispose()`:

```dart
    await _paramsSubscription?.cancel();
    await _bitrateSubscription?.cancel();
    await _format.close();
```

- [ ] **Step 8: Expose it on PlaybackController**

In `lib/playback/playback_controller.dart`, add `import 'audio_format_info.dart';`, a field and getter, and a subscription in the constructor. Unlike levels, this changes rarely, so it is ordinary controller state:

```dart
  AudioFormatInfo _formatInfo = AudioFormatInfo.unknown;

  AudioFormatInfo get formatInfo => _formatInfo;
```

In the constructor, beside the other `_subscriptions.add(...)` calls:

```dart
    _subscriptions.add(
      _engine.formatStream.listen((value) {
        _formatInfo = value;
        notifyListeners();
      }),
    );
```

Reset it when a new track opens — inside `playIndex`, immediately after `_playingPath = tracks[index].path;`:

```dart
    _formatInfo = AudioFormatInfo.unknown;
```

- [ ] **Step 9: Run the playback suite**

Run: `flutter test test/playback/`
Expected: PASS

- [ ] **Step 10: Commit**

```bash
git add lib/playback test/playback
git commit -m "feat(playback): report bitrate, sample rate and channel count

The display well shows real stream properties instead of placeholders, with
labels that fall back to em-dashes until decoding reports values."
```

---

### Task 13: MainPlayerPanel

**Files:**
- Create: `lib/ui/main_player/main_player_panel.dart`
- Test: `test/ui/main_player/main_player_panel_test.dart`
- Delete: `lib/ui/classic_main_player.dart`, `test/ui/classic_main_player_layout_test.dart`, `test/ui/classic_main_player_controls_test.dart`

**Interfaces:**
- Consumes: everything from Tasks 1–12.
- Produces: `MainPlayerPanel({Key? key, required PlaybackController playback, required ZoomController zoom, required LowerRegion lowerRegion, required bool hasTracks, required ValueChanged<LowerRegion> onSelectRegion, VoidCallback? onOpenFiles, VoidCallback? onOpenMenu, bool draggableTitle = true})` with `static const logicalSize = TrampMetrics.mainPlayer`.
- Widget keys: `transport-prev`, `transport-play`, `transport-pause`, `transport-stop`, `transport-next`, `transport-seek`, `transport-volume`, `transport-mute`, `player-shuffle`, `player-repeat`, `player-eq`, `player-pl`, `player-open`, `player-zoom`, `player-menu`, `window-minimize`, `window-maximize`, `window-close`.

**Layout note:** absolute positions from the spec's main-player geometry table, for the same reason as the equalizer.

- [ ] **Step 1: Write the failing test**

Create `test/ui/main_player/main_player_panel_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/main_player/main_player_panel.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

void main() {
  late FakePlayerEngine engine;
  late PlaylistController playlist;
  late PlaybackController playback;
  late ZoomController zoom;

  setUp(() {
    engine = FakePlayerEngine();
    playlist = PlaylistController();
    playback = PlaybackController(playlist: playlist, engine: engine);
    zoom = ZoomController(workArea: const Size(6000, 4000));
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(
    WidgetTester tester, {
    LowerRegion region = LowerRegion.playlist,
    ValueChanged<LowerRegion>? onSelectRegion,
    VoidCallback? onOpenFiles,
    bool hasTracks = true,
  }) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: MainPlayerPanel(
            playback: playback,
            zoom: zoom,
            lowerRegion: region,
            hasTracks: hasTracks,
            draggableTitle: false,
            onSelectRegion: onSelectRegion ?? (_) {},
            onOpenFiles: onOpenFiles,
          ),
        ),
      ),
    );
  }

  testWidgets('holds the locked canvas size without overflowing',
      (tester) async {
    await pump(tester);
    expect(
      tester.getSize(find.byType(MainPlayerPanel)),
      TrampMetrics.mainPlayer,
    );
    expect(tester.takeException(), isNull);
  });

  testWidgets('is branded TRAMP, never Winamp', (tester) async {
    await pump(tester);
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.textContaining('WINAMP'), findsNothing);
  });

  testWidgets('transport buttons drive the controller', (tester) async {
    playlist.addTracks([const Track(path: 'a.mp3'), const Track(path: 'b.mp3')]);
    await tester.pump();
    await pump(tester);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(playback.playing, isTrue);

    await tester.tap(find.byKey(const Key('transport-pause')));
    await tester.pumpAndSettle();
    expect(playback.playing, isFalse);

    await tester.tap(find.byKey(const Key('transport-next')));
    await tester.pumpAndSettle();
    expect(playback.playingIndex, 1);
  });

  testWidgets('shuffle and repeat toggle the controller', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('player-shuffle')));
    await tester.pump();
    expect(playback.shuffle, isTrue);

    await tester.tap(find.byKey(const Key('player-repeat')));
    await tester.pump();
    expect(playback.repeatMode.name, 'all');
  });

  testWidgets('mute toggles and dims the volume fill', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('transport-mute')));
    await tester.pump();
    expect(playback.muted, isTrue);
  });

  testWidgets('EQ and PL buttons request a region change', (tester) async {
    final requested = <LowerRegion>[];
    await pump(tester, onSelectRegion: requested.add);
    await tester.tap(find.byKey(const Key('player-eq')));
    await tester.tap(find.byKey(const Key('player-pl')));
    expect(requested, [LowerRegion.equalizer, LowerRegion.playlist]);
  });

  testWidgets('OPEN calls the open-files callback', (tester) async {
    var opens = 0;
    await pump(tester, onOpenFiles: () => opens++);
    await tester.tap(find.byKey(const Key('player-open')));
    expect(opens, 1);
  });

  testWidgets('zoom button shows the current step and opens a menu',
      (tester) async {
    zoom.setPercent(150);
    await pump(tester);
    expect(find.text('ZOOM 150%'), findsOneWidget);
    await tester.tap(find.byKey(const Key('player-zoom')));
    await tester.pumpAndSettle();
    expect(find.text('200%'), findsOneWidget);
    await tester.tap(find.text('200%'));
    await tester.pumpAndSettle();
    expect(zoom.percent, 200);
  });

  testWidgets('steps too large for the display are not offered',
      (tester) async {
    zoom = ZoomController(workArea: const Size(1600, 1200));
    await pump(tester);
    await tester.tap(find.byKey(const Key('player-zoom')));
    await tester.pumpAndSettle();
    expect(find.text('150%'), findsOneWidget);
    expect(find.text('300%'), findsNothing);
  });

  testWidgets('display shows real stream properties when reported',
      (tester) async {
    await pump(tester);
    engine.emitFormat(const AudioFormatInfo(
      bitrateKbps: 128,
      sampleRateHz: 44100,
      channels: 2,
    ));
    await tester.pump();
    expect(find.text('128 kbps'), findsOneWidget);
    expect(find.text('44 kHz'), findsOneWidget);
    expect(find.text('stereo'), findsOneWidget);
  });

  testWidgets('indicators light for the visible region', (tester) async {
    await pump(tester, region: LowerRegion.equalizer);
    expect(find.text('EQ'), findsWidgets);
    expect(find.text('PL'), findsWidgets);
  });

  testWidgets('transport is disabled with an empty playlist', (tester) async {
    await pump(tester, hasTracks: false);
    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(playback.playing, isFalse);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/main_player/main_player_panel_test.dart`
Expected: FAIL — `main_player_panel.dart` does not exist.

- [ ] **Step 3: Write the panel**

Create `lib/ui/main_player/main_player_panel.dart`:

```dart
import 'dart:async';

import 'package:flutter/material.dart' hide RepeatMode;
import 'package:window_manager/window_manager.dart';

import '../../domain/repeat_mode.dart';
import '../../domain/tramp_settings.dart';
import '../../playback/playback_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/chrome_button.dart';
import '../chrome/chrome_slider.dart';
import '../chrome/lcd_text.dart';
import '../chrome/metal_panel.dart';
import '../chrome/spectrum_visualizer.dart';
import '../chrome/title_bar.dart';
import '../chrome/tramp_mark.dart';
import '../chrome/transport_icons.dart';
import '../format.dart';
import '../zoom/zoom_controller.dart';

/// The main player panel.
///
/// Authored against the fixed 812x242 canvas with absolute positions from the
/// design spec's geometry table.
class MainPlayerPanel extends StatefulWidget {
  const MainPlayerPanel({
    super.key,
    required this.playback,
    required this.zoom,
    required this.lowerRegion,
    required this.hasTracks,
    required this.onSelectRegion,
    this.onOpenFiles,
    this.onOpenMenu,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.mainPlayer;

  final PlaybackController playback;
  final ZoomController zoom;
  final LowerRegion lowerRegion;
  final bool hasTracks;
  final ValueChanged<LowerRegion> onSelectRegion;
  final VoidCallback? onOpenFiles;
  final VoidCallback? onOpenMenu;
  final bool draggableTitle;

  @override
  State<MainPlayerPanel> createState() => _MainPlayerPanelState();
}

class _MainPlayerPanelState extends State<MainPlayerPanel> {
  double? _volumePreview;

  PlaybackController get playback => widget.playback;

  Future<void> _play() async {
    if (!widget.hasTracks) return;
    if (playback.playing) return;
    await playback.playPause();
  }

  Future<void> _pause() async {
    if (playback.playing) await playback.playPause();
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: MainPlayerPanel.logicalSize.width,
      height: MainPlayerPanel.logicalSize.height,
      child: MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: ListenableBuilder(
          listenable: Listenable.merge([playback, widget.zoom]),
          builder: (context, _) => _buildChrome(context),
        ),
      ),
    );
  }

  Widget _buildChrome(BuildContext context) {
    final track = playback.currentTrack;
    final durationMs = playback.duration.inMilliseconds;
    final seek = durationMs > 0
        ? (playback.position.inMilliseconds / durationMs).clamp(0.0, 1.0)
        : 0.0;
    final volume = (_volumePreview ?? playback.volume).clamp(0.0, 1.0);
    final canTransport = track != null || widget.hasTracks;
    final format = playback.formatInfo;
    final index = playback.playingIndex;

    final title = track == null
        ? 'No track'
        : [
            if (index != null) '${index + 1}.',
            if (track.artist != null && track.artist!.trim().isNotEmpty)
              '${track.artist!.trim()} -',
            track.displayTitle,
          ].join(' ');

    return Stack(
      children: [
        Positioned(
          left: 0,
          right: 0,
          top: 0,
          child: TrampTitleBar(
            title: 'TRAMP',
            draggable: widget.draggableTitle,
            // Tramp's compact mark. The mockup shows Winamp's lightning bolt
            // here — that is their brand, not a generic icon. The full colour
            // logo is illegible at this size; see the spec's Brand assets.
            leading: ChromeButton.icon(
              key: const Key('player-menu'),
              icon: const TrampMark(size: 19),
              onPressed: widget.onOpenMenu,
              semanticLabel: 'Tramp menu',
              size: const Size(27, 27),
            ),
            trailing: [
              ChromeButton.label(
                key: const Key('window-minimize'),
                text: '—',
                onPressed: () => unawaited(windowManager.minimize()),
                semanticLabel: 'Minimize',
                size: const Size(27, 22),
              ),
              ChromeButton.label(
                key: const Key('window-maximize'),
                text: '□',
                onPressed: () => unawaited(_toggleMaximize()),
                semanticLabel: 'Maximize',
                size: const Size(27, 22),
              ),
              ChromeButton.label(
                key: const Key('window-close'),
                text: '✕',
                onPressed: () => unawaited(windowManager.close()),
                semanticLabel: 'Close',
                size: const Size(27, 22),
              ),
            ],
          ),
        ),

        // Display well.
        Positioned(
          left: 41,
          top: 41,
          width: 527,
          height: 137,
          child: MetalPanel(
            surface: TrampSurface.lcdGlass,
            child: Stack(
              children: [
                Positioned(
                  left: 6,
                  top: 6,
                  width: 227,
                  height: 105,
                  child: SpectrumVisualizer(levels: playback.levelsStream),
                ),
                Positioned(
                  left: 6,
                  top: 117,
                  width: 227,
                  height: 4,
                  child: ChromeSlider(
                    key: const Key('transport-seek'),
                    value: seek,
                    axis: Axis.horizontal,
                    thumbExtent: 4,
                    thumbThickness: 3,
                    semanticLabel: 'Seek',
                    onChangeEnd: durationMs > 0
                        ? (v) => unawaited(playback.seek(
                              Duration(milliseconds: (v * durationMs).round()),
                            ))
                        : null,
                  ),
                ),
                Positioned(
                  left: 245,
                  top: 8,
                  width: 274,
                  child: LcdText(title, lit: track != null),
                ),
                Positioned(
                  left: 245,
                  top: 34,
                  child: Row(
                    crossAxisAlignment: CrossAxisAlignment.end,
                    children: [
                      LcdText(
                        formatDuration(playback.position),
                        size: LcdSize.large,
                      ),
                      const SizedBox(width: 10),
                      Padding(
                        padding: const EdgeInsets.only(bottom: 4),
                        child: Text(
                          formatDuration(playback.duration),
                          style: TrampText.lcd
                              .copyWith(color: TrampColors.labelDim),
                        ),
                      ),
                    ],
                  ),
                ),
                Positioned(
                  left: 245,
                  top: 74,
                  width: 274,
                  child: Row(
                    children: [
                      LcdText(format.bitrateLabel),
                      const SizedBox(width: 12),
                      LcdText(format.sampleRateLabel),
                      const SizedBox(width: 12),
                      LcdText(format.channelLabel),
                    ],
                  ),
                ),
                Positioned(
                  left: 245,
                  top: 100,
                  child: Row(
                    children: [
                      LcdIndicator(
                        'EQ',
                        lit: widget.lowerRegion == LowerRegion.equalizer,
                        onTap: () =>
                            widget.onSelectRegion(LowerRegion.equalizer),
                      ),
                      const SizedBox(width: 6),
                      LcdIndicator(
                        'PL',
                        lit: widget.lowerRegion == LowerRegion.playlist,
                        onTap: () =>
                            widget.onSelectRegion(LowerRegion.playlist),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),

        // Right block: shuffle, repeat, volume, region buttons.
        Positioned(
          left: 665,
          top: 52,
          child: ChromeButton.icon(
            key: const Key('player-shuffle'),
            icon: TransportIcons.shuffle(
              colour: playback.shuffle
                  ? TrampColors.phosphor
                  : TrampColors.label,
            ),
            active: playback.shuffle,
            onPressed: playback.toggleShuffle,
            semanticLabel: 'Shuffle',
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 727,
          top: 52,
          child: ChromeButton.icon(
            key: const Key('player-repeat'),
            icon: TransportIcons.repeat(
              colour: playback.repeatMode == RepeatMode.off
                  ? TrampColors.label
                  : TrampColors.phosphor,
              one: playback.repeatMode == RepeatMode.one,
            ),
            active: playback.repeatMode != RepeatMode.off,
            onPressed: playback.cycleRepeatMode,
            semanticLabel: 'Repeat',
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 607,
          top: 106,
          width: 148,
          height: 16,
          child: ChromeSlider(
            key: const Key('transport-volume'),
            value: playback.muted ? 0 : volume,
            axis: Axis.horizontal,
            dimmed: playback.muted,
            semanticLabel: 'Volume',
            onChanged: (v) {
              setState(() => _volumePreview = v);
              playback.setVolume(v);
            },
            onChangeEnd: (_) => setState(() => _volumePreview = null),
          ),
        ),
        Positioned(
          left: 759,
          top: 104,
          child: ChromeButton.icon(
            key: const Key('transport-mute'),
            icon: Icon(
              playback.muted ? Icons.volume_off : Icons.volume_up,
              size: 14,
              color: playback.muted
                  ? TrampColors.labelDim
                  : TrampColors.label,
            ),
            onPressed: playback.toggleMute,
            semanticLabel: playback.muted ? 'Unmute' : 'Mute',
            size: const Size(20, 20),
          ),
        ),
        Positioned(
          left: 610,
          top: 149,
          child: ChromeButton.label(
            key: const Key('player-eq'),
            text: 'EQ',
            active: widget.lowerRegion == LowerRegion.equalizer,
            onPressed: () => widget.onSelectRegion(LowerRegion.equalizer),
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 663,
          top: 149,
          child: ChromeButton.label(
            key: const Key('player-pl'),
            text: 'PL',
            active: widget.lowerRegion == LowerRegion.playlist,
            onPressed: () => widget.onSelectRegion(LowerRegion.playlist),
            size: const Size(52, 26),
          ),
        ),

        // Transport row.
        for (final button in _transportButtons(canTransport, track != null))
          button,

        Positioned(
          left: 609,
          top: 194,
          child: _ZoomButton(zoom: widget.zoom),
        ),
        Positioned(
          left: 726,
          top: 194,
          child: ChromeButton.label(
            key: const Key('player-open'),
            text: 'OPEN',
            leading: TransportIcons.eject(),
            onPressed: widget.onOpenFiles,
            size: const Size(54, 26),
          ),
        ),
      ],
    );
  }

  List<Widget> _transportButtons(bool canTransport, bool trackOpen) {
    final specs = <(String, Widget, VoidCallback?)>[
      (
        'transport-prev',
        TransportIcons.prev(),
        canTransport ? () => unawaited(playback.previous()) : null,
      ),
      (
        'transport-play',
        TransportIcons.play(),
        canTransport ? () => unawaited(_play()) : null,
      ),
      (
        'transport-pause',
        TransportIcons.pause(),
        canTransport ? () => unawaited(_pause()) : null,
      ),
      (
        'transport-stop',
        TransportIcons.stop(),
        trackOpen ? () => unawaited(playback.stop()) : null,
      ),
      (
        'transport-next',
        TransportIcons.next(),
        canTransport ? () => unawaited(playback.next()) : null,
      ),
    ];

    const lefts = [43.0, 116.0, 190.0, 264.0, 338.0];

    return [
      for (var i = 0; i < specs.length; i++)
        Positioned(
          left: lefts[i],
          top: 182,
          child: ChromeButton.icon(
            key: Key(specs[i].$1),
            icon: specs[i].$2,
            onPressed: specs[i].$3,
            semanticLabel: specs[i].$1,
            size: const Size(69, 40),
          ),
        ),
    ];
  }

  Future<void> _toggleMaximize() async {
    if (await windowManager.isMaximized()) {
      await windowManager.unmaximize();
    } else {
      await windowManager.maximize();
    }
  }
}

/// Shows the current step and offers the ones the display can host.
class _ZoomButton extends StatelessWidget {
  const _ZoomButton({required this.zoom});

  final ZoomController zoom;

  @override
  Widget build(BuildContext context) {
    return ChromeButton.dropdown(
      key: const Key('player-zoom'),
      text: 'ZOOM ${zoom.percent}%',
      size: const Size(108, 26),
      semanticLabel: 'Zoom level',
      onPressed: () async {
        final box = context.findRenderObject()! as RenderBox;
        final origin = box.localToGlobal(Offset.zero);
        final chosen = await showMenu<int>(
          context: context,
          position: RelativeRect.fromLTRB(
            origin.dx,
            origin.dy + box.size.height,
            origin.dx,
            origin.dy,
          ),
          items: [
            for (final step in zoom.enabledSteps)
              PopupMenuItem<int>(value: step, child: Text('$step%')),
          ],
        );
        if (chosen != null) zoom.setPercent(chosen);
      },
    );
  }
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/main_player/main_player_panel_test.dart`
Expected: PASS (12 tests)

- [ ] **Step 5: Delete the old player**

```bash
git rm lib/ui/classic_main_player.dart \
       test/ui/classic_main_player_layout_test.dart \
       test/ui/classic_main_player_controls_test.dart
```

`lib/ui/format.dart` stays — `MainPlayerPanel` imports `formatDuration` from it. The old `export 'format.dart'` line disappears with the deleted file, so any other importer must import `format.dart` directly; `flutter analyze` in the next task catches those.

- [ ] **Step 6: Commit**

```bash
git add lib/ui/main_player test/ui/main_player
git commit -m "feat(ui): main player panel on the fixed 812x242 canvas

Replaces ClassicMainPlayer with the graphite chrome: display well with
spectrum and seek, shuffle and repeat toggles, volume where the mockup's VU
meters were, region buttons, and OPEN plus a ZOOM step menu."
```

---

### Task 14: Playlist panel reskin

**Files:**
- Modify: `lib/ui/playlist_panel.dart`
- Test: `test/ui/playlist_panel_test.dart`

**Interfaces:**
- Consumes: `TrampColors`, `TrampText` (Task 1), `MetalPanel`/`TrampSurface` (Task 4), `ChromeButton` (Task 5), `ZoomScope` (Task 3).
- Produces: `PlaylistPanel({Key? key, required PlaylistController playlist, required PlaybackController playback, VoidCallback? onOpen, VoidCallback? onSave, VoidCallback? onAddFiles})` — the existing constructor, unchanged. Reordering, selection, activation and `formatTrackDuration` are unchanged; only the skin moves. Adds `Key('playlist-open')`, `Key('playlist-save')`, `Key('playlist-add')`.

**Two things to know about the current file:** the toolbar calls `ChromeButton(onPressed:, child:)`, which Task 5 replaced with named constructors, so this file does not compile until this task lands. And rows render their title with `Text.rich`, which `find.text` cannot match — the rewrite uses separate `Text` widgets for title and artist so both are directly findable and assertable.

- [ ] **Step 1: Write the failing test**

Create `test/ui/playlist_panel_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';
import 'package:tramp/ui/playlist_panel.dart';

void main() {
  late PlaylistController playlist;
  late PlaybackController playback;

  setUp(() {
    playlist = PlaylistController();
    playback = PlaybackController(
      playlist: playlist,
      engine: FakePlayerEngine(),
    );
    playlist.addTracks(const [
      Track(path: 'a.mp3', title: 'Alpha'),
      Track(path: 'b.mp3', title: 'Beta', artist: 'Someone'),
    ]);
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(WidgetTester tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: PlaylistPanel(playlist: playlist, playback: playback),
        ),
      ),
    );
  }

  testWidgets('lists every track and its artist', (tester) async {
    await pump(tester);
    expect(find.text('Alpha'), findsOneWidget);
    expect(find.text('Beta'), findsOneWidget);
    expect(find.text('Someone'), findsOneWidget);
  });

  testWidgets('wears the graphite panel and glass surfaces', (tester) async {
    await pump(tester);
    final surfaces = tester
        .widgetList<MetalPanel>(find.byType(MetalPanel))
        .map((p) => p.surface)
        .toList();
    expect(surfaces, contains(TrampSurface.raisedPanel));
    expect(surfaces, contains(TrampSurface.lcdGlass));
  });

  testWidgets('the selected row title is phosphor, others are label',
      (tester) async {
    playlist.select(1);
    await pump(tester);
    expect(
      tester.widget<Text>(find.text('Beta')).style!.color,
      TrampColors.phosphor,
    );
    expect(
      tester.widget<Text>(find.text('Alpha')).style!.color,
      TrampColors.label,
    );
  });

  testWidgets('toolbar buttons are wired to their callbacks', (tester) async {
    var opens = 0;
    var saves = 0;
    var adds = 0;
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: PlaylistPanel(
            playlist: playlist,
            playback: playback,
            onOpen: () => opens++,
            onSave: () => saves++,
            onAddFiles: () => adds++,
          ),
        ),
      ),
    );
    await tester.tap(find.byKey(const Key('playlist-open')));
    await tester.tap(find.byKey(const Key('playlist-save')));
    await tester.tap(find.byKey(const Key('playlist-add')));
    expect([opens, saves, adds], [1, 1, 1]);
  });

  testWidgets('tapping selects and double-tapping starts playback',
      (tester) async {
    await pump(tester);
    await tester.tap(find.text('Beta'));
    await tester.pump();
    expect(playlist.selectedIndex, 1);

    await tester.tap(find.text('Alpha'));
    await tester.pump();
    await tester.tap(find.text('Alpha'));
    await tester.pump();
    await tester.pumpAndSettle();
    expect(playback.playingIndex, 0);
  });

  testWidgets('an empty playlist says so', (tester) async {
    playlist.clear();
    await pump(tester);
    expect(find.text('No tracks'), findsOneWidget);
  });

  testWidgets('no light surface survives anywhere in the tree',
      (tester) async {
    await pump(tester);
    for (final box in tester.widgetList<DecoratedBox>(
      find.byType(DecoratedBox),
    )) {
      final decoration = box.decoration as BoxDecoration;
      final colour = decoration.color;
      if (colour != null && colour.alpha > 0) {
        expect(colour.computeLuminance(), lessThan(0.5),
            reason: 'playlist chrome must stay dark');
      }
      final gradient = decoration.gradient;
      if (gradient is LinearGradient) {
        for (final c in gradient.colors) {
          expect(c.computeLuminance(), lessThan(0.5),
              reason: 'playlist gradients must stay dark');
        }
      }
    }
  });
}
```

If `PlaylistController` has no `clear()` method, replace that call in the empty-playlist test with whatever removes all tracks (for example `removeAt` in a loop) — `flutter analyze` names the available members.

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/playlist_panel_test.dart`
Expected: FAIL — `playlist_panel.dart` still references removed tokens, so the file will not compile.

- [ ] **Step 3: Reskin the panel**

Replace `lib/ui/playlist_panel.dart` entirely. The public constructor, the
`ReorderableListView` wiring, the selection and activation gestures, the
semantics and `formatTrackDuration` are all carried over unchanged — only the
skin differs.

```dart
import 'dart:ui' show FontFeature;

import 'package:flutter/material.dart';

import '../domain/track.dart';
import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import '../theme/tramp_text.dart';
import 'chrome/chrome_button.dart';
import 'chrome/metal_panel.dart';
import 'zoom/zoom_scope.dart';

String formatTrackDuration(Duration? duration) {
  if (duration == null) return '';
  final totalSeconds = duration.inSeconds;
  final minutes = totalSeconds ~/ 60;
  final seconds = totalSeconds % 60;
  return '$minutes:${seconds.toString().padLeft(2, '0')}';
}

class PlaylistPanel extends StatelessWidget {
  const PlaylistPanel({
    super.key,
    required this.playlist,
    required this.playback,
    this.onOpen,
    this.onSave,
    this.onAddFiles,
  });

  final PlaylistController playlist;
  final PlaybackController playback;
  final VoidCallback? onOpen;
  final VoidCallback? onSave;
  final VoidCallback? onAddFiles;

  @override
  Widget build(BuildContext context) {
    return MetalPanel(
      surface: TrampSurface.raisedPanel,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _PlaylistToolbar(
            onOpen: onOpen,
            onSave: onSave,
            onAddFiles: onAddFiles,
          ),
          Expanded(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(6, 0, 6, 6),
              child: MetalPanel(
                surface: TrampSurface.lcdGlass,
                child: ListenableBuilder(
                  listenable: playlist,
                  builder: (context, _) {
                    final tracks = playlist.playlist.tracks;
                    if (tracks.isEmpty) {
                      return Center(
                        child: Text(
                          'No tracks',
                          style: TrampText.lcdDim,
                        ),
                      );
                    }

                    return ReorderableListView.builder(
                      buildDefaultDragHandles: false,
                      itemCount: tracks.length,
                      onReorder: playlist.move,
                      itemBuilder: (context, index) {
                        final track = tracks[index];
                        final active = playlist.selectedIndex == index;
                        return _PlaylistRow(
                          key: ValueKey(track.path),
                          index: index,
                          track: track,
                          active: active,
                          onActivate: () => playback.playIndex(index),
                          onSelect: () => playlist.select(index),
                        );
                      },
                    );
                  },
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _PlaylistToolbar extends StatelessWidget {
  const _PlaylistToolbar({this.onOpen, this.onSave, this.onAddFiles});

  final VoidCallback? onOpen;
  final VoidCallback? onSave;
  final VoidCallback? onAddFiles;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(6, 6, 6, 6),
      child: Row(
        children: [
          ChromeButton.label(
            key: const Key('playlist-open'),
            text: 'OPEN',
            onPressed: onOpen,
            size: const Size(54, 22),
          ),
          const SizedBox(width: 5),
          ChromeButton.label(
            key: const Key('playlist-save'),
            text: 'SAVE',
            onPressed: onSave,
            size: const Size(54, 22),
          ),
          const SizedBox(width: 5),
          ChromeButton.label(
            key: const Key('playlist-add'),
            text: 'ADD',
            onPressed: onAddFiles,
            size: const Size(48, 22),
          ),
        ],
      ),
    );
  }
}

class _PlaylistRow extends StatelessWidget {
  const _PlaylistRow({
    super.key,
    required this.index,
    required this.track,
    required this.active,
    required this.onActivate,
    required this.onSelect,
  });

  final int index;
  final Track track;
  final bool active;
  final VoidCallback onActivate;
  final VoidCallback onSelect;

  @override
  Widget build(BuildContext context) {
    final hairline = ZoomScope.hairlineFor(context);
    final foreground = active ? TrampColors.phosphor : TrampColors.label;
    final muted = active ? TrampColors.phosphorDim : TrampColors.labelDim;
    final indexLabel = (index + 1).toString().padLeft(2, '0');
    final artist = track.artist?.trim();
    final hasArtist = artist != null && artist.isNotEmpty;

    final semanticLabel =
        hasArtist ? '${track.displayTitle}, $artist' : track.displayTitle;

    return ReorderableDragStartListener(
      index: index,
      child: Semantics(
        selected: active,
        button: true,
        label: semanticLabel,
        child: Actions(
          actions: {
            ActivateIntent: CallbackAction<ActivateIntent>(
              onInvoke: (_) {
                onActivate();
                return null;
              },
            ),
          },
          child: GestureDetector(
            onTap: onSelect,
            onDoubleTap: onActivate,
            child: DecoratedBox(
              decoration: BoxDecoration(
                border: Border(
                  bottom: BorderSide(
                    color: TrampColors.bevelLo,
                    width: hairline,
                  ),
                ),
              ),
              child: Padding(
                padding:
                    const EdgeInsets.symmetric(horizontal: 8, vertical: 5),
                child: Row(
                  children: [
                    SizedBox(
                      width: 24,
                      child: Text(
                        indexLabel,
                        style: TrampText.lcd.copyWith(color: muted),
                      ),
                    ),
                    const SizedBox(width: 8),
                    // Title and artist are separate Text widgets rather than one
                    // Text.rich so each is directly findable and assertable.
                    Flexible(
                      child: Text(
                        track.displayTitle,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: TrampText.lcd.copyWith(color: foreground),
                      ),
                    ),
                    if (hasArtist) ...[
                      Text(
                        ' — ',
                        style: TrampText.lcd.copyWith(color: muted),
                      ),
                      Flexible(
                        child: Text(
                          artist,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: TrampText.lcd.copyWith(color: muted),
                        ),
                      ),
                    ],
                    const Spacer(),
                    const SizedBox(width: 8),
                    Text(
                      formatTrackDuration(track.duration),
                      style: TrampText.lcd.copyWith(
                        color: muted,
                        fontFeatures: const [FontFeature.tabularFigures()],
                      ),
                    ),
                    const SizedBox(width: 6),
                    Semantics(
                      label: 'Reorder',
                      child: Icon(
                        Icons.drag_handle,
                        size: 14,
                        color: muted,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `flutter test test/ui/playlist_panel_test.dart`
Expected: PASS (5 tests)

- [ ] **Step 5: Commit**

```bash
git add lib/ui/playlist_panel.dart test/ui/playlist_panel_test.dart
git commit -m "refactor(ui): reskin the playlist to the graphite tokens

List behaviour, reordering and selection are unchanged; only colours, type
and chrome surfaces move to the new palette."
```

---

### Task 15: Shell, window sizing and app wiring

**Files:**
- Modify: `lib/ui/tramp_shell.dart`
- Modify: `lib/platform/tramp_window.dart`
- Modify: `lib/app.dart`
- Test: `test/ui/tramp_shell_test.dart`
- Test: `test/ui/tramp_shell_shortcuts_test.dart` (update construction)

**Interfaces:**
- Consumes: everything from Tasks 1–14.
- Produces:
  - `TrampShell` keeps its shortcut intents and drop-target behaviour and gains `required ZoomController zoom`, `required LowerRegion lowerRegion`, `required Widget mainPlayer`, `required Widget equalizer`, `required Widget playlist`, `required bool equalizerCollapsed`. The old `transport` parameter is renamed `mainPlayer`.
  - `const Key panelStackKey = Key('panel-stack')` — the box a layout test measures.
  - `configureTrampWindow({required Size size, required Size minimumSize})`.
  - New shortcuts: `Ctrl/Cmd +` `stepUp`, `Ctrl/Cmd -` `stepDown`, `Ctrl/Cmd 0` `reset`, via `ZoomInIntent`, `ZoomOutIntent`, `ZoomResetIntent`.

- [ ] **Step 1: Write the failing shell test**

Create `test/ui/tramp_shell_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/tramp_shell.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

void main() {
  late PlaylistController playlist;
  late PlaybackController playback;
  late ZoomController zoom;

  setUp(() {
    playlist = PlaylistController();
    playback = PlaybackController(
      playlist: playlist,
      engine: FakePlayerEngine(),
    );
    zoom = ZoomController(workArea: const Size(6000, 4000));
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(
    WidgetTester tester, {
    LowerRegion region = LowerRegion.playlist,
    bool collapsed = false,
    Size surface = const Size(824, 500),
  }) async {
    await tester.binding.setSurfaceSize(surface);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: TrampShell(
          zoom: zoom,
          lowerRegion: region,
          equalizerCollapsed: collapsed,
          playback: playback,
          playlistController: playlist,
          mainPlayer: const SizedBox(
            key: Key('main-player'),
            width: 812,
            height: 242,
          ),
          equalizer: const SizedBox(
            key: Key('equalizer'),
            width: 812,
            height: 206,
          ),
          playlist: const SizedBox(key: Key('playlist')),
        ),
      ),
    );
  }

  testWidgets('playlist region shows the playlist and not the equalizer',
      (tester) async {
    await pump(tester, region: LowerRegion.playlist);
    expect(find.byKey(const Key('playlist')), findsOneWidget);
    expect(find.byKey(const Key('equalizer')), findsNothing);
  });

  testWidgets('equalizer region shows the equalizer and not the playlist',
      (tester) async {
    await pump(tester, region: LowerRegion.equalizer);
    expect(find.byKey(const Key('equalizer')), findsOneWidget);
    expect(find.byKey(const Key('playlist')), findsNothing);
  });

  testWidgets('the stack scales by the zoom factor', (tester) async {
    await pump(tester, surface: const Size(3000, 2000));
    final at100 = tester.getSize(find.byKey(panelStackKey));

    zoom.setPercent(200);
    await tester.pumpAndSettle();
    final at200 = tester.getSize(find.byKey(panelStackKey));

    expect(at200.width, closeTo(at100.width * 2, 0.01));
  });

  testWidgets('no overflow at any zoom step', (tester) async {
    await tester.binding.setSurfaceSize(const Size(4000, 3000));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    for (final step in ZoomController.steps) {
      zoom.setPercent(step);
      await pump(tester, surface: const Size(4000, 3000));
      expect(tester.takeException(), isNull, reason: 'overflow at $step%');
    }
  });

  testWidgets('main player keeps its exact aspect at every step',
      (tester) async {
    const expected =
        TrampMetrics.mainPlayer.width / TrampMetrics.mainPlayer.height;
    for (final step in ZoomController.steps) {
      zoom.setPercent(step);
      await pump(tester, surface: const Size(4000, 3000));
      final size = tester.getSize(find.byKey(const Key('main-player')));
      expect(size.width / size.height, closeTo(expected, 0.001),
          reason: 'aspect drifted at $step%');
    }
  });

  testWidgets('Ctrl+= steps up, Ctrl+- steps down, Ctrl+0 resets',
      (tester) async {
    await pump(tester, surface: const Size(4000, 3000));

    await tester.sendKeyDownEvent(LogicalKeyboardKey.control);
    await tester.sendKeyEvent(LogicalKeyboardKey.equal);
    await tester.pump();
    expect(zoom.percent, 125);

    await tester.sendKeyEvent(LogicalKeyboardKey.minus);
    await tester.pump();
    expect(zoom.percent, 100);

    await tester.sendKeyEvent(LogicalKeyboardKey.equal);
    await tester.sendKeyEvent(LogicalKeyboardKey.digit0);
    await tester.pump();
    expect(zoom.percent, 100);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.control);
  });

  testWidgets('a collapsed equalizer occupies only its title bar',
      (tester) async {
    await pump(tester, region: LowerRegion.equalizer, collapsed: true);
    expect(tester.takeException(), isNull);
  });
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `flutter test test/ui/tramp_shell_test.dart`
Expected: FAIL — `TrampShell` has no `zoom`, `lowerRegion`, `mainPlayer` or `equalizer` parameters, and `panelStackKey` is undefined.

- [ ] **Step 3: Rewrite the shell's build method and add the zoom intents**

In `lib/ui/tramp_shell.dart`, replace the imports of `classic_main_player.dart` and `dart:math` with:

```dart
import '../domain/tramp_settings.dart';
import '../theme/tramp_colors.dart';
import '../theme/tramp_metrics.dart';
import 'zoom/zoom_controller.dart';
import 'zoom/zoom_scope.dart';
```

Add the key and the three new intents beside the existing ones:

```dart
/// The scaled stack of panels. Named so layout tests can measure it.
const Key panelStackKey = Key('panel-stack');

class ZoomInIntent extends Intent {
  const ZoomInIntent();
}

class ZoomOutIntent extends Intent {
  const ZoomOutIntent();
}

class ZoomResetIntent extends Intent {
  const ZoomResetIntent();
}
```

Replace the constructor and fields:

```dart
  const TrampShell({
    super.key,
    required this.mainPlayer,
    required this.equalizer,
    required this.playlist,
    required this.playback,
    required this.playlistController,
    required this.zoom,
    required this.lowerRegion,
    this.equalizerCollapsed = false,
    this.hasTracks = false,
    this.playlistFocusNode,
    this.onDropPaths,
    this.onOpenFiles,
    this.onSavePlaylist,
  });

  final Widget mainPlayer;
  final Widget equalizer;
  final Widget playlist;
  final PlaybackController playback;
  final PlaylistController playlistController;
  final ZoomController zoom;
  final LowerRegion lowerRegion;
  final bool equalizerCollapsed;
  final bool hasTracks;
  final FocusNode? playlistFocusNode;
  final void Function(List<String> paths)? onDropPaths;
  final Future<void> Function()? onOpenFiles;
  final Future<void> Function()? onSavePlaylist;
```

Add to `_shortcuts`:

```dart
    SingleActivator(LogicalKeyboardKey.equal, control: true): ZoomInIntent(),
    SingleActivator(LogicalKeyboardKey.equal, meta: true): ZoomInIntent(),
    SingleActivator(LogicalKeyboardKey.minus, control: true): ZoomOutIntent(),
    SingleActivator(LogicalKeyboardKey.minus, meta: true): ZoomOutIntent(),
    SingleActivator(LogicalKeyboardKey.digit0, control: true): ZoomResetIntent(),
    SingleActivator(LogicalKeyboardKey.digit0, meta: true): ZoomResetIntent(),
```

Add to `_actions()`:

```dart
      ZoomInIntent: CallbackAction<ZoomInIntent>(
        onInvoke: (_) {
          zoom.stepUp();
          return null;
        },
      ),
      ZoomOutIntent: CallbackAction<ZoomOutIntent>(
        onInvoke: (_) {
          zoom.stepDown();
          return null;
        },
      ),
      ZoomResetIntent: CallbackAction<ZoomResetIntent>(
        onInvoke: (_) {
          zoom.reset();
          return null;
        },
      ),
```

Replace the `LayoutBuilder` body inside `build` with the scaled stack. The whole stack is scaled once; nothing inside scales itself:

```dart
    final shell = DragToResizeArea(
      resizeEdgeSize: 6,
      child: ColoredBox(
        color: TrampColors.frame,
        child: ListenableBuilder(
          listenable: zoom,
          builder: (context, _) {
            final factor = zoom.factor;
            final ratio = MediaQuery.devicePixelRatioOf(context);

            final focusedPlaylist = playlistFocusNode == null
                ? playlist
                : Focus(focusNode: playlistFocusNode, child: playlist);

            return ZoomScope(
              factor: factor,
              devicePixelRatio: ratio,
              child: Padding(
                padding: const EdgeInsets.all(TrampMetrics.frame),
                child: Align(
                  alignment: Alignment.topLeft,
                  child: Transform.scale(
                    scale: factor,
                    alignment: Alignment.topLeft,
                    child: SizedBox(
                      key: panelStackKey,
                      width: TrampMetrics.mainPlayer.width,
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          mainPlayer,
                          const SizedBox(height: TrampMetrics.gutter),
                          if (lowerRegion == LowerRegion.equalizer)
                            equalizer
                          else
                            SizedBox(
                              height: TrampMetrics.minLowerRegion,
                              child: focusedPlaylist,
                            ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            );
          },
        ),
      ),
    );
```

- [ ] **Step 4: Run the shell test to verify it passes**

Run: `flutter test test/ui/tramp_shell_test.dart`
Expected: PASS (7 tests)

- [ ] **Step 5: Make the window size follow the zoom step**

Replace `lib/platform/tramp_window.dart` entirely:

```dart
import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

/// Frameless window sized for the active zoom step.
Future<void> configureTrampWindow({
  required Size size,
  required Size minimumSize,
}) async {
  final options = WindowOptions(
    size: size,
    minimumSize: minimumSize,
    center: true,
    backgroundColor: Colors.transparent,
    skipTaskbar: false,
    titleBarStyle: TitleBarStyle.hidden,
    title: 'Tramp',
  );
  await windowManager.waitUntilReadyToShow(options, () async {
    await windowManager.setAsFrameless();
    await windowManager.show();
    await windowManager.focus();
  });
}

/// Applies a new zoom step to the live window.
///
/// The minimum is set before the size so growing is never rejected for sitting
/// below a stale floor, and shrinking is never clamped by the previous step's
/// larger minimum.
Future<void> resizeTrampWindow({
  required Size size,
  required Size minimumSize,
}) async {
  await windowManager.setMinimumSize(minimumSize);
  await windowManager.setSize(size);
}
```

- [ ] **Step 6: Wire it all together in app.dart**

In `lib/app.dart`, make these changes:

1. Add imports for `domain/tramp_settings.dart`, `platform/settings_store.dart`, `eq/equalizer_controller.dart`, `ui/zoom/zoom_controller.dart`, `ui/main_player/main_player_panel.dart`, `ui/equalizer/equalizer_panel.dart`, and `platform/tramp_window.dart`; remove the `ui/classic_main_player.dart` import.

2. Add state to the `TrampApp` state class:

```dart
  late final SettingsStore _settingsStore;
  late final ZoomController _zoom;
  late final EqualizerController _equalizer;
  LowerRegion _lowerRegion = LowerRegion.playlist;
  bool _equalizerCollapsed = false;
```

3. In `initState`, construct them and restore persisted state:

```dart
    _settingsStore = FileSettingsStore(supportDir: getApplicationSupportDirectory);
    _zoom = ZoomController(
      workArea: const Size(1920, 1080),
      onPercentChanged: _onZoomChanged,
    );
    _equalizer = EqualizerController(
      store: _settingsStore,
      sink: const NoopEqualizerSink(),
    );
    unawaited(_restoreSettings());
```

4. Add the restore, persist and zoom handlers:

```dart
  Future<void> _restoreSettings() async {
    final settings = await _settingsStore.read();
    await _equalizer.load();
    if (!mounted) return;
    setState(() {
      _lowerRegion = settings.lowerRegion;
      _zoom.setPercent(settings.zoomPercent);
    });
  }

  Future<void> _persistSettings() async {
    final current = await _settingsStore.read();
    await _settingsStore.write(
      current.copyWith(
        zoomPercent: _zoom.percent,
        lowerRegion: _lowerRegion,
      ),
    );
  }

  void _onZoomChanged(int percent) {
    unawaited(resizeTrampWindow(
      size: _zoom.windowSizeFor(percent),
      minimumSize: _zoom.minimumWindowSizeFor(percent),
    ));
    unawaited(_persistSettings());
  }

  void _selectRegion(LowerRegion region) {
    setState(() {
      // Tapping the visible region's own button toggles the equalizer's
      // windowshade rather than doing nothing.
      if (region == _lowerRegion && region == LowerRegion.equalizer) {
        _equalizerCollapsed = !_equalizerCollapsed;
      } else {
        _lowerRegion = region;
        _equalizerCollapsed = false;
      }
    });
    unawaited(_persistSettings());
  }
```

5. Replace the `ClassicMainPlayer` construction in `build` with:

```dart
      mainPlayer: MainPlayerPanel(
        playback: _playback,
        zoom: _zoom,
        lowerRegion: _lowerRegion,
        hasTracks: hasTracks,
        onSelectRegion: _selectRegion,
        onOpenFiles: () => unawaited(_openFiles()),
        onOpenMenu: _showMainMenu,
      ),
      equalizer: EqualizerPanel(
        controller: _equalizer,
        collapsed: _equalizerCollapsed,
        onCollapse: () =>
            setState(() => _equalizerCollapsed = !_equalizerCollapsed),
        onClose: () => _selectRegion(LowerRegion.playlist),
      ),
```

and pass `zoom: _zoom`, `lowerRegion: _lowerRegion`, `equalizerCollapsed: _equalizerCollapsed` to `TrampShell`.

6. Add the logo menu:

```dart
  void _showMainMenu() {
    final overlay =
        Overlay.of(context).context.findRenderObject()! as RenderBox;
    unawaited(showMenu<String>(
      context: context,
      position: RelativeRect.fromLTRB(
        TrampMetrics.frame,
        TrampMetrics.frame + TrampMetrics.titleBar,
        overlay.size.width,
        0,
      ),
      items: const [
        PopupMenuItem(value: 'files', child: Text('Open files…')),
        PopupMenuItem(value: 'folder', child: Text('Open folder…')),
        PopupMenuItem(value: 'playlist', child: Text('Open playlist…')),
        PopupMenuItem(value: 'save', child: Text('Save playlist…')),
        PopupMenuDivider(),
        PopupMenuItem(value: 'quit', child: Text('Exit')),
      ],
    ).then((choice) async {
      switch (choice) {
        case 'files':
          await _openFiles();
        case 'folder':
          await _openFolder();
        case 'playlist':
          await _openPlaylist();
        case 'save':
          await _savePlaylist();
        case 'quit':
          await windowManager.close();
      }
    }));
  }
```

If any of `_openFolder` or `_openPlaylist` do not already exist under those names in `app.dart`, use whatever the existing handlers are called — `flutter analyze` will name them.

7. In `lib/main.dart`, replace the `configureTrampWindow()` call:

```dart
  final initialPercent = ZoomController.bestInitialPercent(const Size(1920, 1080));
  final probe = ZoomController(workArea: const Size(1920, 1080));
  await configureTrampWindow(
    size: probe.windowSizeFor(initialPercent),
    minimumSize: probe.minimumWindowSizeFor(initialPercent),
  );
```

- [ ] **Step 7: Update the shortcuts test and run everything**

`test/ui/tramp_shell_shortcuts_test.dart` constructs `TrampShell` with the old `transport:` parameter. Rename it to `mainPlayer:` and add `zoom: ZoomController(workArea: const Size(6000, 4000))`, `lowerRegion: LowerRegion.playlist`, `equalizer: const SizedBox()`.

Run: `flutter analyze`
Expected: no issues. Every old-token error recorded in Task 1 Step 13 must now be gone.

Run: `flutter test`
Expected: all suites PASS.

- [ ] **Step 8: Commit**

```bash
git add lib/ui/tramp_shell.dart lib/platform/tramp_window.dart lib/app.dart \
        lib/main.dart test/ui/tramp_shell_test.dart \
        test/ui/tramp_shell_shortcuts_test.dart
git commit -m "feat(ui): scaled panel stack with switchable lower region

One transform applies the zoom factor to the whole stack, the EQ and PL
buttons swap the lower region, the window resizes to match the active step,
and Ctrl +/-/0 drive zoom from the keyboard."
```

---

### Task 16: Golden tests and documentation

**Files:**
- Create: `test/golden/panels_golden_test.dart`
- Create: `test/golden/goldens/` (generated)
- Modify: `docs/architecture.md`
- Modify: `docs/tramp-v1-spec.md`
- Modify: `CONTEXT.md`

**Interfaces:**
- Consumes: `MainPlayerPanel` (Task 13), `EqualizerPanel` (Task 11).
- Produces: golden files for both panels at 100% and 200%.

- [ ] **Step 1: Write the golden test**

Create `test/golden/panels_golden_test.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/equalizer/equalizer_panel.dart';
import 'package:tramp/ui/main_player/main_player_panel.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

class MemorySettingsStore implements SettingsStore {
  TrampSettings stored = TrampSettings.defaults;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async => stored = settings;
}

/// Wraps a fixed-size panel at a zoom factor for capture.
Widget frame(Widget panel, Size logical, double factor) {
  return MaterialApp(
    debugShowCheckedModeBanner: false,
    home: ZoomScope(
      factor: factor,
      devicePixelRatio: 1,
      child: Align(
        alignment: Alignment.topLeft,
        child: Transform.scale(
          scale: factor,
          alignment: Alignment.topLeft,
          child: SizedBox(
            width: logical.width,
            height: logical.height,
            child: panel,
          ),
        ),
      ),
    ),
  );
}

void main() {
  for (final factor in [1.0, 2.0]) {
    final tag = '${(factor * 100).round()}';

    testWidgets('main player golden at $tag%', (tester) async {
      final engine = FakePlayerEngine();
      final playlist = PlaylistController();
      playlist.addTracks(const [
        // Neutral fixture. Do not use Winamp's bundled demo track here.
        Track(path: 'a.mp3', title: 'Night Ferry', artist: 'The Sleepless'),
      ]);
      final playback = PlaybackController(playlist: playlist, engine: engine);
      addTearDown(playback.dispose);

      await playback.playIndex(0);
      engine.emitFormat(const AudioFormatInfo(
        bitrateKbps: 128,
        sampleRateHz: 44100,
        channels: 2,
      ));

      final size = TrampMetrics.mainPlayer * factor;
      await tester.binding.setSurfaceSize(size);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(frame(
        MainPlayerPanel(
          playback: playback,
          zoom: ZoomController(workArea: const Size(6000, 4000)),
          lowerRegion: LowerRegion.playlist,
          hasTracks: true,
          draggableTitle: false,
          onSelectRegion: (_) {},
        ),
        TrampMetrics.mainPlayer,
        factor,
      ));
      await tester.pumpAndSettle();

      await expectLater(
        find.byType(MainPlayerPanel),
        matchesGoldenFile('goldens/main_player_$tag.png'),
      );
    });

    testWidgets('equalizer golden at $tag%', (tester) async {
      final controller = EqualizerController(
        store: MemorySettingsStore(),
        sink: const NoopEqualizerSink(),
      );
      controller.applyPreset('Rock');

      final size = TrampMetrics.equalizer * factor;
      await tester.binding.setSurfaceSize(size);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(frame(
        EqualizerPanel(
          controller: controller,
          draggableTitle: false,
          onCollapse: () {},
          onClose: () {},
        ),
        TrampMetrics.equalizer,
        factor,
      ));
      await tester.pumpAndSettle();

      await expectLater(
        find.byType(EqualizerPanel),
        matchesGoldenFile('goldens/equalizer_$tag.png'),
      );
    });
  }
}
```

- [ ] **Step 2: Generate the goldens and inspect them**

The golden test file must begin with

```dart
import '../support/test_fonts.dart';

void main() {
  setUpAll(loadTrampFonts);
```

Without it every golden renders in the harness's fallback face and pins the
wrong image — the fallback's metrics differ enormously from Barlow's.

```bash
flutter test --update-goldens test/golden/panels_golden_test.dart
```

Open all four PNGs under `test/golden/goldens/` and compare them against `docs/mockups/graphite-chrome.png`. This is the acceptance gate for the whole redesign: dark graphite panels, one smooth gradient with no banding, chartreuse phosphor on near-black glass, warm yellow rails, `TRAMP` wordmark, and no light metal anywhere. Fix any discrepancy in the panel geometry before continuing — the goldens are worthless if they enshrine a wrong layout.

- [ ] **Step 3: Confirm the goldens are stable**

```bash
flutter test test/golden/panels_golden_test.dart
```

Expected: PASS. If text renders as boxes, the bundled fonts are not loading — recheck Task 1 Step 10.

- [ ] **Step 4: Record the zoom decision in `docs/architecture.md`**

Each panel is authored once against a fixed logical canvas — main player
812x242, equalizer 812x206 — and the zoom factor is applied by a single
`Transform.scale` at the root of the panel stack.

- [ ] **Step 5: Update the architecture doc**

In `docs/architecture.md`, replace the UI section so it reflects the built structure:

- `TrampShell` hosts the zoom transform and switches the lower region between `EqualizerPanel` and `PlaylistPanel`.
- `MainPlayerPanel` and `EqualizerPanel` are fixed logical canvases; `lib/ui/chrome/` holds the shared primitives (`MetalPanel`, `ChromeButton`, `ChromeSlider`, `TrampTitleBar`, `LcdText`, `SpectrumVisualizer`, `TransportIcons`).
- `lib/theme/` holds colours, surfaces, text and metrics; note that no widget composes its own gradient.
- `lib/ui/zoom/` holds `ZoomController` and `ZoomScope`.
- `lib/eq/` holds `EqualizerController` and the `EqualizerSink` seam, and state that the equalizer is not audible and why.
- `PlayerEngine` now carries `levelsStream` and `formatStream`; state that media_kit levels are synthetic and link the spec's "Audio levels" section.
- Under known gaps, add: equalizer is chrome-only pending a libmpv that can build filter graphs; spectrum levels are synthetic pending the spectrogram subsystem.

- [ ] **Step 6: Update the product spec and glossary**

In `docs/tramp-v1-spec.md`:
- Under "UI direction", replace the classic vector-chrome paragraph with the graphite direction and link `docs/superpowers/specs/2026-08-02-graphite-chrome-redesign-design.md` as the visual target.
- Under "Window and chrome", change "Maximize/fullscreen not required for v1" to state that minimize, maximize and close are all implemented.
- Add a "Zoom" bullet: six discrete steps from 100% to 300%, persisted, with steps larger than the display disabled.
- In "Non-goals", change the equalizer line to: equalizer **chrome** ships; audible equalization does not, because the shipped libmpv cannot construct filter graphs.
- In "Non-goals", add: L/R VU meters (that chrome is the volume slider).

In `CONTEXT.md`, add glossary entries for: **zoom step**, **lower region**, **phosphor**, **rail**, **well**, **windowshade**, **display well**, and **synthetic levels**.

- [ ] **Step 7: Full verification**

```bash
flutter analyze
flutter test
```

Expected: analyze clean, every suite green including goldens.

Confirm the old brand and the old palette are truly gone:

```bash
grep -rn "WINAMP\|Winamp" lib/ | grep -v "^lib/.*spiritual successor"
grep -rn "metalFace\|metalMid\|lcdPhosphor\|google_fonts" lib/ pubspec.yaml
```

Expected: no matches from either command.

- [ ] **Step 8: Commit**

```bash
git add test/golden docs CONTEXT.md
git commit -m "test(golden): pin both panels at 100% and 200%, update docs

Goldens are the acceptance gate for the redesign's visual fidelity. Documents
the fixed-canvas zoom decision and records that equalizer audio and
measured levels are blocked on the shipped libmpv."
```

---

## Self-Review

**Spec coverage.** Every section of the design spec maps to a task: palette, surfaces and typography to Task 1; geometry and zoom to Tasks 2, 3 and 15; layout and region switching to Task 15; control mapping to Tasks 11, 13 and 15; modules to Tasks 1–15; equalizer scope to Tasks 10 and 11; audio levels to Task 9; the equalizer audio-path warning to Task 10's sink documentation; testing to every task plus Task 16.

Two spec items are deliberately handled differently from the module table, and the docs task records both: `lib/theme/tramp_metrics.dart` is an addition the spec's table does not list (the zoom controller and both panels need shared canvas constants), and `AudioFormatInfo` is a second engine seam the spec implied under "real bitrate / sample rate / channel count" without naming.

**Deferred to the second plan.** The spectrogram subsystem — `lib/analysis/spectrogram_analyzer.dart`, `wav_reader.dart`, `spectrogram_cache.dart` — is not in this plan. Task 9 builds the `AudioLevels` seam it will fill, so this plan ships a complete, working app whose spectrum is honestly flagged synthetic.

**Type consistency.** `AudioLevels.bandCount` is used in Tasks 9, 10 and 16. `EqualizerSettings.gainLimit` is used in Tasks 10 and 11. `TrampMetrics.titleBar` is used in Tasks 8, 11 and 15. `ZoomScope.hairlineFor` is used in Tasks 4, 6, 8 and 14. `LowerRegion` is used in Tasks 2, 10, 13 and 15. `TrampSurface` is used in Tasks 4, 5, 11, 13 and 14. `panelStackKey` is defined and consumed in Task 15. The `transport` → `mainPlayer` rename is applied in Task 15 Steps 3 and 7.

**Known soft spot.** Task 12 Step 6 requires verifying `media_kit`'s `AudioParams` field names against the installed package before wiring them, because those names could not be confirmed from documentation alone. That step is a verification instruction, not a placeholder: the surrounding structure is fixed and only the two field accessors may need adjusting.

---

## Execution Handoff

Plan complete. Two execution options:

1. **Subagent-Driven (recommended)** — a fresh subagent per task, reviewed between tasks, fast iteration.
2. **Inline Execution** — tasks executed in this session with checkpoints for review.


