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

  testWidgets('free-resize logical child tracks surface / factor', (tester) async {
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

    expect(
      logicalChildSize,
      Size(surface.width / factor, surface.height / factor),
    );
  });

  testWidgets(
      'fixed logicalSize keeps authored canvas when the surface rounds short',
      (tester) async {
    Size? logicalChildSize;
    // 75% of 825×348 is 618.75×261; OS-style rounding short by a fraction.
    const surface = Size(618.5, 260.8);
    const authored = Size(825, 348);
    const factor = 0.75;
    await tester.binding.setSurfaceSize(const Size(640, 280));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: SizedBox(
          width: surface.width,
          height: surface.height,
          child: ZoomedCanvas(
            factor: factor,
            logicalSize: authored,
            child: LayoutBuilder(
              builder: (context, constraints) {
                logicalChildSize = Size(
                  constraints.maxWidth,
                  constraints.maxHeight,
                );
                // Fixed chrome that must not be asked to lay out under 825×348.
                return const SizedBox(
                  width: 825,
                  height: 348,
                  child: ColoredBox(color: Color(0xFF00FF00)),
                );
              },
            ),
          ),
        ),
      ),
    );

    expect(logicalChildSize, authored);
    expect(tester.takeException(), isNull);
  });

  testWidgets(
    'hit tests reach bottom-right controls when factor < 1',
    (tester) async {
      var taps = 0;
      // Default app zoom is 75%: window = logical × 0.75.
      const factor = 0.75;
      const logical = Size(825, 348);
      final surface = Size(logical.width * factor, logical.height * factor);
      await tester.binding.setSurfaceSize(surface);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(
        MaterialApp(
          home: SizedBox(
            width: surface.width,
            height: surface.height,
            child: ZoomedCanvas(
              factor: factor,
              logicalSize: logical,
              child: SizedBox(
                width: logical.width,
                height: logical.height,
                child: Stack(
                  children: [
                    Positioned(
                      right: 20,
                      bottom: 20,
                      child: GestureDetector(
                        key: const Key('br-btn'),
                        onTap: () => taps++,
                        child: const SizedBox(
                          width: 66,
                          height: 50,
                          child: ColoredBox(color: Color(0xFF00FF00)),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      );

      await tester.tap(find.byKey(const Key('br-btn')));
      await tester.pump();
      expect(taps, 1);
    },
  );

  testWidgets(
    'free-resize hit tests reach bottom-right when factor < 1',
    (tester) async {
      var taps = 0;
      const factor = 0.75;
      const surface = Size(600, 400);
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
                  return SizedBox(
                    width: constraints.maxWidth,
                    height: constraints.maxHeight,
                    child: Stack(
                      children: [
                        Positioned(
                          right: 12,
                          bottom: 12,
                          child: GestureDetector(
                            key: const Key('pl-br-btn'),
                            onTap: () => taps++,
                            child: const SizedBox(
                              width: 52,
                              height: 52,
                              child: ColoredBox(color: Color(0xFF00FF00)),
                            ),
                          ),
                        ),
                      ],
                    ),
                  );
                },
              ),
            ),
          ),
        ),
      );

      await tester.tap(find.byKey(const Key('pl-br-btn')));
      await tester.pump();
      expect(taps, 1);
    },
  );
}
