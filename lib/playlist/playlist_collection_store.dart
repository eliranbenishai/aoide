import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/saved_playlist.dart';

/// The lazily-read companion to the collection index: what each entry holds,
/// and how long each distinct track runs for.
///
/// One value rather than two store calls because the two maps live in one file
/// and must be written together — a write that knew only the track sets would
/// erase the running times beside them.
class CollectionTrackSets {
  const CollectionTrackSets({
    this.byEntry = const {},
    this.durationsMs = const {},
  });

  static const empty = CollectionTrackSets();

  /// Entry path → that entry's normalized track paths, in file order.
  final Map<String, List<String>> byEntry;

  /// Normalized track path → running time in milliseconds.
  ///
  /// Keyed by **track** rather than by entry because a running time is a
  /// property of the file: a track kept in three playlists runs for one length,
  /// and the deduplicated total wants exactly one of them. A track whose length
  /// is not known is simply absent.
  final Map<String, int> durationsMs;

  bool get isEmpty => byEntry.isEmpty && durationsMs.isEmpty;
  bool get isNotEmpty => !isEmpty;
}

/// Persistence for the **playlist collection**, split across two files so a
/// larger collection never costs launch time.
///
/// The *index* holds only what the collection panel paints and is read at
/// startup. The *track sets* hold each entry's normalized track paths and are
/// read lazily — only when the deduplicated About figures are wanted.
abstract class PlaylistCollectionStore {
  Future<List<SavedPlaylist>> readIndex();
  Future<void> writeIndex(List<SavedPlaylist> entries);

  /// The companion file. Off the startup path.
  Future<CollectionTrackSets> readTrackSets();
  Future<void> writeTrackSets(CollectionTrackSets trackSets);
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
  Future<CollectionTrackSets> readTrackSets() async {
    final file = await _file(trackSetsFileName);
    if (!await file.exists()) return CollectionTrackSets.empty;
    try {
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return CollectionTrackSets.empty;
      final raw = decoded['trackSets'];
      final sets = <String, List<String>>{};
      if (raw is Map) {
        for (final entry in raw.entries) {
          final key = entry.key;
          final value = entry.value;
          if (key is! String || value is! List) continue;
          sets[key] = [
            for (final path in value)
              if (path is String && path.isNotEmpty) path,
          ];
        }
      }
      // Absent in files written before running times were kept. Those figures
      // come back on the entry's next refresh rather than costing the count.
      final rawDurations = decoded['trackDurations'];
      final durations = <String, int>{};
      if (rawDurations is Map) {
        for (final entry in rawDurations.entries) {
          final key = entry.key;
          final value = entry.value;
          if (key is! String || value is! num) continue;
          final ms = value.toInt();
          if (ms > 0) durations[key] = ms;
        }
      }
      return CollectionTrackSets(byEntry: sets, durationsMs: durations);
    } catch (_) {
      return CollectionTrackSets.empty;
    }
  }

  @override
  Future<void> writeTrackSets(CollectionTrackSets trackSets) async {
    final file = await _file(trackSetsFileName);
    await file.writeAsString(
      jsonEncode({
        'trackSets': trackSets.byEntry,
        'trackDurations': trackSets.durationsMs,
      }),
    );
  }
}
