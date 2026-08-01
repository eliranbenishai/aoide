import 'dart:async';

import 'package:media_kit/media_kit.dart' hide Track;

import '../domain/track.dart';
import 'player_engine.dart';

typedef TrackMetadataCallback = void Function(
  String path,
  Track Function(Track) update,
);

class MediaKitPlayerEngine implements PlayerEngine {
  MediaKitPlayerEngine({this.onMetadata, Player? player})
      : _player = player ?? Player();

  final Player _player;
  final TrackMetadataCallback? onMetadata;

  @override
  Stream<Duration> get positionStream => _player.stream.position;

  @override
  Stream<Duration> get durationStream => _player.stream.duration;

  @override
  Stream<bool> get playingStream => _player.stream.playing;

  @override
  Stream<void> get completedStream => _player.stream.completed
      .where((completed) => completed)
      .map((_) {});

  @override
  Future<void> open(Track track) async {
    await _player.open(Media(track.path), play: false);
    unawaited(_tryEmitMetadata(track));
  }

  @override
  Future<void> play() => _player.play();

  @override
  Future<void> pause() => _player.pause();

  @override
  Future<void> stop() => _player.stop();

  @override
  Future<void> seek(Duration position) => _player.seek(position);

  @override
  Future<void> setVolume(double volume) =>
      _player.setVolume(volume.clamp(0.0, 1.0) * 100);

  @override
  Future<void> dispose() => _player.dispose();

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
