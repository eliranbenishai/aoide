import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:tramp/app.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/platform/os_media_controls_stub.dart';

import 'support/test_fonts.dart';

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('TrampApp shows chrome shell with brand', (WidgetTester tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 900));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      TrampApp(
        engine: FakePlayerEngine(),
        osMediaControls: NoOpOsMediaControls(),
      ),
    );
    await tester.pump();
    expect(find.textContaining('TRAMP'), findsWidgets);
  });
}
