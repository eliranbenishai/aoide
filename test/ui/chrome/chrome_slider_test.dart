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
}
