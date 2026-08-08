/// Ten-band equalizer state.
///
/// Persisted and displayed in EQ chrome; when the host wires an mpv equalizer
/// sink, gains reach audio via `buildEqualizerAf` / lavfi. Audibility is
/// measurement-gated (`tool/eq_measure.dart`) on full libmpv (ADR 0005).
class EqualizerSettings {
  const EqualizerSettings({
    required this.enabled,
    required this.auto,
    required this.preamp,
    required this.gains,
    this.presetName,
  });

  /// Winamp 2.x band centres, in hertz.
  static const List<int> bandFrequencies = [
    60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000,
  ];

  static const double gainLimit = 12;

  static const EqualizerSettings flat = EqualizerSettings(
    enabled: false,
    auto: false,
    preamp: 0,
    gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  );

  final bool enabled;
  final bool auto;
  final double preamp;
  final List<double> gains;
  final String? presetName;

  static double _clamp(double value) =>
      value.clamp(-gainLimit, gainLimit).toDouble();

  EqualizerSettings copyWith({
    bool? enabled,
    bool? auto,
    double? preamp,
    List<double>? gains,
    String? presetName,
    bool clearPresetName = false,
  }) {
    return EqualizerSettings(
      enabled: enabled ?? this.enabled,
      auto: auto ?? this.auto,
      preamp: preamp ?? this.preamp,
      gains: gains ?? this.gains,
      presetName: clearPresetName ? null : (presetName ?? this.presetName),
    );
  }

  /// Editing a band makes the curve no longer the named preset.
  EqualizerSettings withGain(int band, double gain) {
    if (band < 0 || band >= bandFrequencies.length) return this;
    final next = List<double>.of(gains);
    next[band] = _clamp(gain);
    return copyWith(gains: next, clearPresetName: true);
  }

  EqualizerSettings withPreamp(double value) =>
      copyWith(preamp: _clamp(value));

  Map<String, dynamic> toJson() => {
        'enabled': enabled,
        'auto': auto,
        'preamp': preamp,
        'gains': gains,
        'presetName': presetName,
      };

  factory EqualizerSettings.fromJson(Map<String, dynamic> json) {
    final rawGains = json['gains'];
    if (rawGains is! List || rawGains.length != bandFrequencies.length) {
      return flat;
    }
    final gains = <double>[];
    for (final value in rawGains) {
      if (value is! num) return flat;
      gains.add(_clamp(value.toDouble()));
    }
    final preamp = json['preamp'];
    final presetName = json['presetName'];
    return EqualizerSettings(
      enabled: json['enabled'] == true,
      auto: json['auto'] == true,
      preamp: preamp is num ? _clamp(preamp.toDouble()) : 0,
      gains: gains,
      presetName: presetName is String ? presetName : null,
    );
  }

  @override
  bool operator ==(Object other) {
    if (other is! EqualizerSettings) return false;
    if (other.enabled != enabled ||
        other.auto != auto ||
        other.preamp != preamp ||
        other.presetName != presetName ||
        other.gains.length != gains.length) {
      return false;
    }
    for (var i = 0; i < gains.length; i++) {
      if (other.gains[i] != gains[i]) return false;
    }
    return true;
  }

  @override
  int get hashCode =>
      Object.hash(enabled, auto, preamp, presetName, Object.hashAll(gains));

  @override
  String toString() => 'EqualizerSettings(enabled: $enabled, auto: $auto, '
      'preamp: $preamp, gains: $gains, presetName: $presetName)';
}

/// Built-in curves offered by the PRESETS menu.
abstract final class EqualizerPresets {
  static const Map<String, List<double>> builtIn = {
    'Flat': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    'Rock': [5, 4, 2, -1, -2, 1, 3, 5, 5, 5],
    'Pop': [-1, 2, 4, 5, 3, 0, -1, -1, -1, -1],
    'Jazz': [4, 3, 1, 2, -1, -1, 0, 2, 3, 4],
    'Classical': [5, 4, 3, 2, -1, -1, 0, 2, 3, 4],
    'Bass Boost': [9, 7, 5, 2, 0, 0, 0, 0, 0, 0],
    'Treble Boost': [0, 0, 0, 0, 0, 2, 5, 7, 8, 8],
    'Vocal': [-2, -1, 2, 4, 5, 4, 2, 0, -1, -2],
  };
}
