import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/chrome_button.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';

import '../../support/test_fonts.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

TrampSurface surfaceOf(WidgetTester tester) =>
    tester.widget<MetalPanel>(find.byType(MetalPanel)).surface;

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('label button reports its text and fires onPressed',
      (tester) async {
    var taps = 0;
    await tester.pumpWidget(host(
      ChromeButton(text: 'OPEN', onPressed: () => taps++),
    ));
    expect(find.text('OPEN'), findsOneWidget);
    await tester.tap(find.byType(ChromeButton));
    expect(taps, 1);
  });

  testWidgets('a null onPressed makes the button inert and dim', (tester) async {
    await tester.pumpWidget(host(
      const ChromeButton(text: 'OPEN', onPressed: null),
    ));
    await tester.tap(find.byType(ChromeButton));
    expect(tester.takeException(), isNull);
    final text = tester.widget<Text>(find.text('OPEN'));
    expect(text.style!.color, TrampColors.labelDim);
  });

  testWidgets('pressing swaps to the pressed surface and back', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton(text: 'OPEN', onPressed: () {}),
    ));
    expect(surfaceOf(tester), TrampSurface.raisedButton);

    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(ChromeButton)),
    );
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.pressedButton);

    await gesture.up();
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.raisedButton);
  });

  testWidgets('a disabled button never shows the pressed surface',
      (tester) async {
    await tester.pumpWidget(host(
      const ChromeButton(text: 'OPEN', onPressed: null),
    ));
    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(ChromeButton)),
    );
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.raisedButton);
    await gesture.up();
  });

  testWidgets('disabling mid-press clears the pressed surface on release',
      (tester) async {
    await tester.pumpWidget(host(
      ChromeButton(text: 'OPEN', onPressed: () {}),
    ));
    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(ChromeButton)),
    );
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.pressedButton);

    await tester.pumpWidget(host(
      const ChromeButton(text: 'OPEN', onPressed: null),
    ));
    await gesture.up();
    await tester.pump();
    expect(surfaceOf(tester), TrampSurface.raisedButton);
  });

  testWidgets('explicit size is honoured', (tester) async {
    await tester.pumpWidget(host(
      ChromeButton(
        text: 'OPEN',
        onPressed: () {},
        size: const Size(54, 26),
      ),
    ));
    expect(tester.getSize(find.byType(ChromeButton)), const Size(54, 26));
    expect(tester.takeException(), isNull);
  });
}
