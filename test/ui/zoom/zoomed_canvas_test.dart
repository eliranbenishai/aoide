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
            child: Builder(
              builder: (context) {
                seenFactor = ZoomScope.of(context).factor;
                return const ColoredBox(color: Color(0xFF00FF00));
              },
            ),
          ),
        ),
      ),
    );

    expect(seenFactor, 2);
    expect(tester.getSize(find.byType(ZoomedCanvas)), surface);
  });

  testWidgets('logical child size tracks surface / factor without stretch',
      (tester) async {
    Size? logicalChildSize;
    const surface = Size(900, 600);
    const factor = 1.5;
    await tester.binding.setSurfaceSize(surface);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: SizedBox(
          width: surface.width,
          height: surface.height,
          child: ZoomedCanvas(
            factor: factor,
            child: LayoutBuilder(
              builder: (context, constraints) {
                logicalChildSize = Size(
                  constraints.maxWidth,
                  constraints.maxHeight,
                );
                return const ColoredBox(color: Color(0xFF00FF00));
              },
            ),
          ),
        ),
      ),
    );

    expect(logicalChildSize, Size(surface.width / factor, surface.height / factor));

    // Mid-resize: aspect changes; logical canvas follows — uniform scale only.
    const resized = Size(1200, 500);
    await tester.binding.setSurfaceSize(resized);
    await tester.pumpWidget(
      MaterialApp(
        home: SizedBox(
          width: resized.width,
          height: resized.height,
          child: ZoomedCanvas(
            factor: factor,
            child: LayoutBuilder(
              builder: (context, constraints) {
                logicalChildSize = Size(
                  constraints.maxWidth,
                  constraints.maxHeight,
                );
                return const ColoredBox(color: Color(0xFF00FF00));
              },
            ),
          ),
        ),
      ),
    );

    expect(
      logicalChildSize,
      Size(resized.width / factor, resized.height / factor),
    );
  });
}
