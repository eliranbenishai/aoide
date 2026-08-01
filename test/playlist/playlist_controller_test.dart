import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/m3u_codec.dart';
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

  group('openPlaylistFile', () {
    test('loads tracks, sourcePath, and persists path to MemoryStore', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_open_');
      final m3uPath = p.join(dir.path, 'test.m3u');
      final trackPath = p.join(dir.path, 'song.mp3');
      const codec = M3uCodec();
      await File(m3uPath).writeAsString(codec.encode([
        Track(
          path: trackPath,
          title: 'Song',
          artist: 'Artist',
          duration: const Duration(seconds: 100),
        ),
      ]));

      final store = MemoryStore();
      final c = PlaylistController(store: store);

      await c.openPlaylistFile(m3uPath);

      expect(c.playlist.tracks, hasLength(1));
      expect(c.playlist.tracks[0].path, p.normalize(trackPath));
      expect(c.playlist.tracks[0].title, 'Song');
      expect(c.playlist.tracks[0].artist, 'Artist');
      expect(c.playlist.sourcePath, m3uPath);
      expect(store.last, m3uPath);
    });

    test('persists path to FilePlaylistStore', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_open_store_');
      final m3uPath = p.join(dir.path, 'list.m3u');
      await File(m3uPath).writeAsString(
        const M3uCodec().encode([Track(path: '/abs/track.mp3')]),
      );

      final storeDir = await Directory.systemTemp.createTemp('tramp_store_');
      final store = FilePlaylistStore(supportDir: () async => storeDir);
      final c = PlaylistController(store: store);

      await c.openPlaylistFile(m3uPath);

      expect(await store.readLastPlaylistPath(), m3uPath);
    });
  });

  group('savePlaylistFile', () {
    test('writes m3u contents and persists path to store', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_save_');
      final m3uPath = p.join(dir.path, 'out.m3u');
      final trackPath = p.normalize(p.join(dir.path, 'a.mp3'));

      final store = MemoryStore();
      final c = PlaylistController(store: store);
      c.addTracks([
        Track(
          path: trackPath,
          title: 'A',
          artist: 'X',
          duration: const Duration(seconds: 10),
        ),
      ]);

      await c.savePlaylistFile(m3uPath);

      expect(c.playlist.sourcePath, m3uPath);
      expect(store.last, m3uPath);

      final contents = await File(m3uPath).readAsString();
      expect(contents, const M3uCodec().encode(c.playlist.tracks));
    });
  });

  group('restoreLastPlaylist', () {
    test('loads tracks from stored path when file exists', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_restore_');
      final m3uPath = p.join(dir.path, 'saved.m3u');
      final trackPath = p.join(dir.path, 'track.mp3');
      const codec = M3uCodec();
      await File(m3uPath).writeAsString(codec.encode([
        Track(
          path: trackPath,
          title: 'T',
          artist: 'A',
          duration: const Duration(seconds: 60),
        ),
      ]));

      final store = MemoryStore()..last = m3uPath;
      final c = PlaylistController(store: store);

      await c.restoreLastPlaylist();

      expect(c.playlist.tracks, hasLength(1));
      expect(c.playlist.tracks[0].path, p.normalize(trackPath));
      expect(c.playlist.tracks[0].title, 'T');
      expect(c.playlist.tracks[0].artist, 'A');
      expect(c.playlist.sourcePath, m3uPath);
    });

    test('no-ops when stored path is null', () async {
      final store = MemoryStore();
      final c = PlaylistController(store: store);
      c.addTracks(const [Track(path: '/existing.mp3')]);

      await c.restoreLastPlaylist();

      expect(c.playlist.tracks, hasLength(1));
      expect(c.playlist.tracks[0].path, '/existing.mp3');
      expect(c.playlist.sourcePath, isNull);
    });

    test('no-ops when stored file is missing', () async {
      final store = MemoryStore()..last = '/nonexistent/path.m3u';
      final c = PlaylistController(store: store);
      c.addTracks(const [Track(path: '/existing.mp3')]);

      await c.restoreLastPlaylist();

      expect(c.playlist.tracks, hasLength(1));
      expect(c.playlist.tracks[0].path, '/existing.mp3');
      expect(c.playlist.sourcePath, isNull);
    });
  });
}
