import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/logo.dart';

void main() {
  testWidgets('TrampLogo renders the SVG artwork', (tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: Scaffold(
          body: Center(child: TrampLogo()),
        ),
      ),
    );
    expect(find.byType(SvgPicture), findsOneWidget);
  });

  testWidgets('logo asset parses into a picture', (tester) async {
    await tester.runAsync(() async {
      final loader = SvgAssetLoader(trampLogoAsset);
      final info = await vg.loadPicture(loader, null);
      addTearDown(info.picture.dispose);
      expect(info.size.width, greaterThan(0));
      expect(info.size.height, greaterThan(0));
    });
  });
}
