import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/tramp_shell.dart';
import 'package:tramp/ui/window_layout.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';
import 'package:window_manager/window_manager.dart';

class _MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  group('eqModeWindowSize', () {
    test('is the exact EQ stack plus frame at 100%', () {
      // 825+12 wide; 12 + 348 + 6 + 348 tall.
      expect(eqModeWindowSize(1), const Size(837, 714));
    });

    test('scales with the zoom factor', () {
      expect(eqModeWindowSize(2), const Size(1674, 1428));
      expect(eqModeWindowSize(1.5), const Size(1255.5, 1071));
    });
  });

  group('eqModeWindowSize (collapsed)', () {
    test('snaps the lower region to the title bar when collapsed', () {
      // 825+12 wide; 12 + 348 + 6 + 42 (titleBar) tall.
      expect(eqModeWindowSize(1, collapsed: true), const Size(837, 408));
    });

    test('scales the collapsed stack with the zoom factor', () {
      expect(eqModeWindowSize(2, collapsed: true), const Size(1674, 816));
    });
  });

  group('playlistModeMinimumSize', () {
    test('fits the main player plus the minimum well at 100%', () {
      // 12 + 348 + 6 + 240 tall.
      expect(playlistModeMinimumSize(1), const Size(837, 606));
    });

    test('scales with the zoom factor', () {
      expect(playlistModeMinimumSize(2), const Size(1674, 1212));
    });
  });

  group('defaultPlaylistModeWindowSize', () {
    test('scales main width and tall well height', () {
      // 12 + 348 + 6 + 400 tall.
      expect(defaultPlaylistModeWindowSize(1), const Size(837, 766));
      expect(defaultPlaylistModeWindowSize(2), const Size(1674, 1532));
    });
  });

  group('playlistModeWindowSize', () {
    test('falls back to the default when nothing is stored', () {
      expect(
        playlistModeWindowSize(factor: 1),
        const Size(837, 766),
      );
    });

    test('falls back to the default when only one dimension is stored', () {
      expect(
        playlistModeWindowSize(factor: 1, storedWidth: 900),
        const Size(837, 766),
      );
      expect(
        playlistModeWindowSize(factor: 1, storedHeight: 700),
        const Size(837, 766),
      );
    });

    test('scales the stored logical size by the factor', () {
      expect(
        playlistModeWindowSize(factor: 1, storedWidth: 900, storedHeight: 700),
        const Size(900, 700),
      );
      expect(
        playlistModeWindowSize(factor: 2, storedWidth: 900, storedHeight: 700),
        const Size(1800, 1400),
      );
    });

    test('clamps a stored size up to the mode minimum', () {
      expect(
        playlistModeWindowSize(factor: 1, storedWidth: 400, storedHeight: 300),
        const Size(837, 606),
      );
      expect(
        playlistModeWindowSize(factor: 2, storedWidth: 900, storedHeight: 300),
        const Size(1800, 1212),
      );
    });
  });

  group('logicalPlaylistWindowSize', () {
    test('divides the live window size by the factor', () {
      expect(
        logicalPlaylistWindowSize(const Size(1800, 1400), 2),
        const Size(900, 700),
      );
    });

    test('round-trips through playlistModeWindowSize', () {
      final logical = logicalPlaylistWindowSize(const Size(1674, 1532), 2);
      expect(
        playlistModeWindowSize(
          factor: 2,
          storedWidth: logical.width,
          storedHeight: logical.height,
        ),
        const Size(1674, 1532),
      );
    });
  });

  group('windowModeTarget', () {
    test('equalizer mode is fixed at the EQ stack and not resizable', () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.equalizer,
        factor: 2,
        storedPlaylistWidth: 900,
        storedPlaylistHeight: 700,
      );
      expect(target.size, const Size(1674, 1428));
      expect(target.minimumSize, const Size(1674, 1428));
      expect(target.resizable, isFalse);
    });

    test('playlist mode is resizable with the stored size and mode minimum',
        () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
        storedPlaylistWidth: 900,
        storedPlaylistHeight: 700,
      );
      expect(target.size, const Size(900, 700));
      expect(target.minimumSize, const Size(837, 606));
      expect(target.resizable, isTrue);
    });

    test('playlist mode without a stored size uses the default tall size', () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
      );
      expect(target.size, const Size(837, 766));
      expect(target.minimumSize, const Size(837, 606));
      expect(target.resizable, isTrue);
    });

    test('collapsed equalizer mode snaps to the shade stack', () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.equalizer,
        factor: 1,
        equalizerCollapsed: true,
      );
      expect(target.size, const Size(837, 408));
      expect(target.minimumSize, const Size(837, 408));
      expect(target.resizable, isFalse);
    });
  });

  group('panelStackLayout', () {
    test('equalizer mode is always the exact stack', () {
      const layout = PanelStackLayout(
        logicalWidth: 825,
        lowerHeight: 348,
      );
      expect(layout.logicalHeight, 702);
      expect(layout.hostSize(2), const Size(1650, 1404));

      final eq = panelStackLayout(
        lowerRegion: LowerRegion.equalizer,
        factor: 2,
        contentSize: const Size(5000, 3000),
      );
      expect(eq.logicalWidth, 825);
      expect(eq.lowerHeight, 348);
    });

    test('collapsed equalizer mode drops the lower region to the title bar',
        () {
      final eq = panelStackLayout(
        lowerRegion: LowerRegion.equalizer,
        factor: 2,
        contentSize: const Size(5000, 3000),
        equalizerCollapsed: true,
      );
      expect(eq.logicalWidth, 825);
      expect(eq.lowerHeight, 42);
      expect(eq.logicalHeight, 396);
    });

    test('playlist mode fills the content area', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
        contentSize: const Size(1200, 900),
      );
      expect(layout.logicalWidth, 1200);
      // 900 - 348 - 6 left for the well.
      expect(layout.lowerHeight, 546);
      expect(layout.hostSize(1), const Size(1200, 900));
    });

    test('playlist mode never shrinks below the fixed canvases', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
        contentSize: const Size(400, 300),
      );
      expect(layout.logicalWidth, 825);
      expect(layout.lowerHeight, 240);
    });

    test('playlist mode divides the content area by the factor', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 2,
        contentSize: const Size(1648, 1000),
      );
      expect(layout.logicalWidth, 825);
      // max(240, (1000 - (348+6)*2) / 2) = 240.
      expect(layout.lowerHeight, 240);
    });
  });

  group('TrampShell window modes', () {
    late PlaylistController playlist;
    late PlaybackController playback;
    late ZoomController zoom;

    setUp(() {
      playlist = PlaylistController(store: _MemoryStore());
      playback = PlaybackController(
        playlist: playlist,
        engine: FakePlayerEngine(),
      );
      zoom = ZoomController(workArea: const Size(8000, 6000));
    });

    tearDown(() async => playback.dispose());

    Future<void> pump(
      WidgetTester tester, {
      required LowerRegion region,
      required Size surface,
    }) async {
      await tester.binding.setSurfaceSize(surface);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(
        MaterialApp(
          home: TrampShell(
            zoom: zoom,
            lowerRegion: region,
            playback: playback,
            playlistController: playlist,
            mainPlayer: SizedBox(
              key: const Key('main-player'),
              width: TrampMetrics.mainPlayer.width,
              height: TrampMetrics.mainPlayer.height,
            ),
            equalizer: SizedBox(
              key: const Key('equalizer'),
              width: TrampMetrics.equalizer.width,
              height: TrampMetrics.equalizer.height,
            ),
            playlist: const SizedBox(key: Key('playlist')),
          ),
        ),
      );
    }

    testWidgets('playlist mode enables resize edges', (tester) async {
      await pump(
        tester,
        region: LowerRegion.playlist,
        surface: const Size(837, 766),
      );
      expect(find.byType(DragToResizeArea), findsOneWidget);
    });

    testWidgets('equalizer mode has no resize edges', (tester) async {
      await pump(
        tester,
        region: LowerRegion.equalizer,
        surface: const Size(837, 714),
      );
      expect(find.byType(DragToResizeArea), findsNothing);
    });

    testWidgets('playlist mode fills the window around the fixed main canvas',
        (tester) async {
      await pump(
        tester,
        region: LowerRegion.playlist,
        surface: const Size(1200, 900),
      );

      expect(tester.getTopLeft(find.byKey(const Key('main-player'))),
          const Offset(6, 6));
      expect(tester.getSize(find.byKey(const Key('main-player'))),
          TrampMetrics.mainPlayer);

      // Playlist fills the content area (window minus 6px frame) below the
      // gutter: 1200-12 wide, 900-12-348-6 tall.
      expect(tester.getTopLeft(find.byKey(const Key('playlist'))),
          const Offset(6, 360));
      expect(tester.getSize(find.byKey(const Key('playlist'))),
          const Size(1188, 534));
    });

    testWidgets('equalizer mode at 200% keeps the stack pinned top-left',
        (tester) async {
      await pump(
        tester,
        region: LowerRegion.equalizer,
        surface: const Size(1674, 1428),
      );
      zoom.setPercent(200);
      await tester.pumpAndSettle();

      expect(tester.getSize(find.byKey(panelStackKey)),
          const Size(1650, 1404));
      expect(tester.getTopLeft(find.byKey(const Key('main-player'))),
          const Offset(6, 6));
      expect(tester.getSize(find.byKey(const Key('main-player'))),
          TrampMetrics.mainPlayer);
      // Painted position of the equalizer: frame + (348+6) * 2.
      expect(tester.getTopLeft(find.byKey(const Key('equalizer'))),
          const Offset(6, 714));
    });

    testWidgets('playlist mode at 200% scales the stack and keeps positions',
        (tester) async {
      await pump(
        tester,
        region: LowerRegion.playlist,
        surface: const Size(1660, 1012),
      );
      zoom.setPercent(200);
      await tester.pumpAndSettle();

      expect(tester.getTopLeft(find.byKey(const Key('main-player'))),
          const Offset(6, 6));
      // Content area is 1648x1000 scaled px: logical width 825, well
      // max(240, (1000 - 354*2) / 2) = 240 logical.
      expect(tester.getTopLeft(find.byKey(const Key('playlist'))),
          const Offset(6, 714));
      expect(tester.getSize(find.byKey(const Key('playlist'))),
          const Size(825, 240));
    });
  });
}
