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

  /// Prefer `skin.json`, fall back to legacy `look.json`.
  static Future<File?> manifestFileIn(Directory packDir) async {
    final skin = File(p.join(packDir.path, 'skin.json'));
    if (await skin.exists()) return skin;
    final look = File(p.join(packDir.path, 'look.json'));
    if (await look.exists()) return look;
    return null;
  }

  Future<LookCatalogResult> scan(Directory skinsDir) async {
    final manifests = <String, LookManifest>{};
    final warnings = <String>[];

    if (!await skinsDir.exists()) {
      return LookCatalogResult(manifests, warnings);
    }

    await for (final entity in skinsDir.list(followLinks: false)) {
      if (entity is! Directory) continue;

      final folderName = p.basename(entity.path);
      final manifestFile = await manifestFileIn(entity);
      if (manifestFile == null) continue;

      try {
        final decoded = jsonDecode(await manifestFile.readAsString());
        if (decoded is! Map) {
          warnings.add('Skin "$folderName": manifest is not an object');
          continue;
        }

        final manifest = LookParser.parse(
          Map<String, dynamic>.from(decoded),
        );
        if (manifest.id != folderName) {
          warnings.add(
            'Skin folder "$folderName" id mismatch: '
            'manifest id "${manifest.id}"',
          );
          continue;
        }

        manifests[manifest.id] = manifest;
      } on FormatException catch (e) {
        warnings.add('Skin "$folderName": ${e.message}');
      } catch (_) {
        warnings.add('Skin "$folderName": failed to read manifest');
      }
    }

    return LookCatalogResult(
      Map.unmodifiable(manifests),
      List.unmodifiable(warnings),
    );
  }

  static Future<Directory> defaultSkinsDirectory(
    Future<Directory> Function() supportDir,
  ) async {
    final dir = await supportDir();
    return Directory(p.join(dir.path, 'skins'));
  }

  @Deprecated('Use defaultSkinsDirectory')
  static Future<Directory> defaultLooksDirectory(
    Future<Directory> Function() supportDir,
  ) =>
      defaultSkinsDirectory(supportDir);
}
