import 'dart:convert';
import 'dart:io';

import 'package:archive/archive.dart';
import 'package:path/path.dart' as p;

import 'look_manifest.dart';
import 'look_parser.dart';

enum LookConflictChoice { replace, cancel }

class LookConflict {
  const LookConflict({
    required this.id,
    required this.installedName,
    required this.installedAuthor,
    required this.incomingName,
    required this.incomingAuthor,
  });

  final String id;
  final String installedName;
  final String? installedAuthor;
  final String incomingName;
  final String? incomingAuthor;
}

class LookInstaller {
  LookInstaller({required this.looksDir, required this.onConflict});

  final Directory looksDir;
  final Future<LookConflictChoice> Function(LookConflict conflict) onConflict;

  Future<bool> installDirectory(Directory source) async {
    final incoming = await _readManifest(source);
    return await _installParsed(source, incoming);
  }

  Future<bool> installZip(File zipFile) async {
    final bytes = await zipFile.readAsBytes();
    final archive = ZipDecoder().decodeBytes(bytes);
    final lookEntry = _findLookJsonEntry(archive);
    if (lookEntry == null) {
      throw FormatException('look.json not found in zip');
    }

    final incoming = _parseManifestBytes(lookEntry.content);
    final prefix = _packRootPrefix(lookEntry.name);
    final tempDir = Directory.systemTemp.createTempSync('tramp-look-install');
    try {
      await _extractZipPrefix(archive, prefix, tempDir);
      return await _installParsed(tempDir, incoming);
    } finally {
      if (tempDir.existsSync()) {
        await tempDir.delete(recursive: true);
      }
    }
  }

  Future<bool> _installParsed(Directory source, LookManifest incoming) async {
    final targetDir = Directory(p.join(looksDir.path, incoming.id));

    if (await targetDir.exists()) {
      final conflict = await _buildConflict(targetDir, incoming);
      final choice = await onConflict(conflict);
      if (choice == LookConflictChoice.cancel) {
        return false;
      }
      await targetDir.delete(recursive: true);
    }

    await _copyPack(source, targetDir);
    return true;
  }

  Future<LookManifest> _readManifest(Directory source) async {
    final lookFile = File(p.join(source.path, 'look.json'));
    if (!await lookFile.exists()) {
      throw FormatException('look.json not found in ${source.path}');
    }

    return _parseManifestBytes(await lookFile.readAsBytes());
  }

  LookManifest _parseManifestBytes(List<int> bytes) {
    final decoded = jsonDecode(utf8.decode(bytes));
    if (decoded is! Map) {
      throw FormatException('look.json is not an object');
    }

    return LookParser.parse(Map<String, dynamic>.from(decoded));
  }

  Future<LookConflict> _buildConflict(
    Directory targetDir,
    LookManifest incoming,
  ) async {
    final existingFile = File(p.join(targetDir.path, 'look.json'));
    var installedName = incoming.id;
    String? installedAuthor;

    if (await existingFile.exists()) {
      try {
        final existing = _parseManifestBytes(await existingFile.readAsBytes());
        installedName = existing.name;
        installedAuthor = existing.author;
      } catch (_) {
        // Keep fallback values when existing manifest is unreadable.
      }
    }

    return LookConflict(
      id: incoming.id,
      installedName: installedName,
      installedAuthor: installedAuthor,
      incomingName: incoming.name,
      incomingAuthor: incoming.author,
    );
  }

  Future<void> _extractZipPrefix(
    Archive archive,
    String prefix,
    Directory targetDir,
  ) async {
    await targetDir.create(recursive: true);
    final targetRoot = p.canonicalize(targetDir.path);

    for (final file in archive.files) {
      if (!file.isFile) continue;

      final normalizedName = file.name.replaceAll('\\', '/');
      if (!normalizedName.startsWith(prefix)) continue;

      final relative = normalizedName.substring(prefix.length);
      if (relative.isEmpty) continue;

      final safeRelative = _safeZipRelativePath(relative);
      final outPath = p.join(targetDir.path, safeRelative);
      final canonicalOut = p.canonicalize(outPath);
      if (!p.isWithin(targetRoot, canonicalOut)) {
        throw FormatException('zip entry escapes pack root: $relative');
      }

      await Directory(p.dirname(outPath)).create(recursive: true);
      final outFile = File(outPath);
      final sink = outFile.openWrite();
      try {
        sink.add(file.content);
      } finally {
        await sink.close();
      }
    }
  }

  /// Normalizes a zip entry path relative to the pack root and rejects escapes.
  String _safeZipRelativePath(String relative) {
    final normalized = relative.replaceAll('\\', '/');
    if (normalized.isEmpty) {
      throw const FormatException('zip entry has empty path');
    }
    if (p.isAbsolute(normalized) ||
        normalized.startsWith('/') ||
        RegExp(r'^[a-zA-Z]:').hasMatch(normalized)) {
      throw FormatException('zip entry is absolute: $relative');
    }

    final segments = <String>[];
    for (final segment in normalized.split('/')) {
      if (segment.isEmpty || segment == '.') continue;
      if (segment == '..') {
        throw FormatException('zip entry escapes pack root: $relative');
      }
      segments.add(segment);
    }
    if (segments.isEmpty) {
      throw FormatException('zip entry has empty path: $relative');
    }
    return p.joinAll(segments);
  }

  Future<void> _copyPack(Directory source, Directory targetDir) async {
    await targetDir.create(recursive: true);

    await for (final entity in source.list(recursive: false)) {
      final name = p.basename(entity.path);
      final dest = p.join(targetDir.path, name);

      if (entity is File) {
        await entity.copy(dest);
      } else if (entity is Directory) {
        await _copyDirectory(entity, Directory(dest));
      }
    }
  }

  Future<void> _copyDirectory(Directory source, Directory target) async {
    await target.create(recursive: true);
    await for (final entity in source.list(recursive: true, followLinks: false)) {
      final relative = p.relative(entity.path, from: source.path);
      final destPath = p.join(target.path, relative);

      if (entity is File) {
        await Directory(p.dirname(destPath)).create(recursive: true);
        await entity.copy(destPath);
      }
    }
  }

  ArchiveFile? _findLookJsonEntry(Archive archive) {
    ArchiveFile? rootLook;
    ArchiveFile? nestedLook;

    for (final file in archive.files) {
      if (!file.isFile) continue;

      final normalized = file.name.replaceAll('\\', '/');
      if (normalized == 'look.json') {
        rootLook = file;
      } else if (RegExp(r'^[^/]+/look\.json$').hasMatch(normalized)) {
        nestedLook ??= file;
      }
    }

    return rootLook ?? nestedLook;
  }

  String _packRootPrefix(String lookJsonPath) {
    final normalized = lookJsonPath.replaceAll('\\', '/');
    if (normalized == 'look.json') {
      return '';
    }

    final slash = normalized.lastIndexOf('/');
    return normalized.substring(0, slash + 1);
  }
}
