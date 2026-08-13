import 'dart:async';

import 'package:media_kit/media_kit.dart' hide Track;

import '../analysis/spectrum_analyzer.dart';
import '../domain/track.dart';
import 'audio_format_info.dart';
import 'audio_levels.dart';
import 'mono_controller.dart';
import 'player_engine.dart';

typedef TrackMetadataCallback = void Function(
  String path,
  Track Function(Track) update,
);

class MediaKitPlayerEngine implements PlayerEngine {
  MediaKitPlayerEngine({
    this.onMetadata,
    Player? player,
    SpectrumAnalyzer? spectrumAnalyzer,
  })  : _player = player ?? Player(),
        _spectrumAnalyzer = spectrumAnalyzer ?? SpectrumAnalyzer() {
    _mono = MonoController(setProperty: _setNativeProperty);
    _playingSubscription = _player.stream.playing.listen((playing) {
      _isPlaying = playing;
    });
    _paramsSubscription = _player.stream.audioParams.listen((params) {
      _sampleRateHz = params.sampleRate;
      _channels = params.channelCount;
      _emitFormat();
    });
    _bitrateSubscription = _player.stream.audioBitrate.listen((bitrate) {
      _bitrateKbps = bitrate == null ? null : (bitrate / 1000).round();
      _emitFormat();
    });
  }

  final Player _player;
  final TrackMetadataCallback? onMetadata;
  final SpectrumAnalyzer _spectrumAnalyzer;
  late final MonoController _mono;

  final _levels = StreamController<AudioLevels>.broadcast();
  final _format = StreamController<AudioFormatInfo>.broadcast();
  StreamSubscription<bool>? _playingSubscription;
  StreamSubscription<dynamic>? _paramsSubscription;
  StreamSubscription<dynamic>? _bitrateSubscription;
  StreamSubscription<AudioLevels>? _analyzerSubscription;
  bool _isPlaying = false;
  bool _hasMedia = false;
  double _currentVolume = 1;
  int? _sampleRateHz;
  int? _channels;
  int? _bitrateKbps;
  bool _forceMono = false;

  @override
  Stream<AudioLevels> get levelsStream => _levels.stream;

  @override
  Stream<AudioFormatInfo> get formatStream => _format.stream;

  void _emitFormat() {
    if (_format.isClosed) return;
    _format.add(AudioFormatInfo(
      bitrateKbps: _bitrateKbps,
      sampleRateHz: _sampleRateHz,
      channels: _channels,
    ));
  }

  Future<void> _setNativeProperty(String name, String value) async {
    final platform = _player.platform;
    if (platform is! NativePlayer) return;
    await platform.setProperty(name, value);
  }

  void _attachAnalyzer(String path) {
    unawaited(_analyzerSubscription?.cancel());
    _analyzerSubscription = _spectrumAnalyzer
        .attach(
          path: path,
          playing: _player.stream.playing,
          position: _player.stream.position,
        )
        .listen((frame) {
      if (_levels.isClosed) return;
      // Product path: never publish synthetic:true during normal play.
      if (frame.synthetic && _isPlaying && _hasMedia) {
        _levels.add(AudioLevels.silent);
        return;
      }
      _levels.add(frame);
    });
  }

  @override
  Stream<Duration> get positionStream => _player.stream.position;

  @override
  Stream<Duration> get durationStream => _player.stream.duration;

  @override
  Stream<bool> get playingStream => _player.stream.playing;

  @override
  Stream<String> get errorStream => _player.stream.error;

  @override
  Stream<void> get completedStream => _player.stream.completed
      .where((completed) => completed)
      .map((_) {});

  @override
  Future<void> open(Track track) async {
    _sampleRateHz = null;
    _channels = null;
    _bitrateKbps = null;
    _emitFormat();
    await _player.open(Media(track.path), play: false);
    _hasMedia = true;
    _attachAnalyzer(track.path);
    // Re-apply mono after open so mpv keeps the downmix for the new file.
    await setForceMono(_forceMono);
    unawaited(_tryEmitMetadata(track));
  }

  @override
  Future<void> play() => _player.play();

  @override
  Future<void> pause() => _player.pause();

  @override
  Future<void> stop() async {
    _hasMedia = false;
    _isPlaying = false;
    await _analyzerSubscription?.cancel();
    _analyzerSubscription = null;
    if (!_levels.isClosed) {
      _levels.add(AudioLevels.silent);
    }
    await _player.stop();
  }

  @override
  Future<void> seek(Duration position) => _player.seek(position);

  @override
  Future<void> setVolume(double volume) {
    _currentVolume = volume.clamp(0.0, 1.0);
    return _player.setVolume(_currentVolume * 100);
  }

  @override
  Future<void> setForceMono(bool enabled) async {
    _forceMono = enabled;
    await _mono.setForceMono(enabled);
  }

  @override
  Future<void> dispose() async {
    await _analyzerSubscription?.cancel();
    await _playingSubscription?.cancel();
    await _paramsSubscription?.cancel();
    await _bitrateSubscription?.cancel();
    await _format.close();
    await _levels.close();
    await _player.dispose();
  }

  Future<void> _tryEmitMetadata(Track track) async {
    final callback = onMetadata;
    if (callback == null) return;

    try {
      final platform = _player.platform;
      if (platform is! NativePlayer) return;

      final title = _nonEmpty(await platform.getProperty('metadata/title'));
      final artist = _nonEmpty(await platform.getProperty('metadata/artist'));
      final album = _nonEmpty(await platform.getProperty('metadata/album'));
      final duration = _player.state.duration;

      if (title == null &&
          artist == null &&
          album == null &&
          duration <= Duration.zero) {
        return;
      }

      callback(track.path, (current) {
        return current.copyWith(
          title: title ?? current.title,
          artist: artist ?? current.artist,
          album: album ?? current.album,
          duration: duration > Duration.zero ? duration : current.duration,
        );
      });
    } catch (_) {
      // Tag read failure must not block playback.
    }
  }

  String? _nonEmpty(String value) {
    final trimmed = value.trim();
    return trimmed.isEmpty ? null : trimmed;
  }
}
