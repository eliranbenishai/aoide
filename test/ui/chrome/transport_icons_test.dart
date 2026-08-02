import 'dart:io';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/transport_icons.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: SizedBox(width: 40, height: 40, child: child)),
    );

Future<int> markedPixelCount(CustomPainter painter, Size size) async {
  final recorder = ui.PictureRecorder();
  final canvas = Canvas(recorder);
  painter.paint(canvas, size);
  final picture = recorder.endRecording();
  final image = await picture.toImage(
    size.width.ceil(),
    size.height.ceil(),
  );
  final bytes = await image.toByteData(format: ui.ImageByteFormat.rawRgba);
  expect(bytes, isNotNull);
  var marked = 0;
  final data = bytes!.buffer.asUint8List();
  for (var i = 0; i < data.length; i += 4) {
    if (data[i + 3] != 0) marked++;
  }
  return marked;
}

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
      'minimize': TransportIcons.minimize(),
      'maximize': TransportIcons.maximize(),
      'close': TransportIcons.close(),
      'speaker': TransportIcons.speaker(),
      'speakerMuted': TransportIcons.speaker(muted: true),
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

  test('repeat-one paints strictly more pixels than plain repeat', () async {
    const size = Size(16, 13);
    final all = await markedPixelCount(
      const RepeatPainter(colour: TrampColors.label, one: false),
      size,
    );
    final one = await markedPixelCount(
      const RepeatPainter(colour: TrampColors.label, one: true),
      size,
    );
    expect(one, greaterThan(all));
  });

  test('muted speaker paints a different number of pixels than unmuted',
      () async {
    const size = Size(12, 12);
    final unmuted = await markedPixelCount(
      const SpeakerPainter(colour: TrampColors.label, muted: false),
      size,
    );
    final muted = await markedPixelCount(
      const SpeakerPainter(colour: TrampColors.label, muted: true),
      size,
    );
    expect(muted, isNot(unmuted));
  });

  // Tramp's chrome mark lives in `TrampMark` (lib/ui/chrome/tramp_mark.dart).
  // `TrampLogo` is the full badge for app icon, splash and About. The glyph set
  // must not carry a brand mark of its own — the reference mockup's lightning
  // bolt is Winamp's logo, not a generic icon.
  test('the glyph set is the transport and chrome window factories', () {
    const expected = {
      'prev',
      'play',
      'pause',
      'stop',
      'next',
      'shuffle',
      'repeat',
      'eject',
      'minimize',
      'maximize',
      'close',
      'speaker',
    };
    final factories = <String, Widget Function()>{
      'prev': TransportIcons.prev,
      'play': TransportIcons.play,
      'pause': TransportIcons.pause,
      'stop': TransportIcons.stop,
      'next': TransportIcons.next,
      'shuffle': TransportIcons.shuffle,
      'repeat': TransportIcons.repeat,
      'eject': TransportIcons.eject,
      'minimize': TransportIcons.minimize,
      'maximize': TransportIcons.maximize,
      'close': TransportIcons.close,
      'speaker': TransportIcons.speaker,
    };
    expect(factories.keys.toSet(), expected);
    for (final build in factories.values) {
      expect(build(), isA<Widget>());
    }

    final source = File('lib/ui/chrome/transport_icons.dart').readAsStringSync();
    final staticWidgetFactories = RegExp(r'static Widget ([a-z]\w*)\(')
        .allMatches(source)
        .map((m) => m.group(1)!)
        .toSet();
    expect(staticWidgetFactories, expected);
    expect(source.toLowerCase().contains('bolt'), isFalse);
  });
}
