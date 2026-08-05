import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/repeat_mode.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/audio_levels.dart';
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

  test('selecting another row does not change now-playing', () async {
    await playback.playIndex(0);
    playlist.select(2);
    expect(playback.currentTrack?.path, '/a.mp3');
    expect(playlist.selectedIndex, 2);
  });

  test('playPause opens selected track when selection differs', () async {
    await playback.playIndex(0);
    playlist.select(2);
    await playback.playPause();
    expect(playback.currentTrack?.path, '/c.mp3');
    expect(playback.playing, isTrue);
    expect(engine.lastOpenedPath, '/c.mp3');
  });

  test('playPause resumes when selection matches playing track', () async {
    await playback.playIndex(1);
    await playback.playPause(); // pause
    expect(playback.playing, isFalse);
    await playback.playPause(); // resume
    expect(playback.playing, isTrue);
    expect(playback.currentTrack?.path, '/b.mp3');
    expect(engine.lastOpenedPath, '/b.mp3');
  });

  test('playPause re-opens after stop', () async {
    await playback.playIndex(1);
    expect(engine.openCount, 1);
    await playback.stop();
    expect(playback.playing, isFalse);
    expect(engine.hasMedia, isFalse);

    await playback.playPause();
    expect(playback.playing, isTrue);
    expect(playback.currentTrack?.path, '/b.mp3');
    expect(engine.lastOpenedPath, '/b.mp3');
    expect(engine.openCount, 2);
  });

  test('removing playing track advances to next remaining', () async {
    await playback.playIndex(1);
    playlist.removeAt(1);
    await Future<void>.delayed(Duration.zero);
    expect(playback.currentTrack?.path, '/c.mp3');
    expect(playback.playing, isTrue);
  });

  test('removing last playing track stops playback', () async {
    await playback.playIndex(2);
    playlist.removeAt(2);
    await Future<void>.delayed(Duration.zero);
    expect(playback.currentTrack, isNull);
    expect(playback.playing, isFalse);
  });

  test('formatInfo returns to unknown when switching tracks', () async {
    await playback.playIndex(0);
    engine.emitFormat(const AudioFormatInfo(
      bitrateKbps: 320,
      sampleRateHz: 44100,
      channels: 2,
    ));
    await Future<void>.delayed(Duration.zero);
    expect(playback.formatInfo.sampleRateHz, 44100);

    await playback.playIndex(1);
    expect(playback.formatInfo, AudioFormatInfo.unknown);

    // Lone bitrate tick must not resurrect the previous track's sample rate.
    engine.emitBitrate(256);
    await Future<void>.delayed(Duration.zero);
    expect(playback.formatInfo.sampleRateHz, isNull);
    expect(playback.formatInfo.bitrateKbps, 256);
  });

  test('level frames do not notify listeners', () async {
    var listenerCalls = 0;
    playback.addListener(() => listenerCalls++);

    await playback.playIndex(0);
    listenerCalls = 0;

    for (var i = 0; i < 5; i++) {
      engine.emitLevels(AudioLevels.synthesised(intensity: 0.5, seed: i));
    }
    await Future<void>.delayed(Duration.zero);

    expect(listenerCalls, 0);
  });
}
