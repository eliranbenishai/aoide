import 'dart:async';

import 'package:flutter/foundation.dart';

import '../domain/repeat_mode.dart';
import '../domain/track.dart';
import '../platform/usage_store.dart';
import '../playlist/playlist_controller.dart';
import 'audio_format_info.dart';
import 'audio_levels.dart';
import 'playback_failure.dart';
import 'player_engine.dart';

class PlaybackController extends ChangeNotifier {
  PlaybackController({
    required PlaylistController playlist,
    required PlayerEngine engine,
    UsageStore? usageStore,
    this.spinPersistDebounce = const Duration(seconds: 2),
  })  : _playlist = playlist,
        _engine = engine,
        _usageStore = usageStore {
    _subscriptions.add(
      _engine.playingStream.listen((value) {
        _playing = value;
        notifyListeners();
      }),
    );
    _subscriptions.add(
      _engine.positionStream.listen((value) {
        _position = value;
        notifyListeners();
      }),
    );
    _subscriptions.add(
      _engine.durationStream.listen((value) {
        _duration = value;
        notifyListeners();
      }),
    );
    _subscriptions.add(
      _engine.completedStream.listen((_) {
        unawaited(_onCompleted());
      }),
    );
    _subscriptions.add(
      _engine.formatStream.listen((value) {
        _formatInfo = value;
        notifyListeners();
      }),
    );
    _subscriptions.add(_engine.errorStream.listen(_onEngineError));
    _previousTrackCount = _playlist.playlist.tracks.length;
    _playlist.addListener(_onPlaylistChanged);
  }

  final PlaylistController _playlist;
  final PlayerEngine _engine;
  final UsageStore? _usageStore;
  final List<StreamSubscription<dynamic>> _subscriptions = [];

  /// Coalesces writes of the lifetime **spin** count. Two seconds, the same
  /// debounce the altered current playlist and playback resume already keep, so
  /// a listener who leaves an album running writes one small file per track
  /// rather than one per notification.
  final Duration spinPersistDebounce;

  bool _playing = false;
  bool _muted = false;
  double _volume = 1.0;
  double _preMuteVolume = 1.0;
  Duration _position = Duration.zero;
  Duration _duration = Duration.zero;
  bool _shuffle = false;
  RepeatMode _repeatMode = RepeatMode.off;
  List<int> _shuffledOrder = [];
  int? _playingIndex;
  String? _playingPath;
  /// False after [stop]: media_kit unloads the file, so the next play must
  /// [playIndex] (re-open) rather than call [PlayerEngine.play] alone.
  bool _mediaOpen = false;
  int _previousTrackCount = 0;
  AudioFormatInfo _formatInfo = AudioFormatInfo.unknown;
  PlaybackFailure? _failure;
  int _spins = 0;
  Timer? _spinPersistTimer;

  bool get playing => _playing;
  /// Track loaded and not playing (after pause; cleared by [stop]).
  bool get paused => _mediaOpen && !_playing;
  int? get playingIndex => _playingIndex;
  bool get muted => _muted;
  double get volume => _volume;
  Duration get position => _position;
  Duration get duration => _duration;
  bool get shuffle => _shuffle;
  RepeatMode get repeatMode => _repeatMode;
  AudioFormatInfo get formatInfo => _formatInfo;

  /// The last track the engine refused, or null while playback is healthy.
  ///
  /// Cleared by the next [playIndex], so it always describes the track the
  /// transport is pointed at now rather than something older.
  PlaybackFailure? get failure => _failure;

  /// Lifetime **spins**: tracks played through to the end.
  ///
  /// Counted from end-of-stream alone, so skipping never inflates it however
  /// late the skip comes, and stopping never does either. Each repeat-one pass
  /// counts, because the track genuinely played through.
  int get spins => _spins;

  /// Picks the lifetime count up where the last session left it.
  ///
  /// Called by the host during bootstrap. A missing or unreadable usage file
  /// reads as zero rather than failing startup — the store answers that.
  Future<void> loadUsage() async {
    final store = _usageStore;
    if (store == null) return;
    final counters = await store.read();
    if (counters.spins == _spins) return;
    _spins = counters.spins;
    notifyListeners();
  }

  /// Writes any pending count now. The debounce means a session that ends
  /// seconds after a track finished would otherwise lose that spin, and quit
  /// already pays for one small write.
  Future<void> flushUsage() async {
    _spinPersistTimer?.cancel();
    _spinPersistTimer = null;
    await _usageStore?.write(UsageCounters(spins: _spins));
  }

  /// Analyser frames, consumed directly by the spectrum display.
  ///
  /// Not surfaced as controller state on purpose: notifying listeners at frame
  /// rate would rebuild the entire player chrome thirty times a second.
  Stream<AudioLevels> get levelsStream => _engine.levelsStream;

  Future<void> setForceMono(bool enabled) => _engine.setForceMono(enabled);

  Track? get currentTrack {
    final index = _playingIndex;
    if (index == null) return null;
    final tracks = _playlist.playlist.tracks;
    if (index < 0 || index >= tracks.length) return null;
    return tracks[index];
  }

  Future<void> playPause() async {
    final selected = _playlist.selectedIndex;
    final nothingOpen = _playingIndex == null;
    final selectionDiffers =
        selected != null && selected != _playingIndex;

    if (nothingOpen || selectionDiffers) {
      if (selected != null) {
        await playIndex(selected);
      } else if (_playingIndex != null) {
        await _pauseOrResumeCurrent();
      } else {
        final tracks = _playlist.playlist.tracks;
        if (tracks.isNotEmpty) {
          await playIndex(0);
        }
      }
      return;
    }

    await _pauseOrResumeCurrent();
  }

  Future<void> stop() async {
    await _engine.stop();
    _mediaOpen = false;
    _position = Duration.zero;
    notifyListeners();
  }

  Future<void> next() async {
    final tracks = _playlist.playlist.tracks;
    if (tracks.isEmpty) return;

    final current = _playingIndex ?? _playlist.selectedIndex ?? 0;
    final nextIndex = _resolveNextIndex(current, tracks.length);
    if (nextIndex == null) {
      await stop();
      return;
    }
    await playIndex(nextIndex);
  }

  Future<void> previous() async {
    final tracks = _playlist.playlist.tracks;
    if (tracks.isEmpty) return;

    final current = _playingIndex ?? _playlist.selectedIndex ?? 0;
    final previousIndex = _resolvePreviousIndex(current, tracks.length);
    if (previousIndex == null) {
      await _engine.seek(Duration.zero);
      return;
    }
    await playIndex(previousIndex);
  }

  Future<void> playIndex(int index) async {
    final tracks = _playlist.playlist.tracks;
    if (index < 0 || index >= tracks.length) return;

    _playingIndex = index;
    _playingPath = tracks[index].path;
    _formatInfo = AudioFormatInfo.unknown;
    _failure = null;
    _playlist.select(index);
    final track = tracks[index];
    await _engine.open(track);
    _mediaOpen = true;
    await _engine.play();
    notifyListeners();
  }

  /// Takes the engine's word that the open track will not play.
  ///
  /// The engine leaves its own state alone, so this is where the transport
  /// stops describing itself as playing. Media is marked closed as well as
  /// stopped, which keeps [paused] down — a track that never started is not
  /// paused, and a later resume must re-open rather than press play on
  /// nothing.
  void _onEngineError(String message) {
    final path = _playingPath;
    if (path == null) return;
    _failure = PlaybackFailure(path: path, message: message);
    _playing = false;
    _mediaOpen = false;
    notifyListeners();
  }

  Future<void> _pauseOrResumeCurrent() async {
    final index = _playingIndex;
    if (index == null) return;
    if (_playing) {
      await _engine.pause();
      return;
    }
    if (!_mediaOpen) {
      await playIndex(index);
      return;
    }
    await _engine.play();
  }

  Future<void> seek(Duration position) async {
    await _engine.seek(position);
  }

  void setVolume(double volume) {
    final clamped = volume.clamp(0.0, 1.0);
    _volume = clamped;
    if (!_muted) {
      unawaited(_engine.setVolume(clamped));
    }
    if (clamped > 0) {
      _preMuteVolume = clamped;
    }
    notifyListeners();
  }

  void toggleMute() {
    if (_muted) {
      _muted = false;
      _volume = _preMuteVolume;
      unawaited(_engine.setVolume(_preMuteVolume));
    } else {
      _preMuteVolume = _volume;
      _muted = true;
      unawaited(_engine.setVolume(0));
    }
    notifyListeners();
  }

  void toggleShuffle() {
    _shuffle = !_shuffle;
    if (_shuffle) {
      _rebuildShuffleOrder();
    }
    notifyListeners();
  }

  void cycleRepeatMode() {
    _repeatMode = switch (_repeatMode) {
      RepeatMode.off => RepeatMode.all,
      RepeatMode.all => RepeatMode.one,
      RepeatMode.one => RepeatMode.off,
    };
    notifyListeners();
  }

  @override
  Future<void> dispose() async {
    _spinPersistTimer?.cancel();
    _spinPersistTimer = null;
    _playlist.removeListener(_onPlaylistChanged);
    for (final subscription in _subscriptions) {
      await subscription.cancel();
    }
    _subscriptions.clear();
    await _engine.dispose();
    super.dispose();
  }

  void _onPlaylistChanged() {
    if (_shuffle) {
      _rebuildShuffleOrder();
    }

    final tracks = _playlist.playlist.tracks;
    final previousLength = _previousTrackCount;
    _previousTrackCount = tracks.length;

    if (_playingPath != null) {
      final newIndex = tracks.indexWhere((track) => track.path == _playingPath);
      if (newIndex != -1) {
        _playingIndex = newIndex;
      } else if (tracks.length == previousLength - 1) {
        final advanceIndex = _playingIndex;
        _playingPath = null;
        if (advanceIndex != null && advanceIndex < tracks.length) {
          unawaited(playIndex(advanceIndex));
          return;
        }
        _playingIndex = null;
        _mediaOpen = false;
        unawaited(_engine.stop());
      } else {
        _playingIndex = null;
        _playingPath = null;
        _mediaOpen = false;
        unawaited(_engine.stop());
      }
    }

    notifyListeners();
  }

  void _rebuildShuffleOrder() {
    final length = _playlist.playlist.tracks.length;
    _shuffledOrder = List<int>.generate(length, (index) => index)..shuffle();
  }

  int? _resolveNextIndex(int current, int length) {
    if (length == 0) return null;

    if (_shuffle) {
      final position = _shuffledOrder.indexOf(current);
      if (position == -1) return current.clamp(0, length - 1);
      if (position < _shuffledOrder.length - 1) {
        return _shuffledOrder[position + 1];
      }
      if (_repeatMode == RepeatMode.all) {
        return _shuffledOrder.first;
      }
      return null;
    }

    if (current < length - 1) return current + 1;
    if (_repeatMode == RepeatMode.all) return 0;
    return null;
  }

  int? _resolvePreviousIndex(int current, int length) {
    if (length == 0) return null;

    if (_shuffle) {
      final position = _shuffledOrder.indexOf(current);
      if (position == -1) return current.clamp(0, length - 1);
      if (position > 0) return _shuffledOrder[position - 1];
      if (_repeatMode == RepeatMode.all) {
        return _shuffledOrder.last;
      }
      return null;
    }

    if (current > 0) return current - 1;
    if (_repeatMode == RepeatMode.all) return length - 1;
    return null;
  }

  /// End of stream: the one place a **spin** is counted.
  ///
  /// Nothing else reaches here. [next], [previous] and [stop] drive the engine
  /// directly, so a skip a second before the end counts for nothing, which is
  /// the whole point of the figure.
  Future<void> _onCompleted() async {
    _countSpin();
    switch (_repeatMode) {
      case RepeatMode.one:
        await _engine.seek(Duration.zero);
        await _engine.play();
      case RepeatMode.all:
        await next();
      case RepeatMode.off:
        final current = _playingIndex;
        if (current == null || current >= _playlist.playlist.tracks.length - 1) {
          await stop();
        } else {
          await next();
        }
    }
  }

  void _countSpin() {
    _spins++;
    _scheduleSpinPersist();
    notifyListeners();
  }

  void _scheduleSpinPersist() {
    final store = _usageStore;
    if (store == null) return;
    _spinPersistTimer?.cancel();
    if (spinPersistDebounce <= Duration.zero) {
      unawaited(store.write(UsageCounters(spins: _spins)));
      return;
    }
    _spinPersistTimer = Timer(spinPersistDebounce, () {
      unawaited(store.write(UsageCounters(spins: _spins)));
    });
  }
}
