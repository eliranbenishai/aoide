import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/saved_playlist.dart';

/// Persistence for the **playlist collection**, split across two files so a
/// larger collection never costs launch time.
///
/// The *index* holds only what the collection panel paints and is read at
/// startup. The *track sets* hold each entry's normalized track paths and are
/// read lazily — only when the deduplicated About figures are wanted.
abstract class PlaylistCollectionStore {
  Future<List<SavedPlaylist>> readIndex();
  Future<void> writeIndex(List<SavedPlaylist> entries);

  /// Entry path → that entry's normalized track paths. Off the startup path.
  Future<Map<String, List<String>>> readTrackSets();
  Future<void> writeTrackSets(Map<String, List<String>> trackSets);
}

/// Two JSON files in the app support dir, beside `settings.json`.
///
/// `session.json` is deliberately not extended: it means "last session", and it
/// is the one store whose reader has no error handling. Both files here follow
/// `FileSettingsStore` instead and fall back to empty on any decode failure, so
/// a hand-edited or truncated file costs the collection rather than startup.
class FilePlaylistCollectionStore implements PlaylistCollectionStore {
  FilePlaylistCollectionStore({required this.supportDir});

  static const indexFileName = 'playlists.json';
  static const trackSetsFileName = 'playlist_tracks.json';

  final Future<Directory> Function() supportDir;

  Future<File> _file(String name) async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, name));
  }

  @override
  Future<List<SavedPlaylist>> readIndex() async {
    final file = await _file(indexFileName);
    if (!await file.exists()) return const [];
    try {
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return const [];
      final raw = decoded['entries'];
      if (raw is! List) return const [];
      final entries = <SavedPlaylist>[];
      for (final item in raw) {
        if (item is! Map) continue;
        try {
          entries.add(
            SavedPlaylist.fromJson(Map<String, dynamic>.from(item)),
          );
        } on FormatException {
          // One unreadable entry must not cost the listener the others.
          continue;
        }
      }
      return entries;
    } catch (_) {
      // Deliberate catch-all: on-disk JSON is untrusted input, and an empty
      // collection is recoverable where a throw on the startup path is not.
      return const [];
    }
  }

  @override
  Future<void> writeIndex(List<SavedPlaylist> entries) async {
    final file = await _file(indexFileName);
    await file.writeAsString(
      jsonEncode({'entries': [for (final e in entries) e.toJson()]}),
    );
  }

  @override
  Future<Map<String, List<String>>> readTrackSets() async {
    final file = await _file(trackSetsFileName);
    if (!await file.exists()) return const {};
    try {
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return const {};
      final raw = decoded['trackSets'];
      if (raw is! Map) return const {};
      final sets = <String, List<String>>{};
      for (final entry in raw.entries) {
        final key = entry.key;
        final value = entry.value;
        if (key is! String || value is! List) continue;
        sets[key] = [
          for (final path in value)
            if (path is String && path.isNotEmpty) path,
        ];
      }
      return sets;
    } catch (_) {
      return const {};
    }
  }

  @override
  Future<void> writeTrackSets(Map<String, List<String>> trackSets) async {
    final file = await _file(trackSetsFileName);
    await file.writeAsString(jsonEncode({'trackSets': trackSets}));
  }
}
