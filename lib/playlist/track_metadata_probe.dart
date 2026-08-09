import 'dart:async';

import 'package:media_kit/media_kit.dart' hide Track;

import '../domain/track.dart';

/// Fills missing track fields (duration / tags) without starting playback.
abstract class TrackMetadataProbe {
  Future<Track> enrich(Track track);
}

/// Probes duration (and basic tags when available) via a dedicated media_kit
/// [Player] so the transport engine is never disturbed.
class MediaKitTrackMetadataProbe implements TrackMetadataProbe {
  MediaKitTrackMetadataProbe({Player? player}) : _player = player;

  Player? _player;
  Future<void>? _queue;

  @override
  Future<Track> enrich(Track track) {
    final previous = _queue;
    final done = Completer<Track>();
    _queue = () async {
      if (previous != null) await previous;
      try {
        done.complete(await _enrichUnlocked(track));
      } catch (error, stack) {
        done.completeError(error, stack);
      }
    }();
    return done.future;
  }

  Future<Track> _enrichUnlocked(Track track) async {
    // M3U EXTINF (and prior probes) already know the length — skip I/O.
    if (track.duration != null && track.duration! > Duration.zero) {
      return track;
    }

    final player = _player ??= Player();
    try {
      await player.open(Media(track.path), play: false);
      final duration = await player.stream.duration
          .firstWhere((d) => d > Duration.zero)
          .timeout(const Duration(seconds: 8), onTimeout: () => Duration.zero);

      String? title;
      String? artist;
      String? album;
      final platform = player.platform;
      if (platform is NativePlayer) {
        title = _nonEmpty(await platform.getProperty('metadata/title'));
        artist = _nonEmpty(await platform.getProperty('metadata/artist'));
        album = _nonEmpty(await platform.getProperty('metadata/album'));
      }

      return track.copyWith(
        title: title ?? track.title,
        artist: artist ?? track.artist,
        album: album ?? track.album,
        duration: duration > Duration.zero ? duration : track.duration,
      );
    } catch (_) {
      return track;
    }
  }

  Future<void> dispose() async {
    if (_queue != null) {
      try {
        await _queue;
      } catch (_) {}
    }
    await _player?.dispose();
    _player = null;
  }

  String? _nonEmpty(String value) {
    final trimmed = value.trim();
    return trimmed.isEmpty ? null : trimmed;
  }
}
