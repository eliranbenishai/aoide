import 'equalizer_settings.dart';

/// Which panel occupies the region below the main player.
enum LowerRegion { equalizer, playlist }

/// Persisted UI state.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.lowerRegion,
    this.equalizer = EqualizerSettings.flat,
    this.playlistWindowWidth,
    this.playlistWindowHeight,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    lowerRegion: LowerRegion.playlist,
  );

  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final LowerRegion lowerRegion;
  final EqualizerSettings equalizer;
  final double? playlistWindowWidth;
  final double? playlistWindowHeight;

  TrampSettings copyWith({
    int? zoomPercent,
    LowerRegion? lowerRegion,
    EqualizerSettings? equalizer,
    double? playlistWindowWidth,
    double? playlistWindowHeight,
  }) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      lowerRegion: lowerRegion ?? this.lowerRegion,
      equalizer: equalizer ?? this.equalizer,
      playlistWindowWidth: playlistWindowWidth ?? this.playlistWindowWidth,
      playlistWindowHeight: playlistWindowHeight ?? this.playlistWindowHeight,
    );
  }

  Map<String, dynamic> toJson() {
    final json = <String, dynamic>{
      'zoomPercent': zoomPercent,
      'lowerRegion': lowerRegion.name,
      'equalizer': equalizer.toJson(),
    };
    if (playlistWindowWidth != null) {
      json['playlistWindowWidth'] = playlistWindowWidth;
    }
    if (playlistWindowHeight != null) {
      json['playlistWindowHeight'] = playlistWindowHeight;
    }
    return json;
  }

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
      playlistWindowWidth: _positiveDouble(json['playlistWindowWidth']),
      playlistWindowHeight: _positiveDouble(json['playlistWindowHeight']),
    );
  }

  static double? _positiveDouble(Object? value) {
    final n = switch (value) {
      num v => v.toDouble(),
      _ => null,
    };
    if (n == null || n <= 0 || !n.isFinite) return null;
    return n;
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.lowerRegion == lowerRegion &&
      other.equalizer == equalizer &&
      other.playlistWindowWidth == playlistWindowWidth &&
      other.playlistWindowHeight == playlistWindowHeight;

  @override
  int get hashCode => Object.hash(
        zoomPercent,
        lowerRegion,
        equalizer,
        playlistWindowWidth,
        playlistWindowHeight,
      );

  @override
  String toString() => 'TrampSettings(zoomPercent: $zoomPercent, '
      'lowerRegion: $lowerRegion, equalizer: $equalizer, '
      'playlistWindowWidth: $playlistWindowWidth, '
      'playlistWindowHeight: $playlistWindowHeight)';
}
