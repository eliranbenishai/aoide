import 'dart:io';

import 'package:flutter/foundation.dart';

import '../domain/playlist.dart';
import '../domain/track.dart';
import 'm3u_codec.dart';
import 'playlist_sort.dart';
import 'playlist_store.dart';

class PlaylistController extends ChangeNotifier {
  PlaylistController({
    required PlaylistStore store,
    M3uCodec codec = const M3uCodec(),
  })  : _store = store,
        _codec = codec;

  final PlaylistStore _store;
  final M3uCodec _codec;

  Playlist _playlist = const Playlist();
  int? _selectedIndex;
  final Set<int> _selectedIndices = <int>{};
  bool _altered = false;

  Playlist get playlist => _playlist;

  /// Primary / anchor selection (last single-select or play target).
  int? get selectedIndex => _selectedIndex;

  /// Multi-selection for bulk ops (select-all / invert / remove).
  Set<int> get selectedIndices => Set<int>.unmodifiable(_selectedIndices);

  /// Whether this is an **altered current playlist**: its track list has
  /// changed since it was loaded or last written whole to a file.
  ///
  /// Raised **only** by mutation of the track list — add, remove, reorder,
  /// sort, reverse, clear — and only where the mutation really changed it, so
  /// the prompt it drives still means something when it appears.
  ///
  /// Lowered **only** by [savePlaylistFile], which writes the whole list to the
  /// file that becomes the playlist's origin, and by [setTracks], which
  /// replaces the playlist wholesale and so sets a fresh baseline — loading a
  /// saved playlist leaves it down however many times the listener loads.
  /// Selection, and the metadata patching in [updateTrackByPath], leave it
  /// exactly where it is.
  ///
  /// There is deliberately no setter: "only a whole write lowers it" is a rule
  /// no caller can talk its way around.
  bool get altered => _altered;

  Future<void> openPlaylistFile(String path) async {
    final contents = await File(path).readAsString();
    final tracks = _codec.parse(contents, playlistFilePath: path);
    setTracks(tracks, sourcePath: path);
    await _store.writeLastPlaylistPath(path);
  }

  /// Writes the **entire** current track list to [path], which becomes the
  /// playlist's origin — the one thing that lowers [altered].
  Future<void> savePlaylistFile(String path) async {
    await File(path).writeAsString(_codec.encode(_playlist.tracks));
    _playlist = _playlist.copyWith(sourcePath: path);
    _altered = false;
    await _store.writeLastPlaylistPath(path);
    notifyListeners();
  }

  Future<void> restoreLastPlaylist() async {
    final path = await _store.readLastPlaylistPath();
    if (path == null || !await File(path).exists()) return;
    await openPlaylistFile(path);
  }

  /// Replaces the whole playlist and sets the baseline [altered] measures from.
  ///
  /// This is what a load is made of, and what a client applies a host snapshot
  /// with, so it must never raise: otherwise every snapshot broadcast would
  /// raise the flag in the window's own mirror of the playlist.
  void setTracks(List<Track> tracks, {String? sourcePath}) {
    _playlist = Playlist(sourcePath: sourcePath, tracks: List.of(tracks));
    _selectedIndex = tracks.isEmpty ? null : _clampIndex(_selectedIndex);
    _syncSelectedIndicesFromPrimary();
    _altered = false;
    notifyListeners();
  }

  void addTracks(List<Track> tracks) {
    if (tracks.isEmpty) return;
    _playlist = _playlist.copyWith(
      tracks: [..._playlist.tracks, ...tracks],
    );
    _altered = true;
    notifyListeners();
  }

  void removeAt(int index) {
    final tracks = List<Track>.of(_playlist.tracks);
    if (index < 0 || index >= tracks.length) return;
    tracks.removeAt(index);
    _playlist = _playlist.copyWith(tracks: tracks);
    _altered = true;
    final prior = Set<int>.from(_selectedIndices);
    _selectedIndex = _adjustSelectedAfterRemove(index);
    _selectedIndices
      ..clear()
      ..addAll(_reindexAfterRemove(prior, index, tracks.length));
    if (_selectedIndex != null) {
      _selectedIndices.add(_selectedIndex!);
    }
    notifyListeners();
  }

  /// Removes every index in [selectedIndices] (or [selectedIndex] if empty).
  void removeSelected() {
    final remove = Set<int>.from(_selectedIndices);
    if (remove.isEmpty && _selectedIndex != null) {
      remove.add(_selectedIndex!);
    }
    if (remove.isEmpty) return;

    final tracks = <Track>[];
    for (var i = 0; i < _playlist.tracks.length; i++) {
      if (!remove.contains(i)) {
        tracks.add(_playlist.tracks[i]);
      }
    }
    final removed = _playlist.tracks.length - tracks.length;
    _playlist = _playlist.copyWith(tracks: tracks);
    if (removed > 0) _altered = true;
    _selectedIndex = null;
    _selectedIndices.clear();
    notifyListeners();
  }

  void move(int oldIndex, int newIndex) {
    final tracks = List<Track>.of(_playlist.tracks);
    if (oldIndex < 0 ||
        oldIndex >= tracks.length ||
        newIndex < 0 ||
        newIndex > tracks.length) {
      return;
    }
    var insertIndex = newIndex;
    if (oldIndex < newIndex) {
      insertIndex -= 1;
    }
    final item = tracks.removeAt(oldIndex);
    tracks.insert(insertIndex, item);
    _playlist = _playlist.copyWith(tracks: tracks);
    // Dropping a row back where it was picked up is not a reorder.
    if (insertIndex != oldIndex) _altered = true;
    final prior = Set<int>.from(_selectedIndices);
    _selectedIndex = _adjustSelectedAfterMove(oldIndex, insertIndex);
    _selectedIndices
      ..clear()
      ..addAll(_reindexAfterMove(prior, oldIndex, insertIndex));
    if (_selectedIndex != null) {
      _selectedIndices.add(_selectedIndex!);
    }
    notifyListeners();
  }

  void select(int index) {
    if (index < 0 || index >= _playlist.tracks.length) return;
    _selectedIndex = index;
    _selectedIndices
      ..clear()
      ..add(index);
    notifyListeners();
  }

  /// Replace multi-selection (used when applying a session snapshot).
  ///
  /// Selection is not the track list, so this leaves [altered] alone — a
  /// snapshot arriving must not raise the flag in the window's mirror.
  void setSelectedIndices(Iterable<int> indices, {int? primary}) {
    final len = _playlist.tracks.length;
    _selectedIndices
      ..clear()
      ..addAll(indices.where((i) => i >= 0 && i < len));
    if (primary != null && primary >= 0 && primary < len) {
      _selectedIndex = primary;
      _selectedIndices.add(primary);
    } else if (_selectedIndices.isEmpty) {
      _selectedIndex = null;
    } else {
      _selectedIndex = (_selectedIndices.toList()..sort()).first;
    }
    notifyListeners();
  }

  void selectAll() {
    final len = _playlist.tracks.length;
    _selectedIndices
      ..clear()
      ..addAll([for (var i = 0; i < len; i++) i]);
    _selectedIndex = len == 0 ? null : 0;
    notifyListeners();
  }

  void invertSelection() {
    final len = _playlist.tracks.length;
    final next = <int>{
      for (var i = 0; i < len; i++)
        if (!_selectedIndices.contains(i)) i,
    };
    _selectedIndices
      ..clear()
      ..addAll(next);
    _selectedIndex = next.isEmpty ? null : (next.toList()..sort()).first;
    notifyListeners();
  }

  void sortBy(PlaylistSortKey key) {
    final previous = _playlist.tracks;
    final tracks = List<Track>.of(_playlist.tracks);
    if (tracks.length < 2) return;
    final selectedPaths = {
      for (final i in _selectedIndices)
        if (i >= 0 && i < tracks.length) tracks[i].path,
    };
    final anchorPath =
        _selectedIndex != null && _selectedIndex! < tracks.length
            ? tracks[_selectedIndex!].path
            : null;

    tracks.sort((a, b) => _compareTracks(a, b, key));
    _playlist = _playlist.copyWith(tracks: tracks);
    _raiseIfReordered(previous, tracks);
    _restoreSelectionByPath(tracks, selectedPaths, anchorPath);
    notifyListeners();
  }

  void reverseTracks() {
    final previous = _playlist.tracks;
    final tracks = List<Track>.of(_playlist.tracks.reversed);
    if (tracks.length < 2) return;
    final selectedPaths = {
      for (final i in _selectedIndices)
        if (i >= 0 && i < _playlist.tracks.length) _playlist.tracks[i].path,
    };
    final anchorPath =
        _selectedIndex != null && _selectedIndex! < _playlist.tracks.length
            ? _playlist.tracks[_selectedIndex!].path
            : null;
    _playlist = _playlist.copyWith(tracks: tracks);
    _raiseIfReordered(previous, tracks);
    _restoreSelectionByPath(tracks, selectedPaths, anchorPath);
    notifyListeners();
  }

  /// Starts a new, empty current playlist — which means it keeps no origin.
  ///
  /// Clearing is a mutation, so it raises [altered], but only where there was
  /// something to clear: clearing an empty playlist changes nothing, and a
  /// prompt about work that never existed would make every prompt cheaper.
  ///
  /// Dropping [Playlist.sourcePath] is deliberate and unchanged: with no origin,
  /// the confirmation's save asks the listener where to write rather than
  /// silently truncating the file they loaded from.
  void clear() {
    if (_playlist.tracks.isNotEmpty) _altered = true;
    _playlist = const Playlist();
    _selectedIndex = null;
    _selectedIndices.clear();
    notifyListeners();
  }

  /// Patch a track in place (e.g. after a background duration/tag probe).
  ///
  /// Returns `true` when a track with [path] was found and changed.
  ///
  /// Deliberately does **not** raise [altered]: this fills in what a file
  /// already said, and it runs on every load, so raising here would mark a
  /// freshly loaded playlist as changed.
  bool updateTrackByPath(String path, Track Function(Track current) update) {
    final tracks = List<Track>.of(_playlist.tracks);
    final index = tracks.indexWhere((t) => t.path == path);
    if (index < 0) return false;
    final next = update(tracks[index]);
    if (next == tracks[index]) return false;
    tracks[index] = next;
    _playlist = _playlist.copyWith(tracks: tracks);
    notifyListeners();
    return true;
  }

  /// Sort and reverse land on a new list every time; only a different running
  /// order is a mutation the listener would want protecting.
  void _raiseIfReordered(List<Track> previous, List<Track> next) {
    if (!listEquals(previous, next)) _altered = true;
  }

  void _restoreSelectionByPath(
    List<Track> tracks,
    Set<String> selectedPaths,
    String? anchorPath,
  ) {
    _selectedIndices
      ..clear()
      ..addAll([
        for (var i = 0; i < tracks.length; i++)
          if (selectedPaths.contains(tracks[i].path)) i,
      ]);
    if (anchorPath == null) {
      _selectedIndex = _selectedIndices.isEmpty
          ? null
          : (_selectedIndices.toList()..sort()).first;
      return;
    }
    final anchor = tracks.indexWhere((t) => t.path == anchorPath);
    _selectedIndex = anchor >= 0 ? anchor : null;
    if (_selectedIndex != null) {
      _selectedIndices.add(_selectedIndex!);
    }
  }

  void _syncSelectedIndicesFromPrimary() {
    _selectedIndices.clear();
    if (_selectedIndex != null) {
      _selectedIndices.add(_selectedIndex!);
    }
  }

  int _compareTracks(Track a, Track b, PlaylistSortKey key) {
    switch (key) {
      case PlaylistSortKey.title:
        return _cmpString(a.displayTitle, b.displayTitle);
      case PlaylistSortKey.artist:
        return _cmpString(a.artist ?? '', b.artist ?? '');
      case PlaylistSortKey.duration:
        final ad = a.duration?.inMilliseconds ?? -1;
        final bd = b.duration?.inMilliseconds ?? -1;
        return ad.compareTo(bd);
      case PlaylistSortKey.path:
        return _cmpString(a.path, b.path);
    }
  }

  int _cmpString(String a, String b) =>
      a.toLowerCase().compareTo(b.toLowerCase());

  int? _clampIndex(int? index) {
    if (index == null) return null;
    final len = _playlist.tracks.length;
    if (len == 0) return null;
    if (index < 0) return 0;
    if (index >= len) return len - 1;
    return index;
  }

  int? _adjustSelectedAfterRemove(int removedIndex) {
    final selected = _selectedIndex;
    if (selected == null) return null;
    if (_playlist.tracks.isEmpty) return null;
    if (selected == removedIndex) {
      return selected.clamp(0, _playlist.tracks.length - 1);
    }
    if (selected > removedIndex) return selected - 1;
    return selected;
  }

  int? _adjustSelectedAfterMove(int oldIndex, int newIndex) {
    final selected = _selectedIndex;
    if (selected == null) return null;
    if (selected == oldIndex) return newIndex;
    if (oldIndex < selected && newIndex >= selected) return selected - 1;
    if (oldIndex > selected && newIndex <= selected) return selected + 1;
    return selected;
  }

  Set<int> _reindexAfterRemove(Set<int> prior, int removed, int newLen) {
    final next = <int>{};
    for (final i in prior) {
      if (i == removed) continue;
      final mapped = i > removed ? i - 1 : i;
      if (mapped >= 0 && mapped < newLen) next.add(mapped);
    }
    return next;
  }

  Set<int> _reindexAfterMove(Set<int> prior, int oldIndex, int newIndex) {
    final next = <int>{};
    for (final i in prior) {
      if (i == oldIndex) {
        next.add(newIndex);
      } else if (oldIndex < i && newIndex >= i) {
        next.add(i - 1);
      } else if (oldIndex > i && newIndex <= i) {
        next.add(i + 1);
      } else {
        next.add(i);
      }
    }
    return next;
  }
}
