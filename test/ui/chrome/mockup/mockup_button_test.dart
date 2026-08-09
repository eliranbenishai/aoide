import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_button.dart';
import 'package:tramp/ui/chrome/mockup/mockup_hover.dart';

import '../../../support/test_fonts.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: child),
    );

Opacity disabledOpacityOf(WidgetTester tester) {
  return tester.widget<Opacity>(
    find.descendant(
      of: find.byType(MockupButton),
      matching: find.byWidgetPredicate(
        (w) => w is Opacity && w.opacity < 1.0,
      ),
    ),
  );
}

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('null onPressed dims the face and reports disabled', (tester) async {
    await tester.pumpWidget(host(
      const MockupButton(label: 'EQ', onPressed: null),
    ));

    final semantics = tester.getSemantics(find.byType(MockupButton));
    expect(semantics.hasFlag(SemanticsFlag.isEnabled), isFalse);

    final opacity = disabledOpacityOf(tester);
    expect(opacity.opacity, MockupHoverTokens.disabledOpacity);
  });

  testWidgets('enabled button stays fully opaque at rest', (tester) async {
    await tester.pumpWidget(host(
      MockupButton(label: 'EQ', onPressed: () {}),
    ));

    expect(
      find.descendant(
        of: find.byType(MockupButton),
        matching: find.byWidgetPredicate(
          (w) => w is Opacity && w.opacity < 1.0,
        ),
      ),
      findsNothing,
    );

    final semantics = tester.getSemantics(find.byType(MockupButton));
    expect(semantics.hasFlag(SemanticsFlag.isEnabled), isTrue);
  });

  testWidgets('disabled button does not fire onPressed', (tester) async {
    var taps = 0;
    await tester.pumpWidget(host(
      const MockupButton(label: 'EQ', onPressed: null),
    ));
    await tester.tap(find.byType(MockupButton));
    expect(taps, 0);
    expect(tester.takeException(), isNull);
  });

  testWidgets('on state tints icon children to btn--on ink', (tester) async {
    await tester.pumpWidget(host(
      MockupButton(
        on: true,
        onPressed: () {},
        child: const Icon(Icons.play_arrow, color: Color(0xFFFFFFFF)),
      ),
    ));

    expect(
      find.descendant(
        of: find.byType(MockupButton),
        matching: find.byWidgetPredicate(
          (w) =>
              w is ColorFiltered &&
              w.colorFilter ==
                  const ColorFilter.mode(Color(0xFF04222B), BlendMode.srcIn),
        ),
      ),
      findsOneWidget,
    );
  });
}
