import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import 'look_manifest.dart';
import 'look_parser.dart';

class LookCatalogResult {
  const LookCatalogResult(this.manifests, this.warnings);

  final Map<String, LookManifest> manifests;
  final List<String> warnings;
}

class LookCatalog {
  const LookCatalog();

  Future<LookCatalogResult> scan(Directory looksDir) async {
    final manifests = <String, LookManifest>{};
    final warnings = <String>[];

    if (!await looksDir.exists()) {
      return LookCatalogResult(manifests, warnings);
    }

    await for (final entity in looksDir.list(followLinks: false)) {
      if (entity is! Directory) continue;

      final folderName = p.basename(entity.path);
      final lookFile = File(p.join(entity.path, 'look.json'));
      if (!await lookFile.exists()) continue;

      try {
        final decoded = jsonDecode(await lookFile.readAsString());
        if (decoded is! Map) {
          warnings.add('Look pack "$folderName": look.json is not an object');
          continue;
        }

        final manifest = LookParser.parse(
          Map<String, dynamic>.from(decoded),
        );
        if (manifest.id != folderName) {
          warnings.add(
            'Look pack folder "$folderName" id mismatch: '
            'manifest id "${manifest.id}"',
          );
          continue;
        }

        manifests[manifest.id] = manifest;
      } on FormatException catch (e) {
        warnings.add('Look pack "$folderName": ${e.message}');
      } catch (_) {
        warnings.add('Look pack "$folderName": failed to read look.json');
      }
    }

    return LookCatalogResult(
      Map.unmodifiable(manifests),
      List.unmodifiable(warnings),
    );
  }

  static Future<Directory> defaultLooksDirectory(
    Future<Directory> Function() supportDir,
  ) async {
    final dir = await supportDir();
    return Directory(p.join(dir.path, 'looks'));
  }
}
