import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/spectrum_visualizer.dart';

void main() {
  testWidgets('SpectrumVisualizer paints while playing', (tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: SpectrumVisualizer(playing: true, volume: 0.8),
      ),
    );
    expect(find.byType(CustomPaint), findsWidgets);
  });
}
