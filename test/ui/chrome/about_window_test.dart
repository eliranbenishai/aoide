import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/logo.dart';
import 'package:tramp/ui/windows/about_window.dart';

import '../../support/test_fonts.dart';
import '../../support/look_harness.dart';

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('About window shows logo and version', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: const AboutWindow(
          version: '0.1.0',
          draggableTitle: false,
        ),
      ),
    );

    expect(find.byType(TrampLogo), findsOneWidget);
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.text('Version 0.1.0'), findsOneWidget);
    expect(find.text('ABOUT'), findsOneWidget);
  });
}
