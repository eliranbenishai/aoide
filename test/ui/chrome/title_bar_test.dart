import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/title_bar.dart';

import '../../support/test_fonts.dart';

Widget host(Widget child) => Directionality(
      textDirection: TextDirection.ltr,
      child: Align(
        alignment: Alignment.topLeft,
        child: SizedBox(width: 812, child: child),
      ),
    );

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('renders the title in caps at the documented height',
      (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP')));
    expect(find.text('TRAMP'), findsOneWidget);
    expect(
      tester.getSize(find.byType(TrampTitleBar)).height,
      TrampMetrics.titleBar,
    );
  });

  testWidgets('never renders the Winamp brand', (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP EQUALIZER')));
    expect(find.textContaining('WINAMP', findRichText: true), findsNothing);
    expect(find.text('TRAMP EQUALIZER'), findsOneWidget);
  });

  testWidgets('rails paint in the rail accent', (tester) async {
    await tester.pumpWidget(host(const TrampTitleBar(title: 'TRAMP')));
    final painters = tester
        .widgetList<CustomPaint>(find.byType(CustomPaint))
        .map((w) => w.painter)
        .whereType<RailPainter>()
        .toList();
    expect(painters, hasLength(2), reason: 'one rail either side of the title');
    expect(painters.first.colour, TrampColors.railAccent);
  });

  testWidgets('leading and trailing slots are placed', (tester) async {
    await tester.pumpWidget(host(
      TrampTitleBar(
        title: 'TRAMP',
        leading: const SizedBox(key: Key('lead'), width: 27, height: 27),
        trailing: const [
          SizedBox(key: Key('t1'), width: 27, height: 27),
          SizedBox(key: Key('t2'), width: 27, height: 27),
        ],
      ),
    ));
    expect(find.byKey(const Key('lead')), findsOneWidget);
    expect(find.byKey(const Key('t1')), findsOneWidget);
    expect(find.byKey(const Key('t2')), findsOneWidget);

    final lead = tester.getCenter(find.byKey(const Key('lead')));
    final title = tester.getCenter(find.text('TRAMP'));
    final trail = tester.getCenter(find.byKey(const Key('t2')));
    expect(lead.dx, lessThan(title.dx));
    expect(trail.dx, greaterThan(title.dx));
  });

  testWidgets('the drag region can be suppressed for tests and goldens',
      (tester) async {
    await tester.pumpWidget(
      host(const TrampTitleBar(title: 'TRAMP', draggable: false)),
    );
    expect(tester.takeException(), isNull);
  });
}
