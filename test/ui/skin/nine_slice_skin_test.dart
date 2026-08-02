import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/nine_slice_skin.dart';

void main() {
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
}
