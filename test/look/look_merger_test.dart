import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/look_manifest.dart';
import 'package:tramp/look/look_merger.dart';
import 'package:tramp/look/look_parser.dart';
import 'package:tramp/theme/mockup_tokens.dart';

void main() {
  test('partial overlay keeps builtin shell colors', () {
    final neon = LookParser.parse({
      'formatVersion': 1,
      'id': 'neon-cyan',
      'name': 'Neon Cyan',
      'extends': 'builtin',
      'colors': {
        'phosphor': {'default': '#3de7ff'},
        'accent': {'default': '#ff3d9a'},
      },
    });
    final resolved = LookMerger.resolve(
      activeId: 'neon-cyan',
      installed: {'neon-cyan': neon},
    );
    expect(resolved.palette.shellHighlight, MockupTokens.shellHi);
    expect(resolved.palette.phosphorDefault, const Color(0xFF3DE7FF));
  });

  test('stops replace wholly', () {
    final overlay = LookParser.parse({
      'formatVersion': 1,
      'id': 'custom',
      'name': 'Custom',
      'extends': 'builtin',
      'materials': {
        'spectrum': {
          'stops': ['#ff0000', '#00ff00'],
        },
      },
    });
    final resolved = LookMerger.resolve(
      activeId: 'custom',
      installed: {'custom': overlay},
    );
    expect(resolved.materials.spectrumStops.length, 2);
    expect(resolved.materials.spectrumStops[0], const Color(0xFFFF0000));
    expect(resolved.materials.spectrumStops[1], const Color(0xFF00FF00));
  });

  test('rejects cycle', () {
    final a = LookManifest(
      formatVersion: 1,
      id: 'a',
      name: 'A',
      extendsId: 'b',
    );
    final b = LookManifest(
      formatVersion: 1,
      id: 'b',
      name: 'B',
      extendsId: 'a',
    );
    expect(
      () => LookMerger.resolve(activeId: 'a', installed: {'a': a, 'b': b}),
      throwsFormatException,
    );
  });

  test('rejects chain deeper than 8', () {
    final installed = <String, LookManifest>{};
    for (var i = 0; i < 9; i++) {
      installed['pack-$i'] = LookManifest(
        formatVersion: 1,
        id: 'pack-$i',
        name: 'Pack $i',
        extendsId: i == 0 ? 'builtin' : 'pack-${i - 1}',
      );
    }
    expect(
      () => LookMerger.resolve(activeId: 'pack-8', installed: installed),
      throwsFormatException,
    );
  });
}
