import '../domain/track.dart';
import 'audio_format_info.dart';
import 'audio_levels.dart';

abstract class PlayerEngine {
  Stream<Duration> get positionStream;
  Stream<Duration> get durationStream;
  Stream<bool> get playingStream;
  Stream<void> get completedStream;

  /// Playback failures, as the engine words them.
  ///
  /// Opening a file the engine cannot play is not an error at the call site —
  /// [open] and [play] both accept it and return — so this is the only place
  /// the failure surfaces. A consumer that ignores it will show a silent
  /// transport as a playing one.
  Stream<String> get errorStream;

  /// Analyser frames for the spectrum display.
  ///
  /// Normal play must publish measured frames (`synthetic: false`). Fabricated
  /// shapes may only use `synthetic: true` as a hard-fail / dev signal.
  Stream<AudioLevels> get levelsStream;

  /// Stream properties of the open track.
  ///
  /// Event-driven: emits when format data changes (including reset to
  /// [AudioFormatInfo.unknown] on [open]). Consumers should treat no event yet
  /// as [AudioFormatInfo.unknown].
  Stream<AudioFormatInfo> get formatStream;

  Future<void> open(Track track);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> seek(Duration position);
  Future<void> setVolume(double volume);

  /// Force stereo→mono downmix (`audio-channels=mono` / `auto`).
  Future<void> setForceMono(bool enabled);

  Future<void> dispose();
}
