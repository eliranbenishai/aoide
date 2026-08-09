import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/window_resize_grip.dart';
import 'package:window_manager/window_manager.dart';
import '../../support/look_harness.dart';

void main() {
  testWidgets('pan start invokes startResizing on bottomRight', (tester) async {
    ResizeEdge? seen;
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) => wrapWithLook(child ?? const SizedBox.shrink()),
        home: Scaffold(
          body: Center(
            child: WindowResizeGrip(
              startResizing: (edge) async {
                seen = edge;
              },
            ),
          ),
        ),
      ),
    );

    await tester.drag(find.byType(WindowResizeGrip), const Offset(-20, -20));
    expect(seen, ResizeEdge.bottomRight);
  });

  testWidgets('disabled grip is empty', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: const Scaffold(
          body: WindowResizeGrip(enabled: false),
        ),
      ),
    );
    expect(tester.getSize(find.byType(WindowResizeGrip)), Size.zero);
  });
}
