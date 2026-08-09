import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/look_parser.dart';

void main() {
  test('parses partial overlay', () {
    final m = LookParser.parse({
      'formatVersion': 1,
      'id': 'neon-cyan',
      'name': 'Neon Cyan',
      'author': 'Example',
      'extends': 'builtin',
      'colors': {
        'phosphor': {'default': '#3de7ff'},
      },
    });
    expect(m.id, 'neon-cyan');
    expect(m.extendsId, 'builtin');
    expect(m.colors['phosphor'], isNotNull);
  });

  test('rejects bad id', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'com.example.neon',
        'name': 'X',
        'extends': 'builtin',
      }),
      throwsFormatException,
    );
  });

  test('rejects unknown color key', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'X',
        'extends': 'builtin',
        'colors': {'neon': '#fff'},
      }),
      throwsFormatException,
    );
  });

  test('rejects absolute font file paths', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'X',
        'extends': 'builtin',
        'fonts': {
          'lcd': {'file': '/tmp/evil.ttf'},
        },
      }),
      throwsA(
        isA<FormatException>().having(
          (e) => e.message,
          'message',
          contains('absolute'),
        ),
      ),
    );
  });

  test('rejects font file paths with .. segments', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'X',
        'extends': 'builtin',
        'fonts': {
          'lcd': {'file': 'fonts/../../evil.ttf'},
        },
      }),
      throwsA(
        isA<FormatException>().having(
          (e) => e.message,
          'message',
          contains('..'),
        ),
      ),
    );
  });
}
