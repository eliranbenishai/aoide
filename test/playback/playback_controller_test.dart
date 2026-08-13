import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/repeat_mode.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/audio_levels.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/platform/usage_store.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

class MemoryUsageStore implements UsageStore {
  UsageCounters counters = UsageCounters.empty;
  int writes = 0;

  @override
  Future<UsageCounters> read() async => counters;

  @override
  Future<void> write(UsageCounters next) async {
    writes++;
    counters = next;
  }
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
    expect(playback.paused, isTrue);
    await playback.playPause(); // resume
    expect(playback.playing, isTrue);
    expect(playback.paused, isFalse);
    expect(playback.currentTrack?.path, '/b.mp3');
    expect(engine.lastOpenedPath, '/b.mp3');
  });

  test('playPause re-opens after stop', () async {
    await playback.playIndex(1);
    expect(engine.openCount, 1);
    await playback.stop();
    expect(playback.playing, isFalse);
    expect(playback.paused, isFalse);
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

  test('reordering under the playing track never interrupts it', () async {
    await playback.playIndex(1);
    final openedBefore = engine.openCount;

    playlist.move(1, 3); // drag B to the end
    await Future<void>.delayed(Duration.zero);

    expect(playback.currentTrack?.path, '/b.mp3');
    expect(playback.playing, isTrue);
    expect(
      engine.openCount,
      openedBefore,
      reason: 'nothing is re-opened, so the audio does not so much as stutter',
    );
    // The playing mark follows the track to its new row.
    expect(playback.playingIndex, 2);
  });

  test('reordering around the playing track moves its mark too', () async {
    await playback.playIndex(2);

    playlist.move(0, 3); // A to the end, C slides up
    await Future<void>.delayed(Duration.zero);

    expect(playback.currentTrack?.path, '/c.mp3');
    expect(playback.playingIndex, 1);
    expect(playback.playing, isTrue);
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

  group('spins', () {
    test('a track reaching its natural end counts', () async {
      await playback.playIndex(0);
      expect(playback.spins, 0);

      await engine.emitCompleted();

      expect(playback.spins, 1);
    });

    test('skipping never counts, however close to the end', () async {
      await playback.playIndex(0);
      // A hair from the end, and the listener hits next anyway.
      await playback.seek(engine.trackDuration - const Duration(seconds: 1));

      await playback.next();
      await playback.previous();

      expect(playback.spins, 0);
    });

    test('stopping never counts', () async {
      await playback.playIndex(0);

      await playback.stop();

      expect(playback.spins, 0);
    });

    test('each repeat-one pass counts, because the track played through',
        () async {
      playback.cycleRepeatMode(); // all
      playback.cycleRepeatMode(); // one
      await playback.playIndex(0);

      for (var pass = 0; pass < 3; pass++) {
        await engine.emitCompleted();
        await Future<void>.delayed(Duration.zero);
      }

      expect(playback.spins, 3);
      expect(playback.currentTrack?.path, '/a.mp3');
    });

    test('the last track of a list counts even though playback then stops',
        () async {
      await playback.playIndex(2);

      await engine.emitCompleted();
      await Future<void>.delayed(Duration.zero);

      expect(playback.spins, 1);
      expect(playback.playing, isFalse);
    });

    test('a whole list played through counts every track', () async {
      await playback.playIndex(0);

      // A ends and B starts, B ends and C starts, C ends and playback stops.
      for (var track = 0; track < 3; track++) {
        await engine.emitCompleted();
        await Future<void>.delayed(Duration.zero);
      }

      expect(playback.spins, 3);
    });

    test('the count notifies, so the About window hears about it', () async {
      await playback.playIndex(0);
      var notifications = 0;
      playback.addListener(() => notifications++);

      await engine.emitCompleted();

      expect(notifications, greaterThan(0));
    });
  });

  group('spins across restarts', () {
    late MemoryUsageStore usage;
    late FakePlayerEngine ownEngine;
    late PlaylistController ownPlaylist;

    setUp(() {
      usage = MemoryUsageStore();
      ownEngine = FakePlayerEngine();
      ownPlaylist = PlaylistController(store: MemoryStore());
      ownPlaylist.addTracks(const [
        Track(path: '/a.mp3', title: 'A'),
        Track(path: '/b.mp3', title: 'B'),
      ]);
    });

    PlaybackController counting({
      int spinsSoFar = 0,
      Duration debounce = Duration.zero,
    }) {
      usage.counters = UsageCounters(spins: spinsSoFar);
      final controller = PlaybackController(
        playlist: ownPlaylist,
        engine: ownEngine,
        usageStore: usage,
        spinPersistDebounce: debounce,
      );
      addTearDown(controller.dispose);
      return controller;
    }

    test('a session picks the lifetime count up where the last left it',
        () async {
      final controller = counting(spinsSoFar: 4095);

      await controller.loadUsage();

      expect(controller.spins, 4095);
    });

    test('a spin is written out, so the next launch reads it back', () async {
      final controller = counting(spinsSoFar: 7);
      await controller.loadUsage();

      await controller.playIndex(0);
      await ownEngine.emitCompleted();
      await Future<void>.delayed(Duration.zero);

      expect(usage.counters.spins, 8);
      expect(usage.writes, 1, reason: 'one write per track, not per notify');
    });

    test('a missing or corrupt usage store reads as zero', () async {
      // What FileUsageStore answers for a file that is not there, or that a
      // crash left half written.
      final controller = counting();

      await controller.loadUsage();

      expect(controller.spins, 0);
    });

    test('writes are debounced while an album plays through', () async {
      final controller = counting(debounce: const Duration(seconds: 2));
      await controller.loadUsage();
      await controller.playIndex(0);

      await ownEngine.emitCompleted();
      await Future<void>.delayed(Duration.zero);
      await ownEngine.emitCompleted();
      await Future<void>.delayed(Duration.zero);

      expect(controller.spins, 2);
      expect(usage.writes, 0, reason: 'nothing is written mid-debounce');

      // Quit does not wait for the timer out; it flushes what is pending.
      await controller.flushUsage();

      expect(usage.counters.spins, 2);
      expect(usage.writes, 1);
    });

    test('a controller with no usage store still counts, it just forgets',
        () async {
      await playback.playIndex(0);

      await engine.emitCompleted();

      expect(playback.spins, 1);
      await playback.flushUsage();
    });
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
