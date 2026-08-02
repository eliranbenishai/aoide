import 'dart:typed_data';
import 'dart:ui' show PictureRecorder, Canvas;

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
    expect(TrampSurfaces.pressedButton().decoration.border, isNull);
    expect(TrampSurfaces.insetWell().decoration.border, isNull);
    expect(TrampSurfaces.lcdGlass().decoration.border, isNull);
  });

  test('raised surfaces highlight the top-left and shadow the bottom-right',
      () {
    final spec = TrampSurfaces.raisedPanel();
    // highlight = top + left; shadow = bottom + right.
    expect(spec.highlight, TrampColors.bevelHi);
    expect(spec.shadow, TrampColors.bevelLo);
  });

  test('inset surfaces reverse the bevel direction', () {
    final spec = TrampSurfaces.insetWell();
    expect(spec.highlight, TrampColors.bevelLo);
    expect(spec.shadow, TrampColors.bevelHi);
    expect(spec.decoration.color, TrampColors.wellDeep);
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
