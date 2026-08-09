import 'dart:convert';
import 'dart:io';

import 'package:archive/archive.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/look/look_installer.dart';

void main() {
  late Directory looksDir;
  late Directory sourceDir;
  LookConflictChoice? lastConflictChoice;
  LookConflict? lastConflict;

  Future<LookConflictChoice> onConflict(LookConflict conflict) async {
    lastConflict = conflict;
    return lastConflictChoice ?? LookConflictChoice.cancel;
  }

  LookInstaller installer() =>
      LookInstaller(looksDir: looksDir, onConflict: onConflict);

  setUp(() async {
    looksDir = Directory.systemTemp.createTempSync('tramp-install-looks');
    sourceDir = Directory.systemTemp.createTempSync('tramp-install-source');
    lastConflictChoice = null;
    lastConflict = null;
  });

  tearDown(() async {
    for (final dir in [looksDir, sourceDir]) {
      if (dir.existsSync()) {
        await dir.delete(recursive: true);
      }
    }
  });

  Map<String, dynamic> manifest({
    String id = 'neon-cyan',
    String name = 'Neon Cyan',
    String? author,
  }) =>
      {
        'formatVersion': 1,
        'id': id,
        'name': name,
        if (author != null) 'author': author,
        'extends': 'builtin',
      };

  Future<void> writeSourcePack({
    required Map<String, dynamic> manifestJson,
    bool withFonts = false,
  }) async {
    await File(p.join(sourceDir.path, 'look.json'))
        .writeAsString(jsonEncode(manifestJson));
    if (withFonts) {
      final fontsDir = Directory(p.join(sourceDir.path, 'fonts'));
      await fontsDir.create(recursive: true);
      await File(p.join(fontsDir.path, 'chrome.ttf'))
          .writeAsString('fake-font');
    }
  }

  Future<File> createZip(List<MapEntry<String, List<int>>> entries) async {
    final archive = Archive();
    for (final entry in entries) {
      archive.addFile(ArchiveFile(entry.key, entry.value.length, entry.value));
    }
    final encoded = ZipEncoder().encode(archive);
    expect(encoded, isNotNull);
    final zipFile = File(p.join(sourceDir.path, 'pack.zip'));
    await zipFile.writeAsBytes(encoded);
    return zipFile;
  }

  group('LookInstaller.installDirectory', () {
    test('fresh install copies look.json and fonts to looksDir/id', () async {
      await writeSourcePack(
        manifestJson: manifest(author: 'Alice'),
        withFonts: true,
      );

      final installed = await installer().installDirectory(sourceDir);

      expect(installed, isTrue);
      final target = Directory(p.join(looksDir.path, 'neon-cyan'));
      expect(await File(p.join(target.path, 'look.json')).exists(), isTrue);
      expect(await File(p.join(target.path, 'fonts', 'chrome.ttf')).exists(),
          isTrue);
      expect(lastConflict, isNull);
    });

    test('conflict and cancel leaves existing files unchanged', () async {
      final existing = Directory(p.join(looksDir.path, 'neon-cyan'));
      await existing.create(recursive: true);
      await File(p.join(existing.path, 'look.json')).writeAsString(
        jsonEncode(manifest(name: 'Existing', author: 'Bob')),
      );
      await File(p.join(existing.path, 'marker.txt'))
          .writeAsString('keep-me');

      await writeSourcePack(
        manifestJson: manifest(name: 'Incoming', author: 'Alice'),
      );
      lastConflictChoice = LookConflictChoice.cancel;

      final installed = await installer().installDirectory(sourceDir);

      expect(installed, isFalse);
      expect(lastConflict!.id, 'neon-cyan');
      expect(lastConflict!.installedName, 'Existing');
      expect(lastConflict!.installedAuthor, 'Bob');
      expect(lastConflict!.incomingName, 'Incoming');
      expect(lastConflict!.incomingAuthor, 'Alice');
      expect(await File(p.join(existing.path, 'marker.txt')).readAsString(),
          'keep-me');
      final manifestJson =
          jsonDecode(await File(p.join(existing.path, 'look.json')).readAsString())
              as Map<String, dynamic>;
      expect(manifestJson['name'], 'Existing');
    });

    test('conflict and replace overwrites existing pack', () async {
      final existing = Directory(p.join(looksDir.path, 'neon-cyan'));
      await existing.create(recursive: true);
      await File(p.join(existing.path, 'look.json')).writeAsString(
        jsonEncode(manifest(name: 'Existing')),
      );
      await File(p.join(existing.path, 'old.txt')).writeAsString('gone');

      await writeSourcePack(
        manifestJson: manifest(name: 'Incoming', author: 'Alice'),
        withFonts: true,
      );
      lastConflictChoice = LookConflictChoice.replace;

      final installed = await installer().installDirectory(sourceDir);

      expect(installed, isTrue);
      final manifestJson = jsonDecode(
        await File(p.join(existing.path, 'look.json')).readAsString(),
      ) as Map<String, dynamic>;
      expect(manifestJson['name'], 'Incoming');
      expect(await File(p.join(existing.path, 'old.txt')).exists(), isFalse);
      expect(await File(p.join(existing.path, 'fonts', 'chrome.ttf')).exists(),
          isTrue);
    });

    test('rejects installing id builtin', () async {
      await writeSourcePack(manifestJson: manifest(id: 'builtin', name: 'Builtin'));

      expect(
        () => installer().installDirectory(sourceDir),
        throwsA(isA<FormatException>()),
      );
    });
  });

  group('LookInstaller.installZip', () {
    test('zip with root look.json installs under id', () async {
      final lookJson = utf8.encode(jsonEncode(manifest()));
      final zipFile = await createZip([
        MapEntry('look.json', lookJson),
      ]);

      final installed = await installer().installZip(zipFile);

      expect(installed, isTrue);
      expect(
        await File(p.join(looksDir.path, 'neon-cyan', 'look.json')).exists(),
        isTrue,
      );
    });

    test('zip with single root folder installs', () async {
      final lookJson = utf8.encode(jsonEncode(manifest()));
      final zipFile = await createZip([
        MapEntry('neon-cyan/look.json', lookJson),
      ]);

      final installed = await installer().installZip(zipFile);

      expect(installed, isTrue);
      expect(
        await File(p.join(looksDir.path, 'neon-cyan', 'look.json')).exists(),
        isTrue,
      );
    });

    test('zip with ../ escape does not write outside temp and fails', () async {
      final lookJson = utf8.encode(jsonEncode(manifest()));
      final markerName =
          'tramp-zip-slip-${DateTime.now().microsecondsSinceEpoch}.txt';
      final marker = File(p.join(Directory.systemTemp.path, markerName));
      addTearDown(() {
        if (marker.existsSync()) marker.deleteSync();
      });

      final zipFile = await createZip([
        MapEntry('look.json', lookJson),
        MapEntry('../$markerName', utf8.encode('pwned')),
      ]);

      await expectLater(
        installer().installZip(zipFile),
        throwsA(
          isA<FormatException>().having(
            (e) => e.message,
            'message',
            contains('escapes pack root'),
          ),
        ),
      );
      expect(marker.existsSync(), isFalse);
      expect(
        Directory(p.join(looksDir.path, 'neon-cyan')).existsSync(),
        isFalse,
      );
    });
  });
}
