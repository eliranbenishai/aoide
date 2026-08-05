import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/about_dialog.dart';
import 'package:tramp/ui/chrome/logo.dart';

import '../../support/test_fonts.dart';

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('About dialog shows logo and version', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Builder(
          builder: (context) {
            return TextButton(
              key: const Key('open-about'),
              onPressed: () => showTrampAboutDialog(context, version: '0.1.0'),
              child: const Text('open'),
            );
          },
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('open-about')));
    await tester.pumpAndSettle();

    expect(find.byType(TrampLogo), findsOneWidget);
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.text('Version 0.1.0'), findsOneWidget);
    expect(find.byKey(const Key('about-close')), findsOneWidget);
  });
}
