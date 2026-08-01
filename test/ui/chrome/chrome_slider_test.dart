import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/chrome_slider.dart';

void main() {
  testWidgets('ChromeSlider reports drag end value', (tester) async {
    double? ended;
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: SizedBox(
            width: 200,
            height: 40,
            child: ChromeSlider(
              value: 0.2,
              onChanged: (_) {},
              onChangeEnd: (v) => ended = v,
            ),
          ),
        ),
      ),
    );
    await tester.drag(find.byType(ChromeSlider), const Offset(100, 0));
    await tester.pumpAndSettle();
    expect(ended, isNotNull);
    expect(ended!, greaterThan(0.2));
  });

  testWidgets('ChromeSlider clears drag preview on cancel without ending', (
    tester,
  ) async {
    double? ended;
    final changed = <double>[];
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: SizedBox(
            width: 200,
            height: 40,
            child: ChromeSlider(
              value: 0.2,
              onChanged: changed.add,
              onChangeEnd: (v) => ended = v,
            ),
          ),
        ),
      ),
    );

    final center = tester.getCenter(find.byType(ChromeSlider));
    final gesture = await tester.startGesture(center);
    await gesture.moveBy(const Offset(80, 0));
    await tester.pump();
    expect(changed, isNotEmpty);

    // Pointer-cancel maps to drag end in the recognizer; exercise the arena
    // rejection cancel callback that must clear the preview without committing.
    final detector = tester.widget<GestureDetector>(
      find.descendant(
        of: find.byType(ChromeSlider),
        matching: find.byType(GestureDetector),
      ),
    );
    detector.onHorizontalDragCancel!.call();
    await tester.pump();

    expect(ended, isNull);
    final thumb = tester.widget<Positioned>(
      find.descendant(
        of: find.byType(ChromeSlider),
        matching: find.byType(Positioned),
      ),
    );
    expect(thumb.left, closeTo(0.2 * (200 - 12), 0.5));

    await gesture.up();
  });
}
