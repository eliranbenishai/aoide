import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/nine_slice_skin.dart';

void main() {
  group('NineSlicePainter.shouldRepaint', () {
    const slices = PlaylistSlices.graphite;
    final sharedImages = <String, ui.Image>{};

    test('returns false when slices and loadedCount unchanged', () {
      final a = NineSlicePainter(
        slices: slices,
        images: sharedImages,
        loadedCount: 3,
      );
      final b = NineSlicePainter(
        slices: slices,
        images: sharedImages,
        loadedCount: 3,
      );
      expect(a.shouldRepaint(b), isFalse);
    });

    test('returns true when loadedCount increases (shared map)', () {
      final before = NineSlicePainter(
        slices: slices,
        images: sharedImages,
        loadedCount: 0,
      );
      final after = NineSlicePainter(
        slices: slices,
        images: sharedImages,
        loadedCount: 9,
      );
      expect(after.shouldRepaint(before), isTrue);
    });

    test('returns true when slices change', () {
      final a = NineSlicePainter(
        slices: slices,
        images: sharedImages,
        loadedCount: 9,
      );
      final b = NineSlicePainter(
        slices: PlaylistSlices(
          nw: slices.nw,
          n: slices.n,
          ne: slices.ne,
          w: slices.w,
          e: slices.e,
          sw: slices.sw,
          s: slices.s,
          se: slices.se,
          well: 'assets/other/well.png',
          border: slices.border,
        ),
        images: sharedImages,
        loadedCount: 9,
      );
      expect(b.shouldRepaint(a), isTrue);
    });
  });

  testWidgets('NineSliceSkin expands to its parent', (tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 800));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      const MaterialApp(
        home: Center(
          child: SizedBox(
            width: 900,
            height: 500,
            child: NineSliceSkin(
              slices: PlaylistSlices.graphite,
              child: SizedBox.shrink(),
            ),
          ),
        ),
      ),
    );

    expect(
      tester.getSize(find.byType(NineSliceSkin)),
      const Size(900, 500),
    );
  });

  testWidgets('the well child is inset by the slice border', (tester) async {
    const slices = PlaylistSlices.graphite;
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 400,
            height: 300,
            child: NineSliceSkin(
              slices: slices,
              child: const Align(
                alignment: Alignment.topLeft,
                child: SizedBox(key: Key('well-child'), width: 1, height: 1),
              ),
            ),
          ),
        ),
      ),
    );

    final skin = tester.getTopLeft(find.byType(NineSliceSkin));
    final childTopLeft = tester.getTopLeft(find.byKey(const Key('well-child')));
    expect(childTopLeft.dx - skin.dx, slices.border.left);
    expect(childTopLeft.dy - skin.dy, slices.border.top);
  });

  testWidgets('loads all slice PNGs after decode', (tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: Center(
          child: SizedBox(
            width: 400,
            height: 300,
            child: NineSliceSkin(
              slices: PlaylistSlices.graphite,
              child: SizedBox.shrink(),
            ),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    final paintFinder = find.descendant(
      of: find.byType(NineSliceSkin),
      matching: find.byWidgetPredicate(
        (w) => w is CustomPaint && w.painter is NineSlicePainter,
      ),
    );
    final painter =
        tester.widget<CustomPaint>(paintFinder).painter! as NineSlicePainter;
    expect(painter.loadedCount, PlaylistSlices.graphite.assets.length);
    expect(painter.images.length, PlaylistSlices.graphite.assets.length);
  });
}
