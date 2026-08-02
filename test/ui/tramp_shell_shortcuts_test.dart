import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/tramp_shell.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late PlaylistController playlist;
  late FakePlayerEngine engine;
  late PlaybackController playback;

  setUp(() {
    playlist = PlaylistController(store: MemoryStore());
    playlist.addTracks(const [
      Track(path: '/a.mp3', title: 'A'),
      Track(path: '/b.mp3', title: 'B'),
      Track(path: '/c.mp3', title: 'C'),
    ]);
    engine = FakePlayerEngine();
    playback = PlaybackController(playlist: playlist, engine: engine);
  });

  tearDown(() async {
    await playback.dispose();
  });

  Future<void> pumpShell(WidgetTester tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: TrampShell(
          playback: playback,
          playlistController: playlist,
          hasTracks: true,
          zoom: ZoomController(workArea: const Size(6000, 4000)),
          lowerRegion: LowerRegion.playlist,
          mainPlayer: const SizedBox(),
          equalizer: const SizedBox(),
          playlist: const SizedBox(),
        ),
      ),
    );
    await tester.pump();
  }

  testWidgets('Space starts playback when tracks exist', (tester) async {
    await pumpShell(tester);
    await tester.sendKeyEvent(LogicalKeyboardKey.space);
    await tester.pump();
    expect(playback.playing, isTrue);
    expect(playback.currentTrack?.path, '/a.mp3');
  });

  testWidgets('Space plays selected track when selection differs', (tester) async {
    await playback.playIndex(0);
    playlist.select(2);
    await pumpShell(tester);

    await tester.sendKeyEvent(LogicalKeyboardKey.space);
    await tester.pump();
    expect(playback.currentTrack?.path, '/c.mp3');
    expect(playback.playing, isTrue);
  });

  testWidgets('arrow keys move playlist selection', (tester) async {
    playlist.select(1);
    await pumpShell(tester);

    await tester.sendKeyEvent(LogicalKeyboardKey.arrowDown);
    await tester.pump();
    expect(playlist.selectedIndex, 2);

    await tester.sendKeyEvent(LogicalKeyboardKey.arrowUp);
    await tester.pump();
    expect(playlist.selectedIndex, 1);
  });

  testWidgets('Delete removes selected track', (tester) async {
    playlist.select(1);
    await pumpShell(tester);

    await tester.sendKeyEvent(LogicalKeyboardKey.delete);
    await tester.pump();
    expect(playlist.playlist.tracks.length, 2);
    expect(playlist.playlist.tracks[1].path, '/c.mp3');
  });

  testWidgets('Enter plays selected track', (tester) async {
    playlist.select(2);
    await pumpShell(tester);

    await tester.sendKeyDownEvent(LogicalKeyboardKey.enter);
    await tester.pump();
    expect(playback.currentTrack?.path, '/c.mp3');
    expect(playback.playing, isTrue);
  });
}
