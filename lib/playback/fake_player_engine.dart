import 'dart:async';

import '../domain/track.dart';
import 'player_engine.dart';

class FakePlayerEngine implements PlayerEngine {
  FakePlayerEngine({this.trackDuration = const Duration(seconds: 3)});

  final Duration trackDuration;

  String? lastOpenedPath;

  Duration _position = Duration.zero;
  Duration _duration = Duration.zero;
  bool _playing = false;

  final _positionController = StreamController<Duration>.broadcast();
  final _durationController = StreamController<Duration>.broadcast();
  final _playingController = StreamController<bool>.broadcast();
  final _completedController = StreamController<void>.broadcast();

  @override
  Stream<Duration> get positionStream => _positionController.stream;

  @override
  Stream<Duration> get durationStream => _durationController.stream;

  @override
  Stream<bool> get playingStream => _playingController.stream;

  @override
  Stream<void> get completedStream => _completedController.stream;

  @override
  Future<void> open(Track track) async {
    lastOpenedPath = track.path;
    _position = Duration.zero;
    _duration = track.duration ?? trackDuration;
    _positionController.add(_position);
    _durationController.add(_duration);
  }

  @override
  Future<void> play() async {
    _playing = true;
    _playingController.add(_playing);
  }

  @override
  Future<void> pause() async {
    _playing = false;
    _playingController.add(_playing);
  }

  @override
  Future<void> stop() async {
    _playing = false;
    _position = Duration.zero;
    _playingController.add(_playing);
    _positionController.add(_position);
  }

  @override
  Future<void> seek(Duration position) async {
    _position = position;
    _positionController.add(_position);
  }

  @override
  Future<void> setVolume(double volume) async {}

  @override
  Future<void> dispose() async {
    await _positionController.close();
    await _durationController.close();
    await _playingController.close();
    await _completedController.close();
  }

  Future<void> emitCompleted() async {
    _completedController.add(null);
  }
}
