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
