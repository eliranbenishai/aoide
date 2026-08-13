import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

/// Lifetime usage counters — the listener's history, not their preferences.
///
/// Kept apart from `settings.json` for two reasons. Resetting settings must
/// leave history alone: fixing a preference is not asking to forget everything
/// you have played. And a **spin** lands every few minutes while music is on,
/// so folding it into settings would rewrite the whole preferences document
/// every time a track ended.
class UsageCounters {
  const UsageCounters({this.spins = 0});

  /// What a listener who has played nothing reads — and what a missing or
  /// unreadable usage file falls back to, so a bad file costs the count rather
  /// than the launch.
  static const empty = UsageCounters();

  /// Tracks played through to the end, over the life of the installation.
  final int spins;

  Map<String, dynamic> toJson() => {'spins': spins};

  factory UsageCounters.fromJson(Map<String, dynamic> json) {
    final spins = json['spins'];
    // A hand-edited negative would read as a machine that has un-played music.
    return UsageCounters(
      spins: spins is num && spins > 0 ? spins.toInt() : 0,
    );
  }
}

abstract class UsageStore {
  Future<UsageCounters> read();
  Future<void> write(UsageCounters counters);
}

/// One JSON file in the app support dir, beside `settings.json`.
///
/// `session.json` is deliberately not extended: it means "last session", and it
/// is the one store whose reader has no error handling. This follows
/// `FileSettingsStore` instead and falls back to [UsageCounters.empty] on any
/// decode failure.
class FileUsageStore implements UsageStore {
  FileUsageStore({required this.supportDir});

  static const fileName = 'usage.json';

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, fileName));
  }

  @override
  Future<UsageCounters> read() async {
    try {
      final file = await _file();
      if (!await file.exists()) return UsageCounters.empty;
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return UsageCounters.empty;
      return UsageCounters.fromJson(Map<String, dynamic>.from(decoded));
    } catch (_) {
      // Deliberate catch-all: on-disk JSON is untrusted input, and a zero is
      // recoverable where a throw on the startup path is not.
      return UsageCounters.empty;
    }
  }

  @override
  Future<void> write(UsageCounters counters) async {
    final file = await _file();
    await file.writeAsString(jsonEncode(counters.toJson()));
  }
}
