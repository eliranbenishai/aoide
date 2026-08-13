import 'dart:io';

import 'package:flutter/foundation.dart';

import '../domain/saved_playlist.dart';
import 'm3u_codec.dart';
import 'playlist_collection_store.dart';

/// The listener's **playlist collection**: the saved playlists Tramp keeps as
/// references to the files where the listener put them.
///
/// A plain [ChangeNotifier] over an injected store, sibling to
/// `PlaylistController`, so every collection decision is exercisable without the
/// session host widget — which is bound to the multi-window and window-manager
/// plugins and cannot be pumped by a test.
///
/// Nothing here writes, moves, or rewrites a playlist file. Files are only ever
/// read, to count their tracks.
class PlaylistCollectionController extends ChangeNotifier {
  PlaylistCollectionController({
    required PlaylistCollectionStore store,
    M3uCodec codec = const M3uCodec(),
  })  : _store = store,
        _codec = codec;

  final PlaylistCollectionStore _store;
  final M3uCodec _codec;

  final List<SavedPlaylist> _entries = [];
  String? _selectedPath;
  String? _lastError;

  /// Saved playlists in the order the panel paints them: alphabetical by the
  /// name the listener reads.
  List<SavedPlaylist> get entries => List<SavedPlaylist>.unmodifiable(_entries);

  /// Normalized path of the entry that is loaded / highlighted, if any.
  String? get selectedPath => _selectedPath;

  /// Last failure a listener could act on (an unreadable playlist file).
  String? get lastError => _lastError;

  /// Reads the index. Small by design — this is on the startup path.
  ///
  /// An index that cannot be read costs the listener their collection, never
  /// their launch: the file store already falls back to empty, and this catches
  /// anything it cannot.
  Future<void> bootstrap() async {
    List<SavedPlaylist> stored;
    try {
      stored = await _store.readIndex();
    } catch (error) {
      stored = const [];
      _lastError = 'Could not read playlist collection: ${_shortError(error)}';
    }
    _entries
      ..clear()
      ..addAll(stored);
    _sort();
    if (!_entries.any((e) => e.path == _selectedPath)) _selectedPath = null;
    notifyListeners();
  }

  /// Adds a reference to the playlist file at [path].
  ///
  /// A path already in the collection selects the entry it is already in rather
  /// than making a twin, because the entry's identity *is* its normalized path.
  /// Returns the entry the listener ends up on, or null when the file could not
  /// be read.
  Future<SavedPlaylist?> add(String path, {String? name}) async {
    final normalized = normalizePlaylistPath(path);
    final existing = _entryFor(normalized);
    if (existing != null) {
      _lastError = null;
      _selectedPath = existing.path;
      notifyListeners();
      return existing;
    }

    final file = File(normalized);
    final String contents;
    try {
      contents = await file.readAsString();
    } catch (error) {
      _lastError = 'Could not read playlist: ${_shortError(error)}';
      notifyListeners();
      return null;
    }

    final tracks = _codec.parse(contents, playlistFilePath: normalized);
    var total = Duration.zero;
    for (final track in tracks) {
      final duration = track.duration;
      if (duration != null) total += duration;
    }

    final entry = SavedPlaylist(
      path: normalized,
      name: name,
      trackCount: tracks.length,
      totalDuration: total,
      modified: await _modifiedOf(file),
    );
    _entries.add(entry);
    _sort();
    _selectedPath = entry.path;
    _lastError = null;

    await _store.writeIndex(_entries);
    await _writeTrackSet(
      entry.path,
      [for (final track in tracks) normalizePlaylistPath(track.path)],
    );
    notifyListeners();
    return entry;
  }

  /// Drops the entry for [path] from the collection. Never touches disk: the
  /// listener's playlist file stays exactly where it was.
  Future<void> remove(String path) async {
    final normalized = normalizePlaylistPath(path);
    final before = _entries.length;
    _entries.removeWhere((entry) => entry.path == normalized);
    if (_entries.length == before) return;
    if (_selectedPath == normalized) _selectedPath = null;

    await _store.writeIndex(_entries);
    final sets = await _store.readTrackSets();
    if (sets.containsKey(normalized)) {
      await _store.writeTrackSets(
        Map<String, List<String>>.from(sets)..remove(normalized),
      );
    }
    notifyListeners();
  }

  /// Highlights the entry for [path]. Loading its tracks is the caller's job.
  ///
  /// A path the collection does not hold — null, or an arbitrary playlist file
  /// the listener opened straight from disk — clears the highlight, so a row
  /// only ever reads as loaded while it really is.
  void select(String? path) {
    var normalized = path == null ? null : normalizePlaylistPath(path);
    if (normalized != null && _entryFor(normalized) == null) normalized = null;
    if (_selectedPath == normalized) return;
    _selectedPath = normalized;
    notifyListeners();
  }

  SavedPlaylist? entryFor(String path) =>
      _entryFor(normalizePlaylistPath(path));

  /// Each entry's cached normalized track paths, read on demand. Deduplicated
  /// About figures are the union of these; nothing on the startup path wants it.
  Future<Map<String, List<String>>> readTrackSets() => _store.readTrackSets();

  SavedPlaylist? _entryFor(String normalizedPath) {
    for (final entry in _entries) {
      if (entry.path == normalizedPath) return entry;
    }
    return null;
  }

  Future<void> _writeTrackSet(String path, List<String> trackPaths) async {
    final sets = Map<String, List<String>>.from(await _store.readTrackSets());
    sets[path] = trackPaths;
    await _store.writeTrackSets(sets);
  }

  Future<DateTime?> _modifiedOf(File file) async {
    try {
      final stamp = await file.lastModified();
      // Truncated to the index's own millisecond precision, so a time that has
      // been through the index compares equal to a freshly read one and the
      // validation pass cannot read every launch as an external edit.
      return DateTime.fromMillisecondsSinceEpoch(stamp.millisecondsSinceEpoch);
    } catch (_) {
      return null;
    }
  }

  void _sort() => _entries.sort(SavedPlaylist.compareByDisplayName);

  String _shortError(Object error) {
    final text = error is FileSystemException
        ? (error.message.isEmpty ? error.toString() : error.message)
        : error.toString();
    return text.length > 160 ? '${text.substring(0, 157)}...' : text;
  }
}
