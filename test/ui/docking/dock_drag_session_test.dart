import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/docking/dock_drag_session.dart';

void main() {
  group('DockDragSession', () {
    test('maps global pixel delta through zoom to logical top-left', () {
      final session = DockDragSession(
        originLogical: const Offset(100, 200),
        originGlobal: const Offset(400, 500),
        zoom: 2.0,
      );

      expect(
        session.logicalTopLeftFor(const Offset(440, 520)),
        const Offset(120, 210),
      );
    });

    test('at 100% zoom pixel delta equals logical delta', () {
      final session = DockDragSession(
        originLogical: Offset.zero,
        originGlobal: const Offset(10, 10),
        zoom: 1.0,
      );

      expect(
        session.logicalTopLeftFor(const Offset(58, 10)),
        const Offset(48, 0),
      );
    });
  });
}
