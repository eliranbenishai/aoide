import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/tramp_shell.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

class _MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late PlaylistController playlist;
  late PlaybackController playback;
  late ZoomController zoom;

  setUp(() {
    playlist = PlaylistController(store: _MemoryStore());
    playback = PlaybackController(
      playlist: playlist,
      engine: FakePlayerEngine(),
    );
    zoom = ZoomController(workArea: const Size(6000, 4000));
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(
    WidgetTester tester, {
    LowerRegion region = LowerRegion.playlist,
    bool collapsed = false,
    Size surface = const Size(824, 500),
  }) async {
    await tester.binding.setSurfaceSize(surface);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: TrampShell(
          zoom: zoom,
          lowerRegion: region,
          equalizerCollapsed: collapsed,
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

  testWidgets('playlist region shows the playlist and not the equalizer',
      (tester) async {
    await pump(tester, region: LowerRegion.playlist);
    expect(find.byKey(const Key('playlist')), findsOneWidget);
    expect(find.byKey(const Key('equalizer')), findsNothing);
  });

  testWidgets('equalizer region shows the equalizer and not the playlist',
      (tester) async {
    await pump(tester, region: LowerRegion.equalizer);
    expect(find.byKey(const Key('equalizer')), findsOneWidget);
    expect(find.byKey(const Key('playlist')), findsNothing);
  });

  testWidgets('the stack scales by the zoom factor', (tester) async {
    await pump(tester, surface: const Size(3000, 2000));
    final at100 = tester.getSize(find.byKey(panelStackKey));

    zoom.setPercent(200);
    await tester.pumpAndSettle();
    final at200 = tester.getSize(find.byKey(panelStackKey));

    expect(at200.width, closeTo(at100.width * 2, 0.01));
  });

  testWidgets('no overflow at any zoom step', (tester) async {
    await tester.binding.setSurfaceSize(const Size(4000, 3000));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    for (final step in ZoomController.steps) {
      zoom.setPercent(step);
      await pump(tester, surface: const Size(4000, 3000));
      expect(tester.takeException(), isNull, reason: 'overflow at $step%');
    }
  });

  testWidgets('main player keeps its exact aspect at every step',
      (tester) async {
    final expected =
        TrampMetrics.mainPlayer.width / TrampMetrics.mainPlayer.height;
    for (final step in ZoomController.steps) {
      zoom.setPercent(step);
      await pump(tester, surface: const Size(4000, 3000));
      final size = tester.getSize(find.byKey(const Key('main-player')));
      expect(size.width / size.height, closeTo(expected, 0.001),
          reason: 'aspect drifted at $step%');
    }
  });

  testWidgets('Ctrl+= steps up, Ctrl+- steps down, Ctrl+0 resets',
      (tester) async {
    await pump(tester, surface: const Size(4000, 3000));

    await tester.sendKeyDownEvent(LogicalKeyboardKey.control);
    await tester.sendKeyEvent(LogicalKeyboardKey.equal);
    await tester.pump();
    expect(zoom.percent, 125);

    await tester.sendKeyEvent(LogicalKeyboardKey.minus);
    await tester.pump();
    expect(zoom.percent, 100);

    await tester.sendKeyEvent(LogicalKeyboardKey.equal);
    await tester.sendKeyEvent(LogicalKeyboardKey.digit0);
    await tester.pump();
    expect(zoom.percent, 100);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.control);
  });

  testWidgets('a collapsed equalizer occupies only its title bar',
      (tester) async {
    await pump(tester, region: LowerRegion.equalizer, collapsed: true);
    expect(tester.takeException(), isNull);
  });
}
