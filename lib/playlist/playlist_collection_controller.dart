import 'dart:io';

import 'package:flutter/foundation.dart';

import '../domain/collection_figures.dart';
import '../domain/saved_playlist.dart';
import '../domain/track.dart';
import 'm3u_codec.dart';
import 'playlist_collection_store.dart';

/// The listener's **playlist collection**: the saved playlists Tramp keeps as
/// references to the files where the listener put them.
///
/// A plain [ChangeNotifier] over an injected store, sibling to
/// `PlaylistController`, so every collection decision is exercisable without
/// the session host widget — which is bound to the multi-window and
/// window-manager plugins and cannot be pumped by a test.
///
/// Nothing here copies, moves, or renames a file the listener owns. A
/// referenced file is only ever *read*, to count its tracks, and [rename]
/// touches the index alone. The single write is [createFromSelection], which
/// makes a **new** playlist at a path the listener chose in a save dialog —
/// authoring a file, not rewriting the location of one that already existed.
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
  int _figuresRevision = 0;

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

  /// Bumped whenever [readFigures] could answer differently: a playlist added,
  /// saved, removed, or found changed on disk.
  ///
  /// Exists so a caller can tell those four moments apart from the far more
  /// frequent ones that cannot move a figure — a row being highlighted, say.
  /// Reading the figures costs the companion file, and nothing should pay that
  /// for a selection change.
  int get figuresRevision => _figuresRevision;

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
    _figuresRevision++;
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

  /// Pulls a shorter playlist out of a longer one: writes the tracks the
  /// listener has selected to [path] and keeps a reference to the result.
  ///
  /// [selectedIndices] index into [tracks] and arrive as an unordered, possibly
  /// gapped set — the platform modifier click builds selections that are not
  /// ranges. They are **sorted** here so the file reads in the running order
  /// the listener is looking at, rather than in whatever order the set happens
  /// to iterate in. Indices outside [tracks] are ignored, the way every other
  /// selection method in this codebase ignores them.
  ///
  /// An empty selection is refused with no file written and no entry kept, the
  /// same answer an empty current playlist gets from create-from-current: the
  /// listener asked to keep something and there is nothing there.
  ///
  /// Deliberately *not* routed through `PlaylistController.savePlaylistFile`.
  /// That method exists to write the **whole** current track list to the file
  /// that becomes its origin, which is the one thing that lowers the altered
  /// state; only some of these tracks are being kept, so the current playlist —
  /// its tracks, its origin, and its altered state — must come out of this
  /// untouched. Nothing here can reach it.
  Future<SavedPlaylist?> createFromSelection(
    String path,
    List<Track> tracks,
    Set<int> selectedIndices,
  ) async {
    final picked = [
      for (final index in selectedIndices.toList()..sort())
        if (index >= 0 && index < tracks.length) tracks[index],
    ];
    if (picked.isEmpty) return null;
    try {
      await File(path).writeAsString(_codec.encode(picked));
    } catch (error) {
      _lastError = 'Could not write playlist: ${_shortError(error)}';
      notifyListeners();
      return null;
    }
    return addWritten(path);
  }

  /// Renames the entry for [path] to [name], or back to its filename when
  /// [name] is null or blank.
  ///
  /// **The file on disk is never touched** — see
  /// `docs/adr/0008-playlist-collection-stores-references.md`. A saved playlist
  /// is a reference to a document the listener also manages in their file
  /// manager and their other players; reaching into their folders because they
  /// retitled a row would break the promise the reference model makes. Only
  /// Tramp's own index moves, which is why this reads no file and so works
  /// perfectly well on a **disabled playlist**.
  ///
  /// Two entries may end up reading the same name. Neither is lost or merged:
  /// an entry's identity is its normalized path, and the display name is only
  /// what the panel paints.
  Future<void> rename(String path, String? name) async {
    final normalized = normalizePlaylistPath(path);
    final at = _entries.indexWhere((entry) => entry.path == normalized);
    if (at < 0) return;
    final trimmed = name?.trim();
    final cleared = trimmed == null || trimmed.isEmpty;
    if ((cleared ? null : trimmed) == _entries[at].name) return;
    _entries[at] = _entries[at].copyWith(
      name: cleared ? null : trimmed,
      clearName: cleared,
    );
    // Alphabetical by what the listener reads, so a renamed row moves to where
    // its new name belongs rather than staying under its old one.
    _sort();
    await _store.writeIndex(_entries);
    notifyListeners();
  }

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
    _figuresRevision++;

    await _store.writeIndex(_entries);
    await _writeTrackSet(entry.path, figures);
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
    final refreshedTrackSets = <String, _Figures>{};
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
      refreshedTrackSets[entry.path] = figures;
    }

    if (refreshed.isNotEmpty) {
      for (var i = 0; i < _entries.length; i++) {
        final replacement = refreshed[_entries[i].path];
        if (replacement != null) _entries[i] = replacement;
      }
      _figuresRevision++;
      await _store.writeIndex(_entries);
      final stored = await _store.readTrackSets();
      final byEntry = Map<String, List<String>>.from(stored.byEntry);
      final durations = Map<String, int>.from(stored.durationsMs);
      for (final refresh in refreshedTrackSets.entries) {
        byEntry[refresh.key] = refresh.value.trackPaths;
        durations.addAll(refresh.value.trackDurationsMs);
      }
      await _store.writeTrackSets(_prune(byEntry, durations));
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
    _figuresRevision++;

    await _store.writeIndex(_entries);
    final sets = await _store.readTrackSets();
    if (sets.byEntry.containsKey(normalized)) {
      await _store.writeTrackSets(
        _prune(
          Map<String, List<String>>.from(sets.byEntry)..remove(normalized),
          sets.durationsMs,
        ),
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
  Future<CollectionTrackSets> readTrackSets() => _store.readTrackSets();

  /// The deduplicated figures the About window's stats well reports.
  ///
  /// The union is built **here, in memory, at the moment it is wanted** — never
  /// accumulated as entries come and go. A running total cannot be subtracted
  /// correctly when an entry is removed or rewritten: the tracks it held may
  /// also be held elsewhere, and arithmetic on a total cannot tell.
  ///
  /// Every entry counts, **disabled playlists included**, because a drive that
  /// is unplugged today is still music the listener keeps. The current playlist
  /// is not in the collection and so contributes nothing until it is saved.
  ///
  /// A track whose running time is unknown — an `#EXTM3U` line with no
  /// `#EXTINF` before it — adds nothing to the total rather than a guess. It
  /// still counts as a track: Tramp knows the listener keeps it, it just does
  /// not know how long it runs.
  ///
  /// Off the startup path by construction: this is the only thing that reads
  /// the companion file, and nothing calls it until the figures are wanted.
  Future<CollectionFigures> readFigures() async {
    final sets = await _store.readTrackSets();
    final union = <String>{};
    for (final entry in _entries) {
      final paths = sets.byEntry[entry.path];
      if (paths != null) union.addAll(paths);
    }
    var total = Duration.zero;
    for (final path in union) {
      final ms = sets.durationsMs[path];
      if (ms != null) total += Duration(milliseconds: ms);
    }
    return CollectionFigures(
      playlists: _entries.length,
      tracks: union.length,
      totalDuration: total,
    );
  }

  SavedPlaylist? _entryFor(String normalizedPath) {
    for (final entry in _entries) {
      if (entry.path == normalizedPath) return entry;
    }
    return null;
  }

  Future<void> _writeTrackSet(String path, _Figures figures) async {
    final stored = await _store.readTrackSets();
    final byEntry = Map<String, List<String>>.from(stored.byEntry)
      ..[path] = figures.trackPaths;
    final durations = Map<String, int>.from(stored.durationsMs)
      ..addAll(figures.trackDurationsMs);
    await _store.writeTrackSets(_prune(byEntry, durations));
  }

  /// Drops running times for tracks no entry holds any more, so the companion
  /// file cannot grow forever as the listener tidies their collection.
  CollectionTrackSets _prune(
    Map<String, List<String>> byEntry,
    Map<String, int> durations,
  ) {
    final held = <String>{for (final paths in byEntry.values) ...paths};
    return CollectionTrackSets(
      byEntry: byEntry,
      durationsMs: {
        for (final path in held)
          if (durations[path] != null) path: durations[path]!,
      },
    );
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
    final trackPaths = <String>[];
    final trackDurationsMs = <String, int>{};
    for (final track in tracks) {
      final normalized = normalizePlaylistPath(track.path);
      trackPaths.add(normalized);
      final duration = track.duration;
      if (duration == null || duration <= Duration.zero) continue;
      total += duration;
      trackDurationsMs[normalized] = duration.inMilliseconds;
    }
    return _Figures(
      trackCount: tracks.length,
      totalDuration: total,
      trackPaths: trackPaths,
      trackDurationsMs: trackDurationsMs,
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
/// the entry's normalized track paths and running times for the companion file.
class _Figures {
  const _Figures({
    required this.trackCount,
    required this.totalDuration,
    required this.trackPaths,
    required this.trackDurationsMs,
  });

  final int trackCount;

  /// This entry's own running time, duplicates and all — what its row paints.
  /// The About total deduplicates instead; see
  /// [PlaylistCollectionController.readFigures].
  final Duration totalDuration;

  final List<String> trackPaths;

  /// Only the tracks whose length this file declared.
  final Map<String, int> trackDurationsMs;
}
