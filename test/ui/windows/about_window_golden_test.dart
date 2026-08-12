// Golden: About window at 100% (480×360) with the placeholder usage stats.
@Tags(['golden'])
library;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';
import 'package:tramp/ui/windows/about_window.dart';

import '../../support/look_harness.dart';
import '../../support/test_fonts.dart';

void main() {
  setUpAll(() async {
    await loadTrampFonts();
    await MockupShell.ensureNoiseReady();
  });

  testWidgets('about window matches the credits layout at 100%',
      (tester) async {
    await tester.binding.setSurfaceSize(TrampMetrics.about);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: wrapWithLook(const AboutWindow(draggableTitle: false)),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await expectLater(
      find.byType(AboutWindow),
      matchesGoldenFile('goldens/about_window.png'),
    );
  });
}
