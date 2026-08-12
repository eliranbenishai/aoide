import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/app.dart';
import 'package:tramp/domain/about_stats.dart';
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
    AboutStats? stats,
  }) {
    return tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: Center(
          child: AboutWindow(
            version: trampAppVersion,
            stats: stats ?? AboutStats.placeholder,
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
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.bySemanticsLabel(trampBackronym), findsOneWidget);
    expect(find.text('V $trampAppVersion'), findsOneWidget);
    expect(find.text(trampTagline), findsOneWidget);
    expect(find.text('ABOUT'), findsOneWidget);
  });

  testWidgets('company credit sits on the maker plate', (tester) async {
    await pumpAbout(tester);

    // The lockup's own type is illegible at plate size, so the plate pairs the
    // device mark with the name in chrome type.
    expect(find.byType(ProximaMagnificaMark), findsOneWidget);
    expect(find.byType(ProximaMagnificaLogo), findsNothing);
    expect(find.text(trampCompanyName.toUpperCase()), findsOneWidget);
    expect(
      find.text('© $trampCopyrightYear $trampCompanyName'),
      findsOneWidget,
    );
  });

  testWidgets('stats well reads out the usage counters', (tester) async {
    await pumpAbout(
      tester,
      stats: const AboutStats(
        playlists: 3,
        tracks: 2048,
        totalDuration: Duration(hours: 5, minutes: 30),
        spins: 17,
      ),
    );

    expect(find.text('PLAYLISTS'), findsOneWidget);
    expect(find.text('3'), findsOneWidget);
    expect(find.text('TRACKS'), findsOneWidget);
    expect(find.text('2,048'), findsOneWidget);
    expect(find.text('TOTAL TIME'), findsOneWidget);
    expect(find.text('5 h 30 m'), findsOneWidget);
    expect(find.text('SPINS'), findsOneWidget);
    expect(find.text('17'), findsOneWidget);
  });

  testWidgets('stats default to the placeholder figures', (tester) async {
    await pumpAbout(tester);

    expect(find.text(AboutStats.placeholder.tracksLabel), findsOneWidget);
    expect(find.text(AboutStats.placeholder.totalTimeLabel), findsOneWidget);
  });

  testWidgets('website chip shows the host and reports the product URL',
      (tester) async {
    Uri? opened;
    await pumpAbout(tester, onOpenUrl: (uri) => opened = uri);

    expect(find.text('tramp.music'), findsOneWidget);

    await tester.tap(find.byKey(const Key('about-website')));
    await tester.pump();

    expect(opened, Uri.parse(trampWebsiteUrl));
  });

  testWidgets('shaded About window collapses to the title bar', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: const Center(
          child: AboutWindow(shaded: true, draggableTitle: false),
        ),
      ),
    );

    expect(
      tester.getSize(find.byType(AboutWindow)),
      Size(TrampMetrics.about.width, TrampMetrics.titleBar),
    );
    expect(find.text('PLAYLISTS'), findsNothing);
  });
}
