import 'dart:io';

import 'package:flutter/foundation.dart';

import '../domain/playlist.dart';
import '../domain/track.dart';
import 'm3u_codec.dart';
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

  Playlist get playlist => _playlist;
  int? get selectedIndex => _selectedIndex;

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
    _selectedIndex = _adjustSelectedAfterRemove(index);
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
    _selectedIndex = _adjustSelectedAfterMove(oldIndex, insertIndex);
    notifyListeners();
  }

  void select(int index) {
    if (index < 0 || index >= _playlist.tracks.length) return;
    _selectedIndex = index;
    notifyListeners();
  }

  void clear() {
    _playlist = const Playlist();
    _selectedIndex = null;
    notifyListeners();
  }

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
}
