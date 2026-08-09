import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';
import 'package:tramp/ui/zoom/zoomed_canvas.dart';

void main() {
  testWidgets('exposes ZoomScope factor and fills the surface', (tester) async {
    double? seenFactor;
    const surface = Size(1650, 696);
    await tester.binding.setSurfaceSize(surface);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: SizedBox(
          width: surface.width,
          height: surface.height,
          child: ZoomedCanvas(
            factor: 2,
            child: SizedBox(
              width: 825,
              height: 348,
              child: Builder(
                builder: (context) {
                  seenFactor = ZoomScope.of(context).factor;
                  return const ColoredBox(color: Color(0xFF00FF00));
                },
              ),
            ),
          ),
        ),
      ),
    );

    expect(seenFactor, 2);
    expect(tester.getSize(find.byType(ZoomedCanvas)), surface);
  });
}
