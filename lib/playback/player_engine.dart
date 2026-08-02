import '../domain/track.dart';
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

  Future<void> open(Track track);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> seek(Duration position);
  Future<void> setVolume(double volume);
  Future<void> dispose();
}
