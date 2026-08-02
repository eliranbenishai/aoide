import 'dart:typed_data';
import 'dart:ui' show Canvas, PictureRecorder;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/tramp_mark.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

void main() {
  testWidgets('paints at its requested size', (tester) async {
    await tester.pumpWidget(host(const TrampMark(size: 19)));
    expect(tester.takeException(), isNull);
    expect(tester.getSize(find.byType(TrampMark)), const Size(19, 19));
  });

  testWidgets('defaults to the neutral chrome label colour', (tester) async {
    await tester.pumpWidget(host(const TrampMark()));
    final painter = tester
        .widget<CustomPaint>(find.byType(CustomPaint).first)
        .painter! as TrampMarkPainter;
    expect(painter.colour, TrampColors.label);
  });

  testWidgets('is tintable like any other chrome glyph', (tester) async {
    await tester.pumpWidget(
      host(const TrampMark(colour: TrampColors.phosphor)),
    );
    final painter = tester
        .widget<CustomPaint>(find.byType(CustomPaint).first)
        .painter! as TrampMarkPainter;
    expect(painter.colour, TrampColors.phosphor);
  });

  testWidgets('carries the brand name for screen readers', (tester) async {
    await tester.pumpWidget(host(const TrampMark()));
    expect(find.bySemanticsLabel('Tramp'), findsOneWidget);
  });

  test('repaints only when the tint changes', () {
    const a = TrampMarkPainter(colour: TrampColors.label);
    const b = TrampMarkPainter(colour: TrampColors.phosphor);
    expect(a.shouldRepaint(b), isTrue);
    expect(a.shouldRepaint(const TrampMarkPainter(colour: TrampColors.label)),
        isFalse);
  });

  test('degenerate sizes paint nothing rather than throwing', () {
    final recorder = PictureRecorder();
    const TrampMarkPainter(colour: TrampColors.label)
        .paint(Canvas(recorder), Size.zero);
    expect(recorder.endRecording(), isNotNull);
  });

  // The whole reason this widget exists is legibility at title-bar size, so
  // assert it actually marks pixels there rather than collapsing to nothing.
  group('at 19 logical pixels', () {
    Future<Uint32List> render(double side) async {
      final recorder = PictureRecorder();
      const TrampMarkPainter(colour: TrampColors.label)
          .paint(Canvas(recorder), Size(side, side));
      final image = await recorder
          .endRecording()
          .toImage(side.toInt(), side.toInt());
      final data = await image.toByteData();
      return data!.buffer.asUint32List();
    }

    test('marks a meaningful share of its box', () async {
      const side = 19.0;
      final pixels = await render(side);
      final painted = pixels.where((p) => p != 0).length;
      final total = side.toInt() * side.toInt();
      final share = painted / total;
      // Enough ink to read as a shape, not so much that it is a filled blob.
      expect(share, greaterThan(0.15), reason: 'too faint to read at 19px');
      expect(share, lessThan(0.75), reason: 'too solid to read as a mark');
    });

    test('leaves the corners clear, so the ring reads as round', () async {
      const side = 19.0;
      final pixels = await render(side);
      expect(pixels[0], 0, reason: 'top-left corner must be outside the ring');
      expect(pixels[side.toInt() - 1], 0, reason: 'top-right corner');
    });
  });
}
