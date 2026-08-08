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

  Playlist get playlist => _playlist;

  /// Primary / anchor selection (last single-select or play target).
  int? get selectedIndex => _selectedIndex;

  /// Multi-selection for bulk ops (select-all / invert / remove).
  Set<int> get selectedIndices => Set<int>.unmodifiable(_selectedIndices);

  Future<void> openPlaylistFile(String path) async {
    final contents = await File(path).readAsString();
    final tracks = _codec.parse(contents, playlistFilePath: path);
    setTracks(tracks, sourcePath: path);
    await _store.writeLastPlaylistPath(path);
  }

  Future<void> savePlaylistFile(String path) async {
    await File(path).writeAsString(_codec.encode(_playlist.tracks));
    _playlist = _playlist.copyWith(sourcePath: path);
    await _store.writeLastPlaylistPath(path);
    notifyListeners();
  }

  Future<void> restoreLastPlaylist() async {
    final path = await _store.readLastPlaylistPath();
    if (path == null || !await File(path).exists()) return;
    await openPlaylistFile(path);
  }

  void setTracks(List<Track> tracks, {String? sourcePath}) {
    _playlist = Playlist(sourcePath: sourcePath, tracks: List.of(tracks));
    _selectedIndex = tracks.isEmpty ? null : _clampIndex(_selectedIndex);
    _syncSelectedIndicesFromPrimary();
    notifyListeners();
  }

  void addTracks(List<Track> tracks) {
    if (tracks.isEmpty) return;
    _playlist = _playlist.copyWith(
      tracks: [..._playlist.tracks, ...tracks],
    );
    notifyListeners();
  }

  void removeAt(int index) {
    final tracks = List<Track>.of(_playlist.tracks);
    if (index < 0 || index >= tracks.length) return;
    tracks.removeAt(index);
    _playlist = _playlist.copyWith(tracks: tracks);
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
    _playlist = _playlist.copyWith(tracks: tracks);
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
    _restoreSelectionByPath(tracks, selectedPaths, anchorPath);
    notifyListeners();
  }

  void reverseTracks() {
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
    _restoreSelectionByPath(tracks, selectedPaths, anchorPath);
    notifyListeners();
  }

  void clear() {
    _playlist = const Playlist();
    _selectedIndex = null;
    _selectedIndices.clear();
    notifyListeners();
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
