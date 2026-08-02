import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';
import 'package:tramp/ui/skin/skin_image.dart';

void main() {
  testWidgets('SkinImage draws one Image for mainFace', (WidgetTester tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: SkinImage(
          asset: GraphiteSkin.mainFace,
          logicalSize: const Size(812, 242),
        ),
      ),
    );
    await tester.pump();

    expect(find.byType(Image), findsOneWidget);
  });
}
