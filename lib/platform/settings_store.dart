import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/tramp_settings.dart';

abstract class SettingsStore {
  Future<TrampSettings> read();
  Future<void> write(TrampSettings settings);
}

/// Mirrors `FilePlaylistStore`: a single JSON file in the app support dir.
///
/// Legacy shapes (`lowerRegion`, top-level playlist size, curve under
/// `equalizer`) are migrated inside [TrampSettings.fromJson].
class FileSettingsStore implements SettingsStore {
  FileSettingsStore({required this.supportDir});

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, 'settings.json'));
  }

  @override
  Future<TrampSettings> read() async {
    final f = await _file();
    if (!await f.exists()) return TrampSettings.defaults;
    // Corrupt settings must never block startup — a bad file is just defaults.
    try {
      final decoded = jsonDecode(await f.readAsString());
      if (decoded is! Map) return TrampSettings.defaults;
      return TrampSettings.fromJson(Map<String, dynamic>.from(decoded));
    } catch (_) {
      // Deliberate catch-all: on-disk JSON is untrusted input. Any decode or
      // interpretation failure (malformed syntax, wrong types, etc.) yields
      // defaults rather than crashing startup.
      return TrampSettings.defaults;
    }
  }

  @override
  Future<void> write(TrampSettings settings) async {
    final f = await _file();
    await f.writeAsString(jsonEncode(settings.toJson()));
  }
}
