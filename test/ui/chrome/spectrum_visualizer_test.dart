import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_levels.dart';
import 'package:tramp/ui/chrome/spectrum_visualizer.dart';
import '../../support/look_harness.dart';

Widget host(Stream<AudioLevels> levels) => lookHost(
      SizedBox(
        width: 228,
        height: 96,
        child: SpectrumVisualizer(levels: levels),
      ),
    );

SpectrumPainter painterOf(WidgetTester tester) =>
    tester.widget<CustomPaint>(find.byType(CustomPaint).first).painter!
        as SpectrumPainter;

void main() {
  late StreamController<AudioLevels> controller;

  // sync: true so FakeAsync delivers frames inside the same pump() as add().
  // Async broadcast schedules a microtask that one pump() does not rebuild from.
  setUp(() => controller = StreamController<AudioLevels>.broadcast(sync: true));
  tearDown(() async {
    if (!controller.isClosed) await controller.close();
  });

  testWidgets('starts silent', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    expect(painterOf(tester).bars.every((b) => b == 0), isTrue);
  });

  testWidgets('rises immediately on a loud frame (fast attack)',
      (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 1),
      leftRms: 1,
      rightRms: 1,
      synthetic: false,
    ));
    await tester.pump();
    expect(painterOf(tester).bars.first, greaterThan(0.9));
  });

  testWidgets('decays gradually rather than snapping to zero', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 1),
      leftRms: 1,
      rightRms: 1,
      synthetic: false,
    ));
    await tester.pump();
    controller.add(AudioLevels.silent);
    await tester.pump();
    final bar = painterOf(tester).bars.first;
    expect(bar, lessThan(1.0));
    expect(bar, greaterThan(0.0), reason: 'slow decay, not an instant drop');
  });

  testWidgets('peak caps sit at or above the bars', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    controller.add(AudioLevels(
      bands: List<double>.filled(AudioLevels.bandCount, 0.8),
      leftRms: 0.8,
      rightRms: 0.8,
      synthetic: false,
    ));
    await tester.pump();
    final p = painterOf(tester);
    for (var i = 0; i < p.bars.length; i++) {
      expect(p.peaks[i], greaterThanOrEqualTo(p.bars[i]));
    }
  });

  testWidgets('survives a stream that closes', (tester) async {
    await tester.pumpWidget(host(controller.stream));
    await controller.close();
    await tester.pump();
    expect(tester.takeException(), isNull);
  });
}
