import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/app.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/logo.dart';
import 'package:tramp/ui/chrome/proxima_logo.dart';
import 'package:tramp/ui/windows/about_window.dart';

import '../../support/test_fonts.dart';
import '../../support/look_harness.dart';

void main() {
  setUpAll(loadTrampFonts);

  Future<void> pumpAbout(
    WidgetTester tester, {
    ValueChanged<Uri>? onOpenUrl,
  }) {
    return tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: Center(
          child: AboutWindow(
            version: trampAppVersion,
            draggableTitle: false,
            onOpenUrl: onOpenUrl,
          ),
        ),
      ),
    );
  }

  testWidgets('About window fills its logical canvas', (tester) async {
    await pumpAbout(tester);
    expect(tester.getSize(find.byType(AboutWindow)), TrampMetrics.about);
  });

  testWidgets('About window shows brand, version, year, and company',
      (tester) async {
    await pumpAbout(tester);

    expect(find.byType(TrampLogo), findsOneWidget);
    expect(find.byType(ProximaMagnificaLogo), findsOneWidget);
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.text('Version $trampAppVersion'), findsOneWidget);
    expect(
      find.text('© $trampCopyrightYear $trampCompanyName'),
      findsOneWidget,
    );
    expect(find.text(trampCompanyName), findsNothing);
    expect(find.text(trampWebsiteUrl), findsOneWidget);
    expect(
      find.text(
        'A desktop music player — playlist-centric, with distinctive chrome.',
      ),
      findsOneWidget,
    );
    expect(find.text('ABOUT'), findsOneWidget);
  });

  testWidgets('website link reports the product URL', (tester) async {
    Uri? opened;
    await pumpAbout(tester, onOpenUrl: (uri) => opened = uri);

    await tester.tap(find.byKey(const Key('about-website')));
    await tester.pump();

    expect(opened, Uri.parse(trampWebsiteUrl));
  });
}
