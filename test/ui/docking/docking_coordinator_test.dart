import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/docking/dock_layout.dart';
import 'package:tramp/ui/docking/docking_coordinator.dart';

void main() {
  group('DockingCoordinator', () {
    test('snaps playlist below main when within 12px', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.layout.playlist.top, 348);
      expect(c.groupOf(WindowId.playlist), contains(WindowId.main));
    });

    test('moving a docked window moves the whole group', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);

      c.move(WindowId.playlist, const Offset(40, 348), shiftUndock: false);

      expect(c.layout.playlist.left, 40);
      expect(c.layout.playlist.top, 348);
      expect(c.layout.main.left, 40);
      expect(c.layout.main.top, 0);
      expect(c.groupOf(WindowId.playlist), contains(WindowId.main));
    });

    test('shiftUndock breaks edges and moves only the dragged window', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);

      c.move(WindowId.playlist, const Offset(100, 500), shiftUndock: true);

      expect(c.layout.playlist.left, 100);
      expect(c.layout.playlist.top, 500);
      expect(c.layout.main.left, 0);
      expect(c.layout.main.top, 0);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
    });

    test('separation over 48 undocks without moving partners', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);

      c.move(WindowId.playlist, const Offset(0, 348 + 49), shiftUndock: false);

      expect(c.layout.playlist.top, 348 + 49);
      expect(c.layout.main.left, 0);
      expect(c.layout.main.top, 0);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
    });

    test('shaded frame height collapses to titleBar', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setShaded(WindowId.playlist, true);

      final frame = c.frameFor(WindowId.playlist, 1.0);
      expect(frame.height, TrampMetrics.titleBar);
      expect(c.layout.playlist.shaded, isTrue);
    });

    test('docking snap uses shaded height', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, true);
      c.setShaded(WindowId.main, true);

      // Main shaded height is titleBar (42). Approach EQ below it within 12px.
      c.move(
        WindowId.equalizer,
        const Offset(0, TrampMetrics.titleBar - 8),
        shiftUndock: false,
      );

      expect(c.layout.equalizer.top, TrampMetrics.titleBar);
      expect(c.groupOf(WindowId.equalizer), contains(WindowId.main));
    });

    test('resizePlaylist keeps top-left', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(12, 40), shiftUndock: true);

      c.resizePlaylist(const Size(900, 500));

      expect(c.layout.playlist.left, 12);
      expect(c.layout.playlist.top, 40);
      expect(c.layout.playlist.width, 900);
      expect(c.layout.playlist.height, 500);
    });

    test('frameFor scales logical rect by zoom', () {
      final c = DockingCoordinator(DockLayout.defaults);
      final frame = c.frameFor(WindowId.main, 2.0);
      expect(
        frame,
        Rect.fromLTWH(
          0,
          0,
          TrampMetrics.mainPlayer.width * 2,
          TrampMetrics.mainPlayer.height * 2,
        ),
      );
    });

    test('does not snap when farther than 12px', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 13), shiftUndock: false);
      expect(c.layout.playlist.top, 348 - 13);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
    });
  });
}
