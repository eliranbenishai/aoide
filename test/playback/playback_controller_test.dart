import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/repeat_mode.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';

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

  test('playIndex opens track and plays', () async {
    await playback.playIndex(1);
    expect(playback.currentTrack?.path, '/b.mp3');
    expect(playback.playing, isTrue);
    expect(engine.lastOpenedPath, '/b.mp3');
  });

  test('repeat one replays same track on completion', () async {
    playback.cycleRepeatMode(); // all
    playback.cycleRepeatMode(); // one
    await playback.playIndex(0);
    await engine.emitCompleted();
    expect(playback.currentTrack?.path, '/a.mp3');
    expect(playback.repeatMode, RepeatMode.one);
  });

  test('next wraps when repeat all', () async {
    playback.cycleRepeatMode(); // all
    await playback.playIndex(2);
    await playback.next();
    expect(playback.currentTrack?.path, '/a.mp3');
  });
}
