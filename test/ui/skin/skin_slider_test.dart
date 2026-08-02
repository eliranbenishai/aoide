import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';
import 'package:tramp/ui/skin/skin_slider.dart';

void main() {
  testWidgets('SkinSlider drag updates value', (tester) async {
    double? v;
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 175,
            height: 12,
            child: SkinSlider(
              axis: Axis.horizontal,
              value: 0.25,
              trackSize: const Size(175, 12),
              thumbAsset: GraphiteSkin.sliderThumb,
              onChanged: (x) => v = x,
            ),
          ),
        ),
      ),
    );

    await tester.drag(find.byType(SkinSlider), const Offset(80, 0));
    expect(v, isNotNull);
    expect(v! > 0.25, isTrue);
  });

  testWidgets('SkinSlider vertical drag up raises value', (tester) async {
    double? v;
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 12,
            height: 175,
            child: SkinSlider(
              axis: Axis.vertical,
              value: 0.25,
              trackSize: const Size(12, 175),
              thumbAsset: GraphiteSkin.sliderThumb,
              onChanged: (x) => v = x,
            ),
          ),
        ),
      ),
    );

    // Screen y grows downward, so dragging up (negative dy) raises the value.
    await tester.drag(find.byType(SkinSlider), const Offset(0, -80));
    expect(v, isNotNull);
    expect(v! > 0.25, isTrue);
  });

  testWidgets('SkinSlider reports the final value on drag end', (tester) async {
    double? ended;
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 175,
            height: 12,
            child: SkinSlider(
              axis: Axis.horizontal,
              value: 0.25,
              trackSize: const Size(175, 12),
              thumbAsset: GraphiteSkin.sliderThumb,
              onChangeEnd: (x) => ended = x,
            ),
          ),
        ),
      ),
    );

    await tester.drag(find.byType(SkinSlider), const Offset(60, 0));
    await tester.pump();
    expect(ended, isNotNull);
  });

  testWidgets('SkinSlider paints the thumb image', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SkinSlider(
            axis: Axis.horizontal,
            value: 0.5,
            trackSize: const Size(175, 12),
            thumbAsset: GraphiteSkin.sliderThumb,
          ),
        ),
      ),
    );

    final image = tester.widget<Image>(find.byType(Image));
    expect((image.image as AssetImage).assetName, GraphiteSkin.sliderThumb);
  });
}
