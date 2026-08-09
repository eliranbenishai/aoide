import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_title_bar.dart';

import '../../../support/test_fonts.dart';

Rect _localRect(WidgetTester tester, Finder finder, Finder ancestor) {
  final target = tester.renderObject<RenderBox>(finder);
  final root = tester.renderObject<RenderBox>(ancestor);
  final topLeft = target.localToGlobal(Offset.zero, ancestor: root);
  return topLeft & target.size;
}

void main() {
  setUpAll(loadTrampFonts);

  Future<void> pumpBar(WidgetTester tester, Widget bar) async {
    await tester.binding.setSurfaceSize(const Size(825, 42));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: SizedBox(
            width: 825,
            height: 42,
            child: bar,
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();
  }

  testWidgets('Main and compact EQ/PL title chrome share the bar mid-line',
      (tester) async {
    Future<({double nameCy, double btnCy})> centersFor(Widget bar) async {
      await pumpBar(tester, bar);
      final root = find.byType(MockupTitleBar);
      final midY = tester.getRect(root).center.dy;

      final name = find.descendant(of: root, matching: find.byType(Text)).last;
      final nameCy = _localRect(tester, name, root).center.dy;
      final btnCy = _localRect(
        tester,
        find.bySemanticsLabel('Close'),
        root,
      ).center.dy;

      expect(nameCy, closeTo(midY, 0.6),
          reason: 'window name must sit on the 42px mid-line');
      expect(btnCy, closeTo(midY, 0.6),
          reason: 'window buttons must sit on the 42px mid-line');
      expect(nameCy, closeTo(btnCy, 0.6),
          reason: 'name and buttons must share one vertical center');
      return (nameCy: nameCy, btnCy: btnCy);
    }

    final main = await centersFor(
      const MockupTitleBar(windowName: 'Main Player'),
    );
    final eq = await centersFor(
      const MockupTitleBar(
        windowName: 'Equalizer',
        showBrand: false,
        showZoom: false,
      ),
    );
    final pl = await centersFor(
      const MockupTitleBar(
        windowName: 'Playlist Editor',
        showBrand: false,
        showZoom: false,
      ),
    );

    expect(eq.btnCy, closeTo(main.btnCy, 0.6),
        reason: 'EQ buttons must match Main vertical center');
    expect(pl.btnCy, closeTo(main.btnCy, 0.6),
        reason: 'PL buttons must match Main vertical center');
  });
}
