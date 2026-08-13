import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_button.dart';
import 'package:tramp/ui/chrome/mockup/mockup_title_bar.dart';

import '../../../support/look_harness.dart';
import '../../../support/test_fonts.dart';

/// A host with an Overlay, which a tooltip needs somewhere to float.
Widget overlayHost(Widget child) => MaterialApp(
      home: Scaffold(body: Center(child: wrapWithLook(child))),
    );

/// Parks a mouse over [finder] and waits out the hover delay.
Future<void> hoverOver(WidgetTester tester, Finder finder) async {
  final gesture = await tester.createGesture(kind: PointerDeviceKind.mouse);
  await gesture.addPointer(location: Offset.zero);
  addTearDown(gesture.removePointer);
  await tester.pump();
  await gesture.moveTo(tester.getCenter(finder));
  await tester.pump();
  await tester.pump(const Duration(seconds: 1));
}

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('hovering a glyph button names what it does', (tester) async {
    await tester.pumpWidget(
      overlayHost(
        MockupButton(
          key: const Key('probe'),
          semanticLabel: 'Add tracks',
          onPressed: () {},
          child: const SizedBox(width: 20, height: 20),
        ),
      ),
    );

    expect(find.text('Add tracks'), findsNothing);
    await hoverOver(tester, find.byKey(const Key('probe')));
    expect(find.text('Add tracks'), findsOneWidget);
  });

  testWidgets('an explicit tooltip outranks an abbreviated face', (tester) async {
    // The EQ / PL / Mono case: the face is too terse to explain itself.
    await tester.pumpWidget(
      overlayHost(
        MockupButton(
          key: const Key('probe'),
          label: 'EQ',
          tooltip: 'Show equalizer',
          onPressed: () {},
        ),
      ),
    );

    await hoverOver(tester, find.byKey(const Key('probe')));
    expect(find.text('Show equalizer'), findsOneWidget);
  });

  testWidgets('a labelled button falls back to its own face', (tester) async {
    await tester.pumpWidget(
      overlayHost(
        MockupButton(
          key: const Key('probe'),
          label: 'Mono',
          onPressed: () {},
        ),
      ),
    );

    await hoverOver(tester, find.byKey(const Key('probe')));
    // The face renders uppercased; the tip is the label as written.
    expect(find.text('Mono'), findsOneWidget);
  });

  testWidgets('a disabled button still says what it would do', (tester) async {
    await tester.pumpWidget(
      overlayHost(
        MockupButton(
          key: const Key('probe'),
          semanticLabel: 'Remove selected tracks',
          onPressed: null,
          child: const SizedBox(width: 20, height: 20),
        ),
      ),
    );

    await hoverOver(tester, find.byKey(const Key('probe')));
    expect(find.text('Remove selected tracks'), findsOneWidget);
  });

  testWidgets('title bar window buttons name themselves', (tester) async {
    await tester.pumpWidget(
      overlayHost(
        SizedBox(
          width: 400,
          child: MockupTitleBar(
            windowName: 'Playlist Manager',
            showBrand: false,
            showZoom: false,
            onClose: () {},
          ),
        ),
      ),
    );

    await hoverOver(tester, find.bySemanticsLabel('Close'));
    expect(find.text('Close'), findsOneWidget);
  });

  testWidgets('a control with nothing to say renders no tooltip', (tester) async {
    await tester.pumpWidget(
      overlayHost(
        MockupButton(
          key: const Key('probe'),
          semanticLabel: '   ',
          onPressed: () {},
          child: const SizedBox(width: 20, height: 20),
        ),
      ),
    );

    await hoverOver(tester, find.byKey(const Key('probe')));
    expect(find.byType(Tooltip), findsNothing);
  });

  testWidgets('chrome still renders with no Overlay to float in', (tester) async {
    // Goldens and isolated widget tests pump chrome bare. A tooltip must never
    // become a condition of a button rendering at all.
    await tester.pumpWidget(
      lookHost(
        MockupButton(
          key: const Key('probe'),
          semanticLabel: 'Add tracks',
          onPressed: () {},
          child: const SizedBox(width: 20, height: 20),
        ),
      ),
    );

    expect(tester.takeException(), isNull);
    expect(find.byKey(const Key('probe')), findsOneWidget);
  });
}
