/// Which panel occupies the region below the main player.
enum LowerRegion { equalizer, playlist }

/// Persisted UI state. Equalizer state joins this in Task 9.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.lowerRegion,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
  );

  /// Kept in step with `ZoomController.steps`; an out-of-range value on disk is
  /// treated as corrupt rather than adopted.
  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final LowerRegion lowerRegion;

  TrampSettings copyWith({int? zoomPercent, LowerRegion? lowerRegion}) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      lowerRegion: lowerRegion ?? this.lowerRegion,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'lowerRegion': lowerRegion.name,
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final region = json['lowerRegion'];
    return TrampSettings(
      zoomPercent: zoom is int && validZoomPercents.contains(zoom)
          ? zoom
          : defaults.zoomPercent,
      lowerRegion: LowerRegion.values.firstWhere(
        (value) => value.name == region,
        orElse: () => defaults.lowerRegion,
      ),
    );
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.lowerRegion == lowerRegion;

  @override
  int get hashCode => Object.hash(zoomPercent, lowerRegion);

  @override
  String toString() =>
      'TrampSettings(zoomPercent: $zoomPercent, lowerRegion: $lowerRegion)';
}
