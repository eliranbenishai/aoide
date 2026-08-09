import 'package:flutter/painting.dart';

import '../theme/mockup_tokens.dart';
import 'look_manifest.dart';
import 'look_materials.dart';
import 'look_palette.dart';
import 'look_parser.dart';
import 'resolved_look.dart';

abstract final class BuiltinLook {
  static const _manifestJson = {
    'formatVersion': 1,
    'id': 'builtin',
    'name': 'Builtin',
    'extends': 'builtin',
    'colors': {
      'shell': {
        'highlight': '#323744',
        'base': '#262b38',
        'mid': '#1a1d26',
        'low': '#12141a',
        'deep': '#0a0b0e',
      },
      'ink': {
        'default': '#e8eaf0',
        'dim': '#8b919e',
        'faint': '#5b6270',
      },
      'phosphor': {
        'default': '#3de7ff',
        'hot': '#b8f6ff',
        'dim': '#1a7a88',
        'deep': '#0d3d46',
      },
      'accent': {
        'default': '#ff3d9a',
        'dim': '#8a2258',
      },
      'well': '#050608',
    },
    'materials': {
      'bevel': {
        'lightOpacity': 0.15,
        'softOpacity': 0.06,
      },
      'spectrum': {
        'stops': ['#cbf9ff', '#3de7ff', '#1b9ec4', '#ff3d9a'],
      },
      'rail': {
        'stops': ['#1a7a88', '#8a2258', '#1a7a88'],
      },
    },
  };

  static final LookManifest manifest =
      LookParser.parse(_manifestJson, allowBuiltin: true);

  /// Compile-time builtin look.
  ///
  /// Palette/materials Colors are the same [MockupTokens] / const instances
  /// chrome used before LookScope, so builtin goldens stay pixel-identical.
  static final ResolvedLook resolved = ResolvedLook(
    id: 'builtin',
    name: 'Builtin',
    palette: const LookPalette(
      shellHighlight: MockupTokens.shellHi,
      shellBase: MockupTokens.shell,
      shellMid: MockupTokens.shellMid,
      shellLow: MockupTokens.shellLo,
      shellDeep: MockupTokens.shellDeep,
      inkDefault: MockupTokens.ink,
      inkDim: MockupTokens.inkDim,
      inkFaint: MockupTokens.inkFaint,
      phosphorDefault: MockupTokens.phos,
      phosphorHot: MockupTokens.phosHot,
      phosphorDim: MockupTokens.phosDim,
      phosphorDeep: MockupTokens.phosDeep,
      accentDefault: MockupTokens.accent,
      accentDim: MockupTokens.accentDim,
      well: MockupTokens.well,
    ),
    materials: const LookMaterials(
      bevelLightOpacity: 0.15,
      bevelSoftOpacity: 0.06,
      spectrumStops: [
        Color(0xFFCBF9FF),
        MockupTokens.phos,
        Color(0xFF1B9EC4),
        MockupTokens.accent,
      ],
      railStops: [
        MockupTokens.phosDim,
        MockupTokens.accentDim,
        MockupTokens.phosDim,
      ],
    ),
    chromeFamily: 'TrampCondensed',
    lcdFamily: 'TrampMono',
  );
}
