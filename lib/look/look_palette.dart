import 'package:flutter/painting.dart';

import 'look_color.dart';

class LookPalette {
  const LookPalette({
    required this.shellHighlight,
    required this.shellBase,
    required this.shellMid,
    required this.shellLow,
    required this.shellDeep,
    required this.inkDefault,
    required this.inkDim,
    required this.inkFaint,
    required this.phosphorDefault,
    required this.phosphorHot,
    required this.phosphorDim,
    required this.phosphorDeep,
    required this.accentDefault,
    required this.accentDim,
    required this.well,
  });

  final Color shellHighlight;
  final Color shellBase;
  final Color shellMid;
  final Color shellLow;
  final Color shellDeep;
  final Color inkDefault;
  final Color inkDim;
  final Color inkFaint;
  final Color phosphorDefault;
  final Color phosphorHot;
  final Color phosphorDim;
  final Color phosphorDeep;
  final Color accentDefault;
  final Color accentDim;
  final Color well;

  factory LookPalette.fromMergedColors(Map<String, dynamic> colors) {
    Color groupColor(String group, String key) {
      final g = colors[group];
      if (g is! Map || g[key] is! String) {
        throw FormatException('missing color: $group.$key');
      }
      return lookColorFromHex(g[key] as String);
    }

    Color scalarColor(String key) {
      final value = colors[key];
      if (value is! String) {
        throw FormatException('missing color: $key');
      }
      return lookColorFromHex(value);
    }

    return LookPalette(
      shellHighlight: groupColor('shell', 'highlight'),
      shellBase: groupColor('shell', 'base'),
      shellMid: groupColor('shell', 'mid'),
      shellLow: groupColor('shell', 'low'),
      shellDeep: groupColor('shell', 'deep'),
      inkDefault: groupColor('ink', 'default'),
      inkDim: groupColor('ink', 'dim'),
      inkFaint: groupColor('ink', 'faint'),
      phosphorDefault: groupColor('phosphor', 'default'),
      phosphorHot: groupColor('phosphor', 'hot'),
      phosphorDim: groupColor('phosphor', 'dim'),
      phosphorDeep: groupColor('phosphor', 'deep'),
      accentDefault: groupColor('accent', 'default'),
      accentDim: groupColor('accent', 'dim'),
      well: scalarColor('well'),
    );
  }

  Map<String, dynamic> toJson() => {
        'shell': {
          'highlight': lookColorToHex(shellHighlight),
          'base': lookColorToHex(shellBase),
          'mid': lookColorToHex(shellMid),
          'low': lookColorToHex(shellLow),
          'deep': lookColorToHex(shellDeep),
        },
        'ink': {
          'default': lookColorToHex(inkDefault),
          'dim': lookColorToHex(inkDim),
          'faint': lookColorToHex(inkFaint),
        },
        'phosphor': {
          'default': lookColorToHex(phosphorDefault),
          'hot': lookColorToHex(phosphorHot),
          'dim': lookColorToHex(phosphorDim),
          'deep': lookColorToHex(phosphorDeep),
        },
        'accent': {
          'default': lookColorToHex(accentDefault),
          'dim': lookColorToHex(accentDim),
        },
        'well': lookColorToHex(well),
      };

  factory LookPalette.fromJson(Map<String, dynamic> json) =>
      LookPalette.fromMergedColors(json);
}
