import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/ui/docking/dock_drag_area.dart';
import 'package:tramp/ui/docking/dock_layout.dart';
import 'package:tramp/ui/docking/docking_coordinator.dart';

void main() {
  group('DockDragArea', () {
    testWidgets('pan drives logical topLeft through zoom into coordinator',
        (tester) async {
      final docking = DockingCoordinator(DockLayout.defaults);
      Offset? last;
      var endedCount = 0;

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: DockDragArea(
              zoom: 2.0,
              logicalTopLeft: () => Offset(
                docking.layout.main.left,
                docking.layout.main.top,
              ),
              onMove: (topLeft, {required shiftUndock, required ended}) {
                last = topLeft;
                docking.move(
                  WindowId.main,
                  topLeft,
                  shiftUndock: shiftUndock,
                );
                if (ended) endedCount++;
              },
              child: const SizedBox(
                width: 200,
                height: 42,
                child: Text('title'),
              ),
            ),
          ),
        ),
      );

      final center = tester.getCenter(find.text('title'));
      final gesture = await tester.startGesture(center);
      // 40px screen → 20 logical at 2× zoom.
      await gesture.moveBy(const Offset(40, 0));
      await tester.pump();

      expect(last, const Offset(20, 0));
      expect(docking.layout.main.left, 20);

      await gesture.up();
      await tester.pump();
      expect(endedCount, 1);
    });

    testWidgets('Shift while dragging sets shiftUndock', (tester) async {
      bool? sawShift;

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: DockDragArea(
              zoom: 1.0,
              logicalTopLeft: () => Offset.zero,
              onMove: (topLeft, {required shiftUndock, required ended}) {
                sawShift = shiftUndock;
              },
              child: const SizedBox(
                width: 200,
                height: 42,
                child: Text('title'),
              ),
            ),
          ),
        ),
      );

      await tester.sendKeyDownEvent(LogicalKeyboardKey.shiftLeft);
      final center = tester.getCenter(find.text('title'));
      final gesture = await tester.startGesture(center);
      await gesture.moveBy(const Offset(10, 0));
      await tester.pump();
      await gesture.up();
      await tester.sendKeyUpEvent(LogicalKeyboardKey.shiftLeft);

      expect(sawShift, isTrue);
    });

    testWidgets('group drag: moving docked main moves playlist via coordinator',
        (tester) async {
      final docking = DockingCoordinator(DockLayout.defaults);
      // Snap playlist under main first.
      docking.move(
        WindowId.playlist,
        const Offset(0, 348 - 10),
        shiftUndock: false,
      );
      expect(docking.groupOf(WindowId.main), contains(WindowId.playlist));

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: DockDragArea(
              zoom: 1.0,
              logicalTopLeft: () => Offset(
                docking.layout.main.left,
                docking.layout.main.top,
              ),
              onMove: (topLeft, {required shiftUndock, required ended}) {
                docking.move(
                  WindowId.main,
                  topLeft,
                  shiftUndock: shiftUndock,
                );
              },
              child: const SizedBox(
                width: 200,
                height: 42,
                child: Text('title'),
              ),
            ),
          ),
        ),
      );

      final center = tester.getCenter(find.text('title'));
      final gesture = await tester.startGesture(center);
      await gesture.moveBy(const Offset(30, 0));
      await tester.pump();
      await gesture.up();

      expect(docking.layout.main.left, 30);
      expect(docking.layout.playlist.left, 30);
      expect(docking.layout.playlist.top, 348);
    });
  });
}
