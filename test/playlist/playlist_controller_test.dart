import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';

class MemoryStore implements PlaylistStore {
  String? last;

  @override
  Future<String?> readLastPlaylistPath() async => last;

  @override
  Future<void> writeLastPlaylistPath(String? path) async => last = path;
}

void main() {
  test('add, reorder, remove, select', () {
    final c = PlaylistController(store: MemoryStore());
    c.addTracks(const [Track(path: '/a.mp3'), Track(path: '/b.mp3')]);
    expect(c.playlist.tracks, hasLength(2));
    c.move(0, 2); // after b
    expect(c.playlist.tracks.map((t) => t.path), ['/b.mp3', '/a.mp3']);
    c.select(1);
    expect(c.selectedIndex, 1);
    c.removeAt(0);
    expect(c.selectedIndex, 0);
  });
}
