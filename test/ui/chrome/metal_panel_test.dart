import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/theme/tramp_surfaces.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

BoxDecoration decorationOf(WidgetTester tester) {
  final box = tester.widget<DecoratedBox>(find.byType(DecoratedBox));
  return box.decoration as BoxDecoration;
}

BevelPainter bevelOf(WidgetTester tester) {
  final paint = tester.widget<CustomPaint>(find.byType(CustomPaint).first);
  return paint.foregroundPainter! as BevelPainter;
}

void main() {
  testWidgets('raised panel uses the panel gradient', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    final gradient = decorationOf(tester).gradient! as LinearGradient;
    expect(gradient.colors.first, TrampColors.panelTop);
  });

  testWidgets('lcd glass surface fills with the glass colour', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.lcdGlass,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(decorationOf(tester).color, TrampColors.lcdGlass);
  });

  testWidgets('bevel width is snapped from the ambient zoom', (tester) async {
    await tester.pumpWidget(
      const ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(bevelOf(tester).spec.bevel, closeTo(2 / 1.5, 1e-9));
  });

  testWidgets('renders and paints without a ZoomScope ancestor',
      (tester) async {
    await tester.pumpWidget(
      const Center(
        child: MetalPanel(
          surface: TrampSurface.raisedButton,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(tester.takeException(), isNull);
    expect(bevelOf(tester).spec.bevel, ZoomScope.hairline);
  });

  testWidgets('the fill carries no border so it can be rounded and painted',
      (tester) async {
    await tester.pumpWidget(
      const Center(
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: SizedBox(width: 10, height: 10),
        ),
      ),
    );
    expect(decorationOf(tester).border, isNull);
    expect(decorationOf(tester).borderRadius, isNotNull);
    expect(tester.takeException(), isNull);
  });

  testWidgets('padding wraps the child when supplied', (tester) async {
    await tester.pumpWidget(
      const MetalPanel(
        surface: TrampSurface.raisedPanel,
        padding: EdgeInsets.all(4),
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(find.byType(Padding), findsOneWidget);
  });
}
