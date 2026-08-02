import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';
import 'package:tramp/ui/skin/skin_button.dart';
import 'package:tramp/ui/skin/skin_image.dart';

/// The asset an active [SkinImage] is currently painting.
String _shownAsset(WidgetTester tester) {
  final image = tester.widget<Image>(find.byType(Image));
  return (image.image as AssetImage).assetName;
}

void main() {
  testWidgets('SkinButton calls onPressed', (tester) async {
    var taps = 0;
    await tester.pumpWidget(
      MaterialApp(
        home: SkinButton(
          size: const Size(69, 40),
          idleAsset: GraphiteSkin.transportPlayIdle,
          pressedAsset: GraphiteSkin.transportPlayPressed,
          onPressed: () => taps++,
          semanticLabel: 'Play',
        ),
      ),
    );

    await tester.tap(find.byType(SkinButton));
    expect(taps, 1);
  });

  testWidgets('SkinButton swaps to the pressed sprite while held', (
    tester,
  ) async {
    await tester.pumpWidget(
      MaterialApp(
        home: SkinButton(
          size: const Size(69, 40),
          idleAsset: GraphiteSkin.transportPlayIdle,
          pressedAsset: GraphiteSkin.transportPlayPressed,
          onPressed: () {},
          semanticLabel: 'Play',
        ),
      ),
    );

    expect(_shownAsset(tester), GraphiteSkin.transportPlayIdle);

    final gesture = await tester.startGesture(
      tester.getCenter(find.byType(SkinButton)),
    );
    await tester.pump();
    expect(_shownAsset(tester), GraphiteSkin.transportPlayPressed);

    await gesture.up();
    await tester.pump();
    expect(_shownAsset(tester), GraphiteSkin.transportPlayIdle);
  });

  testWidgets('SkinButton shows the active sprite when active', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: SkinButton(
          size: const Size(69, 40),
          idleAsset: GraphiteSkin.transportPlayIdle,
          activeAsset: GraphiteSkin.transportPlayPressed,
          active: true,
          onPressed: () {},
          semanticLabel: 'Play',
        ),
      ),
    );

    expect(_shownAsset(tester), GraphiteSkin.transportPlayPressed);
  });

  testWidgets('SkinButton exposes a button semantics node', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: SkinButton(
          size: const Size(69, 40),
          idleAsset: GraphiteSkin.transportPlayIdle,
          onPressed: () {},
          semanticLabel: 'Play',
        ),
      ),
    );

    expect(
      tester.getSemantics(find.byType(SkinButton)),
      matchesSemantics(
        label: 'Play',
        isButton: true,
        hasEnabledState: true,
        isEnabled: true,
        isImage: true,
        hasTapAction: true,
      ),
    );
  });
}
