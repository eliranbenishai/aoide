import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/look_catalog.dart';

void main() {
  late Directory looksDir;

  setUp(() async {
    looksDir = Directory.systemTemp.createTempSync('tramp-looks-test');
  });

  tearDown(() async {
    if (looksDir.existsSync()) {
      await looksDir.delete(recursive: true);
    }
  });

  Future<void> writePack(String folderName, Map<String, dynamic> manifest) async {
    final packDir = Directory('${looksDir.path}/$folderName');
    await packDir.create(recursive: true);
    await File('${packDir.path}/look.json')
        .writeAsString(jsonEncode(manifest));
  }

  group('LookCatalog.scan', () {
    test('empty directory yields empty map and no warnings', () async {
      final result = await const LookCatalog().scan(looksDir);
      expect(result.manifests, isEmpty);
      expect(result.warnings, isEmpty);
    });

    test('valid pack with matching folder name is indexed by id', () async {
      await writePack('neon-cyan', {
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'Neon Cyan',
        'extends': 'builtin',
      });

      final result = await const LookCatalog().scan(looksDir);
      expect(result.manifests, hasLength(1));
      expect(result.manifests['neon-cyan']!.name, 'Neon Cyan');
      expect(result.warnings, isEmpty);
    });

    test('mismatched folder name vs manifest id is skipped with warning', () async {
      await writePack('wrong-folder', {
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'Neon Cyan',
        'extends': 'builtin',
      });

      final result = await const LookCatalog().scan(looksDir);
      expect(result.manifests, isEmpty);
      expect(result.warnings, hasLength(1));
      expect(result.warnings.single, contains('wrong-folder'));
      expect(result.warnings.single, contains('neon-cyan'));
    });

    test('invalid look.json is skipped with warning', () async {
      final packDir = Directory('${looksDir.path}/bad-pack');
      await packDir.create(recursive: true);
      await File('${packDir.path}/look.json').writeAsString('{not json');

      final result = await const LookCatalog().scan(looksDir);
      expect(result.manifests, isEmpty);
      expect(result.warnings, isNotEmpty);
    });
  });

  group('LookCatalog.defaultLooksDirectory', () {
    test('returns supportDir/looks', () async {
      final support = Directory.systemTemp.createTempSync('tramp-support');
      addTearDown(() {
        if (support.existsSync()) support.deleteSync(recursive: true);
      });

      final looks = await LookCatalog.defaultLooksDirectory(() async => support);
      expect(looks.path, '${support.path}${Platform.pathSeparator}looks');
    });
  });
}
