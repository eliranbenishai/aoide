import 'look_manifest.dart';
import 'look_parser.dart';

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
}
