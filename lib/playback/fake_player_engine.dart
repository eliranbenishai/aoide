import 'dart:async';

import '../domain/track.dart';
import 'audio_format_info.dart';
import 'audio_levels.dart';
import 'player_engine.dart';

class FakePlayerEngine implements PlayerEngine {
  FakePlayerEngine({this.trackDuration = const Duration(seconds: 3)});

  final Duration trackDuration;

  String? lastOpenedPath;
  int openCount = 0;
  bool forceMono = false;
  final List<String> audioChannelsValues = <String>[];

  /// Matches media_kit: [stop] unloads media; [play] is a no-op until [open].
  bool get hasMedia => _hasMedia;
  bool get isPlaying => _playing;

  Duration _position = Duration.zero;
  Duration _duration = Duration.zero;
  bool _playing = false;
  bool _hasMedia = false;

  final _positionController = StreamController<Duration>.broadcast();
  final _durationController = StreamController<Duration>.broadcast();
  final _playingController = StreamController<bool>.broadcast();
  final _completedController = StreamController<void>.broadcast();
  final _errorController = StreamController<String>.broadcast();
  final _levelsController = StreamController<AudioLevels>.broadcast();
  final _formatController = StreamController<AudioFormatInfo>.broadcast();

  int? _sampleRateHz;
  int? _channels;
  int? _bitrateKbps;

  void _emitFormat() {
    _formatController.add(AudioFormatInfo(
      bitrateKbps: _bitrateKbps,
      sampleRateHz: _sampleRateHz,
      channels: _channels,
    ));
  }

  @override
  Stream<Duration> get positionStream => _positionController.stream;

  @override
  Stream<Duration> get durationStream => _durationController.stream;

  @override
  Stream<bool> get playingStream => _playingController.stream;

  @override
  Stream<void> get completedStream => _completedController.stream;

  @override
  Stream<String> get errorStream => _errorController.stream;

  /// Reports a failure the way libmpv does: after [open] and [play] have both
  /// already succeeded, leaving this engine's own state untouched. Correcting
  /// that state is the controller's job, which is what tests here assert.
  Future<void> emitError(String message) async {
    _errorController.add(message);
  }

  @override
  Stream<AudioLevels> get levelsStream => _levelsController.stream;

  @override
  Stream<AudioFormatInfo> get formatStream => _formatController.stream;

  /// Push one analyser frame. Tests drive the spectrum through this rather than
  /// waiting on a timer, so they stay deterministic.
  void emitLevels(AudioLevels levels) => _levelsController.add(levels);

  void emitFormat(AudioFormatInfo info) {
    _bitrateKbps = info.bitrateKbps;
    _sampleRateHz = info.sampleRateHz;
    _channels = info.channels;
    _emitFormat();
  }

  /// Simulates a lone media_kit bitrate tick after a track change.
  void emitBitrate(int? kbps) {
    _bitrateKbps = kbps;
    _emitFormat();
  }

  @override
  Future<void> open(Track track) async {
    lastOpenedPath = track.path;
    openCount++;
    _hasMedia = true;
    _position = Duration.zero;
    _duration = track.duration ?? trackDuration;
    _positionController.add(_position);
    _durationController.add(_duration);
    _sampleRateHz = null;
    _channels = null;
    _bitrateKbps = null;
    _emitFormat();
  }

  @override
  Future<void> play() async {
    if (!_hasMedia) return;
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
    _hasMedia = false;
    lastOpenedPath = null;
    _position = Duration.zero;
    _duration = Duration.zero;
    _playingController.add(_playing);
    _positionController.add(_position);
    _durationController.add(_duration);
  }

  @override
  Future<void> seek(Duration position) async {
    _position = position;
    _positionController.add(_position);
  }

  @override
  Future<void> setVolume(double volume) async {}

  @override
  Future<void> setForceMono(bool enabled) async {
    forceMono = enabled;
    audioChannelsValues.add(enabled ? 'mono' : 'auto');
  }

  @override
  Future<void> dispose() async {
    await _positionController.close();
    await _durationController.close();
    await _playingController.close();
    await _completedController.close();
    await _errorController.close();
    await _levelsController.close();
    await _formatController.close();
  }

  Future<void> emitCompleted() async {
    _completedController.add(null);
  }
}
