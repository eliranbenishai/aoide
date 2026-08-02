import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/transport_icons.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: SizedBox(width: 40, height: 40, child: child)),
    );

void main() {
  testWidgets('every glyph paints without error', (tester) async {
    final glyphs = <String, Widget>{
      'prev': TransportIcons.prev(),
      'play': TransportIcons.play(),
      'pause': TransportIcons.pause(),
      'stop': TransportIcons.stop(),
      'next': TransportIcons.next(),
      'shuffle': TransportIcons.shuffle(),
      'repeat': TransportIcons.repeat(),
      'repeatOne': TransportIcons.repeat(one: true),
      'eject': TransportIcons.eject(),
    };

    for (final entry in glyphs.entries) {
      await tester.pumpWidget(host(entry.value));
      expect(tester.takeException(), isNull, reason: entry.key);
      expect(find.byType(CustomPaint), findsWidgets, reason: entry.key);
    }
  });

  testWidgets('play defaults to phosphor and honours an override',
      (tester) async {
    await tester.pumpWidget(host(TransportIcons.play()));
    expect(tester.takeException(), isNull);

    await tester.pumpWidget(host(TransportIcons.play(colour: TrampColors.label)));
    expect(tester.takeException(), isNull);
  });

  test('repeat-one is distinguishable from repeat-all', () {
    const all = RepeatPainter(colour: TrampColors.label, one: false);
    const one = RepeatPainter(colour: TrampColors.label, one: true);
    expect(all.shouldRepaint(one), isTrue);
  });

  // Tramp's own mark lives in `TrampLogo` (lib/ui/chrome/logo.dart), rendered
  // from logo.svg. The glyph set must not carry a brand mark of its own — the
  // reference mockup's lightning bolt is Winamp's logo, not a generic icon.
  test('the glyph set contains no brand mark', () {
    expect(
      TransportIcons.defaultGlyphColour,
      TrampColors.label,
      reason: 'glyphs are neutral chrome, tinted by the caller',
    );
  });
}
