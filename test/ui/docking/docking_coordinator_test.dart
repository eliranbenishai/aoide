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

    test('moving docked main moves the whole group', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);

      c.move(WindowId.main, const Offset(40, 0), shiftUndock: false);

      expect(c.layout.main.left, 40);
      expect(c.layout.main.top, 0);
      expect(c.layout.playlist.left, 40);
      expect(c.layout.playlist.top, 348);
      expect(c.groupOf(WindowId.main), contains(WindowId.playlist));
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

    test('shiftUndock within 12px of partner does not re-snap', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.layout.dockEdges, isNotEmpty);

      // Still within snapThreshold of main.bottom, but shift must keep undocked.
      const undockedTop = 348.0 + 8;
      c.move(WindowId.playlist, const Offset(0, undockedTop), shiftUndock: true);

      expect(c.layout.playlist.left, 0);
      expect(c.layout.playlist.top, undockedTop);
      expect(c.layout.dockEdges, isEmpty);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
      expect(c.layout.main.left, 0);
      expect(c.layout.main.top, 0);
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

    test('reanchorForZoom keeps free window screen top-left', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      c.setVisible(WindowId.playlist, false);
      c.move(WindowId.main, const Offset(100, 80), shiftUndock: false);

      const fromZoom = 1.0;
      const toZoom = 2.0;
      final pixelBefore = c.frameFor(WindowId.main, fromZoom).topLeft;
      c.reanchorForZoom(fromZoom: fromZoom, toZoom: toZoom);
      final pixelAfter = c.frameFor(WindowId.main, toZoom).topLeft;

      expect(pixelAfter.dx, closeTo(pixelBefore.dx, 0.001));
      expect(pixelAfter.dy, closeTo(pixelBefore.dy, 0.001));
      expect(c.layout.main.left, closeTo(50, 0.001));
      expect(c.layout.main.top, closeTo(40, 0.001));
    });

    test('reanchorForZoom reseats docked playlist flush under main', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      c.move(WindowId.main, const Offset(40, 20), shiftUndock: false);
      c.move(WindowId.playlist, const Offset(40, 348 + 20 - 10), shiftUndock: false);
      expect(c.layout.playlist.top, 348 + 20);
      expect(c.groupOf(WindowId.playlist), contains(WindowId.main));

      const fromZoom = 1.0;
      const toZoom = 0.75;
      final mainPixel = c.frameFor(WindowId.main, fromZoom).topLeft;
      c.reanchorForZoom(fromZoom: fromZoom, toZoom: toZoom);

      final mainAfter = c.frameFor(WindowId.main, toZoom);
      expect(mainAfter.left, closeTo(mainPixel.dx, 0.001));
      expect(mainAfter.top, closeTo(mainPixel.dy, 0.001));
      // Still edge-docked and flush in logical space (no gap from size change).
      expect(c.groupOf(WindowId.playlist), contains(WindowId.main));
      expect(
        c.layout.playlist.top,
        closeTo(c.layout.main.top + TrampMetrics.mainPlayer.height, 0.001),
      );
      expect(c.layout.playlist.left, closeTo(c.layout.main.left, 0.001));
    });

    test('reanchorForZoom keeps EQ snapped to the right of main', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.playlist, false);
      c.move(WindowId.main, const Offset(10, 10), shiftUndock: false);
      c.move(
        WindowId.equalizer,
        Offset(TrampMetrics.mainPlayer.width + 10 - 8, 10),
        shiftUndock: false,
      );
      expect(c.layout.equalizer.left, TrampMetrics.mainPlayer.width + 10);
      expect(c.groupOf(WindowId.equalizer), contains(WindowId.main));

      c.reanchorForZoom(fromZoom: 1.0, toZoom: 2.0);

      expect(c.groupOf(WindowId.equalizer), contains(WindowId.main));
      expect(
        c.layout.equalizer.left,
        closeTo(c.layout.main.left + TrampMetrics.mainPlayer.width, 0.001),
      );
      expect(c.layout.equalizer.top, closeTo(c.layout.main.top, 0.001));
    });

    test('does not snap when farther than snapThreshold', () {
      final c = DockingCoordinator(DockLayout.defaults);
      final gap = 20.0 + 1;
      c.move(
        WindowId.playlist,
        Offset(0, 348 - gap),
        shiftUndock: false,
      );
      expect(c.layout.playlist.top, 348 - gap);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
    });

    test('snap:false defers edge snap until a later snap:true move', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(
        WindowId.playlist,
        const Offset(0, 348 - 10),
        shiftUndock: false,
        snap: false,
      );
      expect(c.layout.playlist.top, 348 - 10);
      expect(c.layout.dockEdges, isEmpty);

      c.move(
        WindowId.playlist,
        const Offset(0, 348 - 10),
        shiftUndock: false,
        snap: true,
      );
      expect(c.layout.playlist.top, 348);
      expect(c.groupOf(WindowId.playlist), contains(WindowId.main));
    });

    test('hiding a window drops its dock edges and leaves others free to snap',
        () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.equalizer, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.groupOf(WindowId.main), contains(WindowId.equalizer));

      c.setVisible(WindowId.equalizer, false);

      expect(c.layout.dockEdges, isEmpty);
      expect(c.groupOf(WindowId.main), equals({WindowId.main}));

      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.groupOf(WindowId.main), contains(WindowId.playlist));
    });

    test('mid-drag main move keeps sticky group (no false undock)', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.groupOf(WindowId.main), contains(WindowId.playlist));

      // Farther than undockSeparation mid-drag — main sticky still moves PL.
      c.move(
        WindowId.main,
        const Offset(0, 80),
        shiftUndock: false,
        snap: false,
      );
      expect(c.groupOf(WindowId.main), contains(WindowId.playlist));
      expect(c.layout.main.top, 80);
      expect(c.layout.playlist.top, 348 + 80);
    });

    test('dragging playlist peels it off the docked group', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.groupOf(WindowId.main), contains(WindowId.playlist));

      c.move(
        WindowId.playlist,
        const Offset(40, 500),
        shiftUndock: false,
        snap: false,
      );

      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
      expect(c.layout.dockEdges, isEmpty);
      expect(c.layout.main.left, 0);
      expect(c.layout.main.top, 0);
      expect(c.layout.playlist.left, 40);
      expect(c.layout.playlist.top, 500);
    });

    test('moving main translates all visible windows even when undocked', () {
      final c = DockingCoordinator(DockLayout.defaults);
      // Defaults: EQ + PL visible, no edges.
      expect(c.layout.dockEdges, isEmpty);
      final eqTop = c.layout.equalizer.top;
      final plTop = c.layout.playlist.top;

      c.move(WindowId.main, const Offset(50, 20), shiftUndock: false);

      expect(c.layout.main.left, 50);
      expect(c.layout.main.top, 20);
      expect(c.layout.equalizer.left, 50);
      expect(c.layout.equalizer.top, eqTop + 20);
      expect(c.layout.playlist.left, 50);
      expect(c.layout.playlist.top, plTop + 20);
      expect(c.moveCohortOf(WindowId.main), containsAll([
        WindowId.main,
        WindowId.equalizer,
        WindowId.playlist,
      ]));
    });

    test('moving main does not create snap edges', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.playlist, false);
      // Place EQ just below main within threshold; main move must not snap.
      c.move(
        WindowId.equalizer,
        const Offset(0, 348 + 40),
        shiftUndock: true,
      );
      c.move(WindowId.main, const Offset(0, 30), shiftUndock: false);

      expect(c.layout.dockEdges, isEmpty);
      expect(c.layout.equalizer.top, 348 + 40 + 30);
    });

    test('playlist does not snap to left/right side of main', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      // Approach main's right edge within threshold, vertically overlapping.
      c.move(
        WindowId.playlist,
        const Offset(825 - 10, 0),
        shiftUndock: false,
      );

      expect(c.layout.playlist.left, 825 - 10);
      expect(c.layout.dockEdges, isEmpty);
      expect(c.groupOf(WindowId.playlist), equals({WindowId.playlist}));
    });

    test('playlist top/bottom snap flushes left when within threshold', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      c.move(
        WindowId.playlist,
        const Offset(8, 348 - 10),
        shiftUndock: false,
      );

      expect(c.layout.playlist.top, 348);
      expect(c.layout.playlist.left, 0);
      expect(
        c.layout.dockEdges.any(
          (e) =>
              (e.a == WindowId.playlist || e.b == WindowId.playlist) &&
              (e.side == DockSide.left || e.side == DockSide.right),
        ),
        isTrue,
      );
    });

    test('playlist top/bottom snap keeps horizontal offset when far', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      const left = 20.0 + 5;
      c.move(
        WindowId.playlist,
        const Offset(left, 348 - 10),
        shiftUndock: false,
      );

      expect(c.layout.playlist.top, 348);
      expect(c.layout.playlist.left, left);
    });

    test('equalizer still snaps to the right of main', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.playlist, false);
      c.move(
        WindowId.equalizer,
        const Offset(825 - 10, 0),
        shiftUndock: false,
      );

      expect(c.layout.equalizer.left, 825);
      expect(c.groupOf(WindowId.equalizer), contains(WindowId.main));
    });

    test('hidden windows are excluded from main move cohort', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.equalizer, false);
      final plTop = c.layout.playlist.top;
      final eqLeft = c.layout.equalizer.left;
      final eqTop = c.layout.equalizer.top;

      c.move(WindowId.main, const Offset(30, 10), shiftUndock: false);

      expect(c.layout.playlist.left, 30);
      expect(c.layout.playlist.top, plTop + 10);
      expect(c.layout.equalizer.left, eqLeft);
      expect(c.layout.equalizer.top, eqTop);
      expect(c.moveCohortOf(WindowId.main), equals({WindowId.main, WindowId.playlist}));
    });

    test('settings is excluded from main move cohort even when visible', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.settings, true);
      final settingsLeft = c.layout.settings.left;
      final settingsTop = c.layout.settings.top;

      c.move(WindowId.main, const Offset(40, 20), shiftUndock: false);

      expect(c.layout.settings.left, settingsLeft);
      expect(c.layout.settings.top, settingsTop);
      expect(c.moveCohortOf(WindowId.main).contains(WindowId.settings), isFalse);
    });

    test('moving settings never snaps or peels partners', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.layout.dockEdges, isNotEmpty);
      final edgesBefore = c.layout.dockEdges;
      final plLeft = c.layout.playlist.left;

      c.move(WindowId.settings, const Offset(10, 10), shiftUndock: false);

      expect(c.layout.settings.left, 10);
      expect(c.layout.settings.top, 10);
      expect(c.layout.dockEdges, edgesBefore);
      expect(c.layout.playlist.left, plLeft);
      expect(c.groupOf(WindowId.settings), equals({WindowId.settings}));
    });

    test('settings is ignored as a snap target', () {
      final c = DockingCoordinator(DockLayout.defaults);
      c.setVisible(WindowId.settings, true);
      // Place settings far from main so only settings is nearby.
      c.move(WindowId.settings, const Offset(2000, 2000), shiftUndock: false);

      c.move(
        WindowId.playlist,
        const Offset(2000, 2000 - 8),
        shiftUndock: false,
      );

      expect(c.layout.playlist.top, 2000 - 8);
      expect(
        c.layout.dockEdges.any(
          (e) => e.a == WindowId.settings || e.b == WindowId.settings,
        ),
        isFalse,
      );
    });

    test('snapThreshold instance field gates snap distance', () {
      final c = DockingCoordinator(DockLayout.defaults, snapThreshold: 0);
      c.move(WindowId.playlist, const Offset(0, 348 - 10), shiftUndock: false);
      expect(c.layout.playlist.top, 348 - 10);
      expect(c.layout.dockEdges, isEmpty);
    });
  });
}
