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
      'bolt': TransportIcons.bolt(),
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

  test('bolt paints in the rail accent by default', () {
    expect(TransportIcons.defaultBoltColour, TrampColors.railAccent);
  });
}
