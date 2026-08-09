import 'package:flutter/painting.dart';

import 'look_color.dart';

class LookMaterials {
  const LookMaterials({
    required this.bevelLightOpacity,
    required this.bevelSoftOpacity,
    required this.spectrumStops,
    required this.railStops,
  });

  final double bevelLightOpacity;
  final double bevelSoftOpacity;
  final List<Color> spectrumStops;
  final List<Color> railStops;

  factory LookMaterials.fromMergedMaterials(Map<String, dynamic> materials) {
    num opacity(String group, String key) {
      final g = materials[group];
      if (g is! Map || g[key] is! num) {
        throw FormatException('missing material: $group.$key');
      }
      return g[key] as num;
    }

    List<Color> stops(String group) {
      final g = materials[group];
      if (g is! Map || g['stops'] is! List) {
        throw FormatException('missing material: $group.stops');
      }
      return [
        for (final stop in g['stops'] as List)
          lookColorFromHex(stop as String),
      ];
    }

    return LookMaterials(
      bevelLightOpacity: opacity('bevel', 'lightOpacity').toDouble(),
      bevelSoftOpacity: opacity('bevel', 'softOpacity').toDouble(),
      spectrumStops: stops('spectrum'),
      railStops: stops('rail'),
    );
  }
}
