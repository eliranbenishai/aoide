// Golden images for code-constructed mockup chrome primitives.
//
// Platform note: Flutter goldens are platform-specific. These images are
// generated on Windows; other OSes need their own set or a tolerance compare.
@Tags(['golden'])
library;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/ui/chrome/mockup/mockup_button.dart';
import 'package:tramp/ui/chrome/mockup/mockup_icons.dart';
import 'package:tramp/ui/chrome/mockup/mockup_led.dart';
import 'package:tramp/ui/chrome/mockup/mockup_screen.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';
import 'package:tramp/ui/chrome/mockup/mockup_slider.dart';
import 'package:tramp/ui/chrome/mockup/mockup_title_bar.dart';

import '../../../support/test_fonts.dart';

Widget _frame(Widget child, Size size) {
  return MaterialApp(
    debugShowCheckedModeBanner: false,
    home: Align(
      alignment: Alignment.topLeft,
      child: SizedBox(
        width: size.width,
        height: size.height,
        child: ColoredBox(
          color: MockupTokens.shellDeep,
          child: Material(
            color: Colors.transparent,
            child: child,
          ),
        ),
      ),
    ),
  );
}

Future<void> _pumpGolden(
  WidgetTester tester,
  Widget child,
  Size size,
  String name,
) async {
  await tester.binding.setSurfaceSize(size);
  addTearDown(() => tester.binding.setSurfaceSize(null));
  await tester.pumpWidget(_frame(child, size));
  await tester.pumpAndSettle();
  await expectLater(
    find.byType(MaterialApp),
    matchesGoldenFile('goldens/$name.png'),
  );
}

void main() {
  setUpAll(() async {
    await loadTrampFonts();
    await MockupShell.ensureNoiseReady();
  });

  testWidgets('title bar strip', (tester) async {
    await _pumpGolden(
      tester,
      const MockupTitleBar(windowName: 'Main Player'),
      const Size(825, 42),
      'title_bar_strip',
    );
  });

  testWidgets('button off and on', (tester) async {
    await _pumpGolden(
      tester,
      Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          MockupButton(label: 'EQ', width: 74, height: 38, onPressed: () {}),
          const SizedBox(width: 12),
          MockupButton(
            label: 'EQ',
            on: true,
            width: 74,
            height: 38,
            onPressed: () {},
          ),
        ],
      ),
      const Size(200, 56),
      'button_off_on',
    );
  });

  testWidgets('slider at 66%', (tester) async {
    await _pumpGolden(
      tester,
      const Padding(
        padding: EdgeInsets.symmetric(horizontal: 16),
        child: Center(
          child: MockupSlider(value: 0.66),
        ),
      ),
      const Size(280, 40),
      'slider_66',
    );
  });

  testWidgets('LED lit and unlit', (tester) async {
    await _pumpGolden(
      tester,
      const Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          MockupLed(),
          SizedBox(width: 24),
          MockupLed(lit: true),
        ],
      ),
      const Size(80, 40),
      'led_lit_unlit',
    );
  });

  testWidgets('screen well with scanlines', (tester) async {
    await _pumpGolden(
      tester,
      const Padding(
        padding: EdgeInsets.all(12),
        child: MockupScreen(
          child: SizedBox(
            width: 200,
            height: 80,
            child: Center(
              child: Text(
                '2:41',
                style: TextStyle(
                  fontFamily: 'TrampMono',
                  fontWeight: FontWeight.w500,
                  fontSize: 28,
                  color: MockupTokens.phos,
                ),
              ),
            ),
          ),
        ),
      ),
      const Size(224, 104),
      'screen_well',
    );
  });

  testWidgets('shell with rivets plate rail', (tester) async {
    await _pumpGolden(
      tester,
      MockupShell(
        width: 320,
        child: SizedBox(
          height: 120,
          child: Stack(
            children: [
              const Positioned(
                left: 9,
                bottom: 8,
                child: MockupRivet(),
              ),
              const Positioned(
                right: 9,
                bottom: 8,
                child: MockupRivet(),
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(16, 16, 16, 24),
                child: Column(
                  children: [
                    Expanded(
                      child: MockupPlate(
                        child: const Center(
                          child: Text(
                            'PLATE',
                            style: TextStyle(
                              fontFamily: 'TrampCondensed',
                              fontWeight: FontWeight.w700,
                              fontSize: 13,
                              letterSpacing: 2.3,
                              color: MockupTokens.inkDim,
                            ),
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(height: 12),
                    const SizedBox(
                      width: double.infinity,
                      child: MockupRail(),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
      const Size(320, 120),
      'shell_plate_rail',
    );
  });

  testWidgets('transport icons strip', (tester) async {
    await _pumpGolden(
      tester,
      ColoredBox(
        color: MockupTokens.shellMid,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            MockupIcons.previous(),
            MockupIcons.play(color: MockupTokens.phosHot),
            MockupIcons.pause(),
            MockupIcons.stop(),
            MockupIcons.next(),
            MockupIcons.eject(),
            MockupIcons.mute(),
          ],
        ),
      ),
      const Size(280, 40),
      'transport_icons',
    );
  });
}
