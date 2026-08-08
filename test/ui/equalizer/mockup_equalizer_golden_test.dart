// Golden: mockup-faithful equalizer at 100% (825×348).
@Tags(['golden'])
library;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';
import 'package:tramp/ui/windows/equalizer_window.dart';

import '../../support/test_fonts.dart';

/// Gains approximating mockup EQ thumb positions in `player-mockup-2.html`.
EqualizerSettings get _mockupCurve => EqualizerSettings(
      enabled: true,
      auto: false,
      preamp: 3.8,
      gains: const [6.2, 4.6, 1.0, -1.9, -0.5, 2.2, 3.4, 1.4, 0.0, 5.0],
      presetName: 'Late Night',
    );

void main() {
  setUpAll(() async {
    await loadTrampFonts();
    await MockupShell.ensureNoiseReady();
  });

  testWidgets('equalizer window matches mockup layout at 100%', (tester) async {
    const size = Size(825, 348);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: EqualizerWindow(
              settings: _mockupCurve,
              draggableTitle: false,
            ),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await expectLater(
      find.byType(EqualizerWindow),
      matchesGoldenFile('goldens/equalizer_window.png'),
    );
  });

  testWidgets('equalizer shade matches title-bar chrome', (tester) async {
    const size = Size(825, 42);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: EqualizerWindow(
              settings: _mockupCurve,
              shaded: true,
              draggableTitle: false,
            ),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await expectLater(
      find.byType(EqualizerWindow),
      matchesGoldenFile('goldens/equalizer_window_shade.png'),
    );
  });
}
