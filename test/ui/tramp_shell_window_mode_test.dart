import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
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
      // 812+12 wide; 12 + 242 + 6 + 206 tall.
      expect(eqModeWindowSize(1), const Size(824, 466));
    });

    test('scales with the zoom factor', () {
      expect(eqModeWindowSize(2), const Size(1648, 932));
      expect(eqModeWindowSize(1.5), const Size(1236, 699));
    });
  });

  group('eqModeWindowSize (collapsed)', () {
    test('snaps the lower region to the title bar when collapsed', () {
      // 812+12 wide; 12 + 242 + 6 + 35 (titleBar) tall.
      expect(eqModeWindowSize(1, collapsed: true), const Size(824, 295));
    });

    test('scales the collapsed stack with the zoom factor', () {
      expect(eqModeWindowSize(2, collapsed: true), const Size(1648, 590));
    });
  });

  group('playlistModeMinimumSize', () {
    test('fits the main player plus the minimum well at 100%', () {
      // 12 + 242 + 6 + 240 tall.
      expect(playlistModeMinimumSize(1), const Size(824, 500));
    });

    test('scales with the zoom factor', () {
      expect(playlistModeMinimumSize(2), const Size(1648, 1000));
    });
  });

  group('defaultPlaylistModeWindowSize', () {
    test('scales main width and tall well height', () {
      // 12 + 242 + 6 + 400 tall.
      expect(defaultPlaylistModeWindowSize(1), const Size(824, 660));
      expect(defaultPlaylistModeWindowSize(2), const Size(1648, 1320));
    });
  });

  group('playlistModeWindowSize', () {
    test('falls back to the default when nothing is stored', () {
      expect(
        playlistModeWindowSize(factor: 1),
        const Size(824, 660),
      );
    });

    test('falls back to the default when only one dimension is stored', () {
      expect(
        playlistModeWindowSize(factor: 1, storedWidth: 900),
        const Size(824, 660),
      );
      expect(
        playlistModeWindowSize(factor: 1, storedHeight: 700),
        const Size(824, 660),
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
        const Size(824, 500),
      );
      expect(
        playlistModeWindowSize(factor: 2, storedWidth: 900, storedHeight: 300),
        const Size(1800, 1000),
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
      final logical = logicalPlaylistWindowSize(const Size(1648, 1320), 2);
      expect(
        playlistModeWindowSize(
          factor: 2,
          storedWidth: logical.width,
          storedHeight: logical.height,
        ),
        const Size(1648, 1320),
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
      expect(target.size, const Size(1648, 932));
      expect(target.minimumSize, const Size(1648, 932));
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
      expect(target.minimumSize, const Size(824, 500));
      expect(target.resizable, isTrue);
    });

    test('playlist mode without a stored size uses the default tall size', () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
      );
      expect(target.size, const Size(824, 660));
      expect(target.minimumSize, const Size(824, 500));
      expect(target.resizable, isTrue);
    });

    test('collapsed equalizer mode snaps to the shade stack', () {
      final target = windowModeTarget(
        lowerRegion: LowerRegion.equalizer,
        factor: 1,
        equalizerCollapsed: true,
      );
      expect(target.size, const Size(824, 295));
      expect(target.minimumSize, const Size(824, 295));
      expect(target.resizable, isFalse);
    });
  });

  group('panelStackLayout', () {
    test('equalizer mode is always the exact stack', () {
      const layout = PanelStackLayout(
        logicalWidth: 812,
        lowerHeight: 206,
      );
      expect(layout.logicalHeight, 454);
      expect(layout.hostSize(2), const Size(1624, 908));

      final eq = panelStackLayout(
        lowerRegion: LowerRegion.equalizer,
        factor: 2,
        contentSize: const Size(5000, 3000),
      );
      expect(eq.logicalWidth, 812);
      expect(eq.lowerHeight, 206);
    });

    test('collapsed equalizer mode drops the lower region to the title bar',
        () {
      final eq = panelStackLayout(
        lowerRegion: LowerRegion.equalizer,
        factor: 2,
        contentSize: const Size(5000, 3000),
        equalizerCollapsed: true,
      );
      expect(eq.logicalWidth, 812);
      expect(eq.lowerHeight, 35);
      expect(eq.logicalHeight, 283);
    });

    test('playlist mode fills the content area', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
        contentSize: const Size(1200, 900),
      );
      expect(layout.logicalWidth, 1200);
      // 900 - 242 - 6 left for the well.
      expect(layout.lowerHeight, 652);
      expect(layout.hostSize(1), const Size(1200, 900));
    });

    test('playlist mode never shrinks below the fixed canvases', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 1,
        contentSize: const Size(400, 300),
      );
      expect(layout.logicalWidth, 812);
      expect(layout.lowerHeight, 240);
    });

    test('playlist mode divides the content area by the factor', () {
      final layout = panelStackLayout(
        lowerRegion: LowerRegion.playlist,
        factor: 2,
        contentSize: const Size(1648, 1000),
      );
      expect(layout.logicalWidth, 824);
      // (1000 - (242+6)*2) / 2 = 252.
      expect(layout.lowerHeight, 252);
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
            mainPlayer: const SizedBox(
              key: Key('main-player'),
              width: 812,
              height: 242,
            ),
            equalizer: const SizedBox(
              key: Key('equalizer'),
              width: 812,
              height: 206,
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
        surface: const Size(824, 660),
      );
      expect(find.byType(DragToResizeArea), findsOneWidget);
    });

    testWidgets('equalizer mode has no resize edges', (tester) async {
      await pump(
        tester,
        region: LowerRegion.equalizer,
        surface: const Size(824, 466),
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
          const Size(812, 242));

      // Playlist fills the content area (window minus 6px frame) below the
      // gutter: 1200-12 wide, 900-12-242-6 tall.
      expect(tester.getTopLeft(find.byKey(const Key('playlist'))),
          const Offset(6, 254));
      expect(tester.getSize(find.byKey(const Key('playlist'))),
          const Size(1188, 640));
    });

    testWidgets('equalizer mode at 200% keeps the stack pinned top-left',
        (tester) async {
      await pump(
        tester,
        region: LowerRegion.equalizer,
        surface: const Size(1648, 932),
      );
      zoom.setPercent(200);
      await tester.pumpAndSettle();

      expect(tester.getSize(find.byKey(panelStackKey)),
          const Size(1624, 908));
      expect(tester.getTopLeft(find.byKey(const Key('main-player'))),
          const Offset(6, 6));
      expect(tester.getSize(find.byKey(const Key('main-player'))),
          const Size(812, 242));
      // Painted position of the equalizer: frame + (242+6) * 2.
      expect(tester.getTopLeft(find.byKey(const Key('equalizer'))),
          const Offset(6, 502));
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
      // Content area is 1648x1000 scaled px: logical width 824, well
      // (1000 - 248*2) / 2 = 252 logical.
      expect(tester.getTopLeft(find.byKey(const Key('playlist'))),
          const Offset(6, 502));
      expect(tester.getSize(find.byKey(const Key('playlist'))),
          const Size(824, 252));
    });
  });
}
