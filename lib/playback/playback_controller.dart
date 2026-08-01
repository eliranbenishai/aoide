import 'dart:async';

import 'package:flutter/foundation.dart';

import '../domain/repeat_mode.dart';
import '../domain/track.dart';
import '../playlist/playlist_controller.dart';
import 'player_engine.dart';

class PlaybackController extends ChangeNotifier {
  PlaybackController({
    required PlaylistController playlist,
    required PlayerEngine engine,
  })  : _playlist = playlist,
        _engine = engine {
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
    _playlist.addListener(_onPlaylistChanged);
  }

  final PlaylistController _playlist;
  final PlayerEngine _engine;
  final List<StreamSubscription<dynamic>> _subscriptions = [];

  bool _playing = false;
  bool _muted = false;
  double _volume = 1.0;
  double _preMuteVolume = 1.0;
  Duration _position = Duration.zero;
  Duration _duration = Duration.zero;
  bool _shuffle = false;
  RepeatMode _repeatMode = RepeatMode.off;
  List<int> _shuffledOrder = [];

  bool get playing => _playing;
  bool get muted => _muted;
  double get volume => _volume;
  Duration get position => _position;
  Duration get duration => _duration;
  bool get shuffle => _shuffle;
  RepeatMode get repeatMode => _repeatMode;

  Track? get currentTrack {
    final index = _playlist.selectedIndex;
    if (index == null) return null;
    final tracks = _playlist.playlist.tracks;
    if (index < 0 || index >= tracks.length) return null;
    return tracks[index];
  }

  Future<void> playPause() async {
    if (_playing) {
      await _engine.pause();
    } else {
      await _engine.play();
    }
  }

  Future<void> stop() async {
    await _engine.stop();
  }

  Future<void> next() async {
    final tracks = _playlist.playlist.tracks;
    if (tracks.isEmpty) return;

    final current = _playlist.selectedIndex ?? 0;
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

    final current = _playlist.selectedIndex ?? 0;
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

    _playlist.select(index);
    final track = tracks[index];
    await _engine.open(track);
    await _engine.play();
    notifyListeners();
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

  Future<void> dispose() async {
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

  Future<void> _onCompleted() async {
    switch (_repeatMode) {
      case RepeatMode.one:
        await _engine.seek(Duration.zero);
        await _engine.play();
      case RepeatMode.all:
        await next();
      case RepeatMode.off:
        final current = _playlist.selectedIndex;
        if (current == null || current >= _playlist.playlist.tracks.length - 1) {
          await stop();
        } else {
          await next();
        }
    }
  }
}
