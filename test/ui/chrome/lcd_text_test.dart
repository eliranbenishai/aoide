import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/lcd_text.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

void main() {
  testWidgets('lit text uses phosphor, unlit uses the dim tone',
      (tester) async {
    await tester.pumpWidget(host(const LcdText('0:05')));
    expect(
      tester.widget<Text>(find.text('0:05')).style!.color,
      TrampColors.phosphor,
    );

    await tester.pumpWidget(host(const LcdText('0:05', lit: false)));
    expect(
      tester.widget<Text>(find.text('0:05')).style!.color,
      TrampColors.phosphorDim,
    );
  });

  testWidgets('large size renders at the big readout scale', (tester) async {
    await tester.pumpWidget(host(const LcdText('0:05', size: LcdSize.large)));
    final normal = tester.widget<Text>(find.text('0:05')).style!.fontSize!;
    await tester.pumpWidget(host(const LcdText('0:05')));
    final small = tester.widget<Text>(find.text('0:05')).style!.fontSize!;
    expect(normal, greaterThan(small));
  });

  testWidgets('all LCD text uses the bundled mono family', (tester) async {
    await tester.pumpWidget(host(const LcdText('128 kbps')));
    expect(
      tester.widget<Text>(find.text('128 kbps')).style!.fontFamily,
      'TrampMono',
    );
  });

  testWidgets('indicator lights when active and reports taps', (tester) async {
    var taps = 0;
    await tester.pumpWidget(host(
      LcdIndicator('EQ', lit: true, onTap: () => taps++),
    ));
    expect(
      tester.widget<Text>(find.text('EQ')).style!.color,
      TrampColors.phosphor,
    );
    await tester.tap(find.text('EQ'));
    expect(taps, 1);
  });

  testWidgets('indicator with no callback is inert', (tester) async {
    await tester.pumpWidget(host(
      const LcdIndicator('PL', lit: false, onTap: null),
    ));
    await tester.tap(find.text('PL'));
    expect(tester.takeException(), isNull);
  });
}
