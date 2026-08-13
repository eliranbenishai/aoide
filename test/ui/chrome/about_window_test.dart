import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/app.dart';
import 'package:tramp/domain/about_stats.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/logo.dart';
import 'package:tramp/ui/chrome/proxima_logo.dart';
import 'package:tramp/ui/session/session_messages.dart';
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
    // The © line carries the pledge, not the company name again: the plate sets
    // that above in chrome type, and repeating it there read as a doubled logo.
    expect(
      find.text('© $trampCopyrightYear $trampFreePromise'),
      findsOneWidget,
    );
    expect(find.textContaining(trampCompanyName), findsNothing);
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

  testWidgets('the well shows zeros until a reading arrives', (tester) async {
    // The default must never be a plausible-looking figure: an About window
    // whose host has not spoken yet has counted nothing, and says so.
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: const Center(child: AboutWindow(draggableTitle: false)),
      ),
    );

    expect(find.text(AboutStats.placeholder.tracksLabel), findsNothing);
    expect(find.text('0 m'), findsOneWidget);
    expect(find.text('0'), findsNWidgets(3));
  });

  testWidgets('a measured reading replaces the zeros', (tester) async {
    await pumpAbout(
      tester,
      stats: const AboutStats(
        playlists: 4,
        tracks: 51,
        totalDuration: Duration(hours: 3, minutes: 12),
        spins: 906,
      ),
    );

    expect(find.text('4'), findsOneWidget);
    expect(find.text('51'), findsOneWidget);
    expect(find.text('3 h 12 m'), findsOneWidget);
    expect(find.text('906'), findsOneWidget);
  });

  testWidgets('a reading off the session bus paints the well', (tester) async {
    // The About window is a secondary engine, so its figures arrive as an
    // event. This is both ends of that contract meeting: what the host would
    // send, decoded the way the client decodes it, rendered in the well.
    const sent = AboutStatsEvent(
      playlists: 12,
      tracks: 1284,
      totalDurationMs: 340800000,
      spins: 4096,
    );
    final received =
        SessionEvent.fromJson(sent.toEnvelope()) as AboutStatsEvent;

    await pumpAbout(tester, stats: received.stats);

    expect(find.text('12'), findsOneWidget);
    expect(find.text('1,284'), findsOneWidget);
    expect(find.text('3 d 22 h'), findsOneWidget);
    expect(find.text('4,096'), findsOneWidget);
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
