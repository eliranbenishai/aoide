import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_palette.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/theme/look_paint.dart';
import 'package:tramp/ui/chrome/mockup/mockup_button.dart';
import 'package:tramp/ui/chrome/mockup/mockup_hover.dart';

import '../../../support/test_fonts.dart';
import '../../../support/look_harness.dart';

Widget host(Widget child) => lookHost(child);

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

    final onInk = LookPaint.buttonOnInk(BuiltinLook.resolved.palette);
    expect(
      find.descendant(
        of: find.byType(MockupButton),
        matching: find.byWidgetPredicate(
          (w) =>
              w is ColorFiltered &&
              w.colorFilter == ColorFilter.mode(onInk, BlendMode.srcIn),
        ),
      ),
      findsOneWidget,
    );
  });

  testWidgets('on-state label ink follows look phosphorDeep', (tester) async {
    final look = _probeLook(
      phosphorDeep: const Color(0xFF3D2A00),
    );
    await tester.pumpWidget(
      Directionality(
        textDirection: TextDirection.ltr,
        child: wrapWithLook(
          Center(
            child: MockupButton(
              on: true,
              label: 'EQ',
              onPressed: () {},
            ),
          ),
          look: look,
        ),
      ),
    );

    final text = tester.widget<Text>(find.text('EQ'));
    expect(text.style?.color, LookPaint.buttonOnInk(look.palette));
    expect(text.style?.color, isNot(BuiltinLook.resolved.palette.phosphorDeep));
  });
}

ResolvedLook _probeLook({required Color phosphorDeep}) {
  final b = BuiltinLook.resolved.palette;
  return ResolvedLook(
    id: 'probe',
    name: 'Probe',
    palette: LookPalette(
      shellHighlight: b.shellHighlight,
      shellBase: b.shellBase,
      shellMid: b.shellMid,
      shellLow: b.shellLow,
      shellDeep: b.shellDeep,
      inkDefault: b.inkDefault,
      inkDim: b.inkDim,
      inkFaint: b.inkFaint,
      phosphorDefault: b.phosphorDefault,
      phosphorHot: b.phosphorHot,
      phosphorDim: b.phosphorDim,
      phosphorDeep: phosphorDeep,
      accentDefault: b.accentDefault,
      accentDim: b.accentDim,
      well: b.well,
    ),
    materials: BuiltinLook.resolved.materials,
    chromeFamily: 'TrampCondensed',
    lcdFamily: 'TrampMono',
  );
}
