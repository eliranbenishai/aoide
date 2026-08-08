import 'dart:math' as math;

/// One frame of analyser data.
///
/// [synthetic] is a hard-fail / dev signal for fabricated shapes only. Normal
/// product playback publishes measured frames (`synthetic: false`), including
/// honest silence.
class AudioLevels {
  AudioLevels({
    required this.bands,
    required this.leftRms,
    required this.rightRms,
    required this.synthetic,
  }) : assert(
          bands.length == bandCount,
          'expected $bandCount bands, got ${bands.length}',
        );

  /// Number of bars the display shows.
  static const int bandCount = 20;

  /// Measured / no-signal silence. Not a fabricated spectrum shape.
  static final AudioLevels silent = AudioLevels(
    bands: List<double>.filled(bandCount, 0),
    leftRms: 0,
    rightRms: 0,
    synthetic: false,
  );

  final List<double> bands;
  final double leftRms;
  final double rightRms;
  final bool synthetic;

  /// A plausible spectrum shape for use until real analysis is available.
  ///
  /// Deterministic in [seed] so tests and goldens are stable. Energy falls off
  /// towards the treble, which is what music generally does.
  factory AudioLevels.synthesised({
    required double intensity,
    required int seed,
  }) {
    final clamped = intensity.clamp(0.0, 1.0);
    if (clamped == 0) return silent;

    final random = math.Random(seed);
    final bands = List<double>.generate(bandCount, (i) {
      final tilt = 1 - (i / bandCount) * 0.65;
      final jitter = 0.55 + random.nextDouble() * 0.45;
      return (clamped * tilt * jitter).clamp(0.0, 1.0);
    });

    return AudioLevels(
      bands: bands,
      leftRms: clamped * (0.7 + random.nextDouble() * 0.3),
      rightRms: clamped * (0.7 + random.nextDouble() * 0.3),
      synthetic: true,
    );
  }
}
