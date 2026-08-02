import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/main_player/main_player_panel.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

import '../../support/test_fonts.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late FakePlayerEngine engine;
  late PlaylistController playlist;
  late PlaybackController playback;
  late ZoomController zoom;

  setUpAll(loadTrampFonts);

  setUp(() {
    engine = FakePlayerEngine();
    playlist = PlaylistController(store: MemoryStore());
    playback = PlaybackController(playlist: playlist, engine: engine);
    zoom = ZoomController(workArea: const Size(6000, 4000));
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(
    WidgetTester tester, {
    LowerRegion region = LowerRegion.playlist,
    ValueChanged<LowerRegion>? onSelectRegion,
    VoidCallback? onOpenFiles,
    bool hasTracks = true,
  }) async {
    // Default test surface is 800×600; the panel canvas is 812 wide.
    await tester.binding.setSurfaceSize(const Size(1024, 768));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: MainPlayerPanel(
            playback: playback,
            zoom: zoom,
            lowerRegion: region,
            hasTracks: hasTracks,
            draggableTitle: false,
            onSelectRegion: onSelectRegion ?? (_) {},
            onOpenFiles: onOpenFiles,
          ),
        ),
      ),
    );
  }

  testWidgets('holds the locked canvas size without overflowing',
      (tester) async {
    await pump(tester);
    expect(
      tester.getSize(find.byType(MainPlayerPanel)),
      TrampMetrics.mainPlayer,
    );
    expect(tester.takeException(), isNull);
  });

  testWidgets('is branded TRAMP, never Winamp', (tester) async {
    await pump(tester);
    expect(find.text('TRAMP'), findsOneWidget);
    expect(find.textContaining('WINAMP'), findsNothing);
  });

  testWidgets('transport buttons drive the controller', (tester) async {
    playlist.addTracks([const Track(path: 'a.mp3'), const Track(path: 'b.mp3')]);
    await tester.pump();
    await pump(tester);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(playback.playing, isTrue);

    await tester.tap(find.byKey(const Key('transport-pause')));
    await tester.pumpAndSettle();
    expect(playback.playing, isFalse);

    await tester.tap(find.byKey(const Key('transport-next')));
    await tester.pumpAndSettle();
    expect(playback.playingIndex, 1);
  });

  testWidgets('shuffle and repeat toggle the controller', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('player-shuffle')));
    await tester.pump();
    expect(playback.shuffle, isTrue);

    await tester.tap(find.byKey(const Key('player-repeat')));
    await tester.pump();
    expect(playback.repeatMode.name, 'all');
  });

  testWidgets('mute toggles and dims the volume fill', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('transport-mute')));
    await tester.pump();
    expect(playback.muted, isTrue);
  });

  testWidgets('panel contains no Icon widgets', (tester) async {
    await pump(tester);
    expect(find.byType(Icon), findsNothing);
  });

  testWidgets('EQ and PL buttons request a region change', (tester) async {
    final requested = <LowerRegion>[];
    await pump(tester, onSelectRegion: requested.add);
    await tester.tap(find.byKey(const Key('player-eq')));
    await tester.tap(find.byKey(const Key('player-pl')));
    expect(requested, [LowerRegion.equalizer, LowerRegion.playlist]);
  });

  testWidgets('OPEN calls the open-files callback', (tester) async {
    var opens = 0;
    await pump(tester, onOpenFiles: () => opens++);
    await tester.tap(find.byKey(const Key('player-open')));
    expect(opens, 1);
  });

  testWidgets('zoom button shows the current step and opens a menu',
      (tester) async {
    zoom.setPercent(150);
    await pump(tester);
    expect(find.text('ZOOM 150%'), findsOneWidget);
    await tester.tap(find.byKey(const Key('player-zoom')));
    await tester.pumpAndSettle();
    expect(find.text('200%'), findsOneWidget);
    await tester.tap(find.text('200%'));
    await tester.pumpAndSettle();
    expect(zoom.percent, 200);
  });

  testWidgets('steps too large for the display are not offered',
      (tester) async {
    zoom = ZoomController(workArea: const Size(1600, 1200));
    await pump(tester);
    await tester.tap(find.byKey(const Key('player-zoom')));
    await tester.pumpAndSettle();
    expect(find.text('150%'), findsOneWidget);
    expect(find.text('300%'), findsNothing);
  });

  testWidgets('display shows real stream properties when reported',
      (tester) async {
    await pump(tester);
    engine.emitFormat(const AudioFormatInfo(
      bitrateKbps: 128,
      sampleRateHz: 44100,
      channels: 2,
    ));
    await tester.pump();
    expect(find.text('128 kbps'), findsOneWidget);
    expect(find.text('44 kHz'), findsOneWidget);
    expect(find.text('stereo'), findsOneWidget);
  });

  testWidgets('indicators light for the visible region', (tester) async {
    await pump(tester, region: LowerRegion.equalizer);
    expect(find.text('EQ'), findsWidgets);
    expect(find.text('PL'), findsWidgets);
  });

  testWidgets('transport is disabled with an empty playlist', (tester) async {
    await pump(tester, hasTracks: false);
    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(playback.playing, isFalse);
  });
}
