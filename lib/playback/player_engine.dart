import '../domain/track.dart';
import 'audio_format_info.dart';
import 'audio_levels.dart';

abstract class PlayerEngine {
  Stream<Duration> get positionStream;
  Stream<Duration> get durationStream;
  Stream<bool> get playingStream;
  Stream<void> get completedStream;

  /// Analyser frames for the spectrum display.
  ///
  /// Implementations that cannot measure real audio must emit frames flagged
  /// `synthetic: true` rather than silently fabricating measured-looking data.
  Stream<AudioLevels> get levelsStream;

  /// Stream properties of the open track. Emits [AudioFormatInfo.unknown] until
  /// decoding reports real values.
  Stream<AudioFormatInfo> get formatStream;

  Future<void> open(Track track);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> seek(Duration position);
  Future<void> setVolume(double volume);
  Future<void> dispose();
}
