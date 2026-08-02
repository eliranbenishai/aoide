import 'equalizer_settings.dart';

/// Which panel occupies the region below the main player.
enum LowerRegion { equalizer, playlist }

/// Persisted UI state.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.lowerRegion,
    this.equalizer = EqualizerSettings.flat,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
  );

  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final LowerRegion lowerRegion;
  final EqualizerSettings equalizer;

  TrampSettings copyWith({
    int? zoomPercent,
    LowerRegion? lowerRegion,
    EqualizerSettings? equalizer,
  }) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      lowerRegion: lowerRegion ?? this.lowerRegion,
      equalizer: equalizer ?? this.equalizer,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'lowerRegion': lowerRegion.name,
        'equalizer': equalizer.toJson(),
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final region = json['lowerRegion'];
    final eq = json['equalizer'];
    return TrampSettings(
      zoomPercent: zoom is int && validZoomPercents.contains(zoom)
          ? zoom
          : defaults.zoomPercent,
      lowerRegion: LowerRegion.values.firstWhere(
        (value) => value.name == region,
        orElse: () => defaults.lowerRegion,
      ),
      equalizer: eq is Map<String, dynamic>
          ? EqualizerSettings.fromJson(eq)
          : EqualizerSettings.flat,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.lowerRegion == lowerRegion &&
      other.equalizer == equalizer;

  @override
  int get hashCode => Object.hash(zoomPercent, lowerRegion, equalizer);

  @override
  String toString() => 'TrampSettings(zoomPercent: $zoomPercent, '
      'lowerRegion: $lowerRegion, equalizer: $equalizer)';
}
