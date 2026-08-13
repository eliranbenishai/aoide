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
  final Set<String> _missing = {};
  String? _selectedPath;
  String? _lastError;

  /// Saved playlists in the order the panel paints them: alphabetical by the
  /// name the listener reads.
  List<SavedPlaylist> get entries => List<SavedPlaylist>.unmodifiable(_entries);

  /// Normalized paths of the **disabled playlists**: entries whose file was not
  /// there the last time it was looked at.
  ///
  /// Derived from the most recent check and never stored, so a file that comes
  /// back — a drive remounted, a share reconnected — re-enables its entry on
  /// the next check with no listener action. Nothing in [SavedPlaylist] carries
  /// this, which is what keeps it out of the index on disk.
  Set<String> get disabledPaths => Set<String>.unmodifiable(_missing);

  bool isDisabled(String path) =>
      _missing.contains(normalizePlaylistPath(path));

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
    _missing.clear();
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
  Future<SavedPlaylist?> add(String path, {String? name}) =>
      _keepReference(path, name: name, rewritten: false);

  /// Keeps a reference to a playlist file **Tramp has just written**, as
  /// create-from-current-playlist does once the file is on disk.
  ///
  /// The same entry, figures, and companion track set [add] produces — this is
  /// that work, not a second copy of it. The one difference is what happens
  /// when the listener saved over a file the collection already holds: the
  /// entry is *updated* rather than left alone, keeping the name they gave it.
  /// [add] leaves an existing entry's figures to [validateReferences] because
  /// it has no reason to think the file moved; here Tramp moved it, so the
  /// cached count and duration are known stale the moment the write lands.
  ///
  /// Either way the collection ends up with exactly one entry for the path,
  /// because an entry's identity is its normalized path.
  Future<SavedPlaylist?> addWritten(String path) =>
      _keepReference(path, rewritten: true);

  Future<SavedPlaylist?> _keepReference(
    String path, {
    String? name,
    required bool rewritten,
  }) async {
    final normalized = normalizePlaylistPath(path);
    final existing = _entryFor(normalized);
    if (existing != null && !rewritten) {
      _lastError = null;
      _selectedPath = existing.path;
      // One stat, so a re-add answers the disabled question with what is on
      // disk now. The entry's figures are deliberately left alone — a moved
      // modification time belongs to [validateReferences].
      final reference = await _visit(normalized);
      if (reference.exists) {
        _missing.remove(normalized);
      } else {
        _missing.add(normalized);
      }
      notifyListeners();
      return existing;
    }

    final reference = await _visit(normalized);
    final figures = await _readFigures(normalized);
    if (figures == null) {
      notifyListeners();
      return null;
    }

    final entry = SavedPlaylist(
      path: normalized,
      // A rewritten entry keeps the name the listener gave it: they saved over
      // a playlist, they did not rename it back to its filename.
      name: existing?.name ?? name,
      trackCount: figures.trackCount,
      totalDuration: figures.totalDuration,
      modified: reference.modified,
    );
    final at = _entries.indexWhere((e) => e.path == normalized);
    if (at >= 0) {
      _entries[at] = entry;
    } else {
      _entries.add(entry);
    }
    _sort();
    _selectedPath = entry.path;
    _missing.remove(normalized);
    _lastError = null;

    await _store.writeIndex(_entries);
    await _writeTrackSet(entry.path, figures.trackPaths);
    notifyListeners();
    return entry;
  }

  /// Checks every reference against the filesystem, after the app has loaded.
  ///
  /// Deliberately **not** on the startup path — the host kicks this off once
  /// the session is up, so a larger collection never costs launch time. Each
  /// entry costs one [FileStat], which answers existence and modification time
  /// in the same visit; only entries whose files actually moved are re-read.
  ///
  /// A file that is gone makes its entry a **disabled playlist**: still listed,
  /// still removable, not loadable. Nothing is ever dropped — a collection whose
  /// every file is missing still lists every entry. A file whose modification
  /// time has moved gets its cached count, duration, and track set recomputed,
  /// because the listener edited it somewhere else.
  ///
  /// Re-runnable and idempotent: a second pass with nothing changed reads no
  /// playlist file, writes nothing, and notifies nobody.
  Future<void> validateReferences() async {
    final missing = <String>{};
    final refreshed = <String, SavedPlaylist>{};
    final refreshedTrackSets = <String, List<String>>{};
    final errorBefore = _lastError;

    for (final entry in List<SavedPlaylist>.of(_entries)) {
      final reference = await _visit(entry.path);
      if (!reference.exists) {
        missing.add(entry.path);
        continue;
      }
      if (_sameStamp(entry.modified, reference.modified)) continue;

      final figures = await _readFigures(entry.path);
      // A file that is there but unreadable keeps its last known figures; the
      // listener is told through [lastError] rather than shown a zero.
      if (figures == null) continue;
      refreshed[entry.path] = entry.copyWith(
        trackCount: figures.trackCount,
        totalDuration: figures.totalDuration,
        modified: reference.modified,
      );
      refreshedTrackSets[entry.path] = figures.trackPaths;
    }

    if (refreshed.isNotEmpty) {
      for (var i = 0; i < _entries.length; i++) {
        final replacement = refreshed[_entries[i].path];
        if (replacement != null) _entries[i] = replacement;
      }
      await _store.writeIndex(_entries);
      await _store.writeTrackSets(
        Map<String, List<String>>.from(await _store.readTrackSets())
          ..addAll(refreshedTrackSets),
      );
    }

    // An entry the listener removed while the pass ran is no longer theirs to
    // judge; refreshed figures are applied by path for the same reason.
    missing.removeWhere((path) => _entryFor(path) == null);

    final disabledMoved = !setEquals(missing, _missing);
    _missing
      ..clear()
      ..addAll(missing);
    if (disabledMoved || refreshed.isNotEmpty || _lastError != errorBefore) {
      notifyListeners();
    }
  }

  /// The entry for [path] when its file is still there, or null when the
  /// collection does not hold [path] or the entry is a **disabled playlist**.
  ///
  /// Checked against disk rather than against the last pass, so a file that
  /// vanished in between disables its row on the click that discovered it, and
  /// a file that came back loads without waiting for another pass. Returning
  /// null rather than throwing is the point: a load that cannot happen must
  /// leave the listener's collection exactly as it was.
  Future<SavedPlaylist?> resolveForLoad(String path) async {
    final normalized = normalizePlaylistPath(path);
    final entry = _entryFor(normalized);
    if (entry == null) return null;
    final reference = await _visit(normalized);
    if (reference.exists) {
      if (_missing.remove(normalized)) notifyListeners();
      return entry;
    }
    if (_missing.add(normalized)) notifyListeners();
    return null;
  }

  /// Drops the entry for [path] from the collection. Never touches disk: the
  /// listener's playlist file stays exactly where it was.
  Future<void> remove(String path) async {
    final normalized = normalizePlaylistPath(path);
    final before = _entries.length;
    _entries.removeWhere((entry) => entry.path == normalized);
    if (_entries.length == before) return;
    if (_selectedPath == normalized) _selectedPath = null;
    _missing.remove(normalized);

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

  /// One visit to a reference on disk.
  ///
  /// [FileStat.stat] answers existence and modification time together and never
  /// throws, so checking a collection costs one syscall per entry. Every stamp
  /// the collection holds comes from here, and is truncated to the index's own
  /// millisecond precision — a stamp that has been through the index therefore
  /// compares equal to a freshly read one, and [validateReferences] cannot read
  /// every launch as an external edit.
  Future<_Reference> _visit(String path) async {
    final stat = await FileStat.stat(path);
    if (stat.type == FileSystemEntityType.notFound) {
      return const _Reference(exists: false);
    }
    return _Reference(
      exists: true,
      modified: DateTime.fromMillisecondsSinceEpoch(
        stat.modified.millisecondsSinceEpoch,
      ),
    );
  }

  /// Reads the playlist file at [path] for the figures the panel paints and the
  /// track set the About stats deduplicate. Null when the file cannot be read,
  /// with [lastError] set — this is the only work in the module that opens one.
  Future<_Figures?> _readFigures(String path) async {
    final String contents;
    try {
      contents = await File(path).readAsString();
    } catch (error) {
      _lastError = 'Could not read playlist: ${_shortError(error)}';
      return null;
    }
    final tracks = _codec.parse(contents, playlistFilePath: path);
    var total = Duration.zero;
    for (final track in tracks) {
      final duration = track.duration;
      if (duration != null) total += duration;
    }
    return _Figures(
      trackCount: tracks.length,
      totalDuration: total,
      trackPaths: [
        for (final track in tracks) normalizePlaylistPath(track.path),
      ],
    );
  }

  /// Compared in epoch milliseconds rather than by [DateTime.==], which also
  /// weighs a stamp's UTC flag.
  bool _sameStamp(DateTime? cached, DateTime? seen) =>
      cached != null &&
      seen != null &&
      cached.millisecondsSinceEpoch == seen.millisecondsSinceEpoch;

  void _sort() => _entries.sort(SavedPlaylist.compareByDisplayName);

  String _shortError(Object error) {
    final text = error is FileSystemException
        ? (error.message.isEmpty ? error.toString() : error.message)
        : error.toString();
    return text.length > 160 ? '${text.substring(0, 157)}...' : text;
  }
}

/// What one visit to a referenced file found.
class _Reference {
  const _Reference({required this.exists, this.modified});

  final bool exists;
  final DateTime? modified;
}

/// What one read of a playlist file yielded: the figures the panel paints, and
/// the entry's normalized track paths for the companion file.
class _Figures {
  const _Figures({
    required this.trackCount,
    required this.totalDuration,
    required this.trackPaths,
  });

  final int trackCount;
  final Duration totalDuration;
  final List<String> trackPaths;
}
