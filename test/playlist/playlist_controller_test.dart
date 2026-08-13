import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/m3u_codec.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_sort.dart';
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

  group('updateTrackByPath', () {
    test('patches duration used by TOTAL without reshuffling selection', () {
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(const [
        Track(path: '/a.mp3'),
        Track(path: '/b.mp3'),
      ]);
      c.select(1);

      expect(
        c.updateTrackByPath(
          '/a.mp3',
          (t) => t.copyWith(duration: const Duration(seconds: 65)),
        ),
        isTrue,
      );
      expect(c.playlist.tracks[0].duration, const Duration(seconds: 65));
      expect(c.selectedIndex, 1);
      expect(c.selectedIndices, {1});

      expect(
        c.updateTrackByPath('/missing.mp3', (t) => t),
        isFalse,
      );
    });
  });

  group('selection and sort', () {
    test('selectAll and invertSelection', () {
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(const [
        Track(path: '/a.mp3'),
        Track(path: '/b.mp3'),
        Track(path: '/c.mp3'),
      ]);
      c.select(1);
      expect(c.selectedIndices, {1});

      c.selectAll();
      expect(c.selectedIndices, {0, 1, 2});
      expect(c.selectedIndex, 0);

      c.invertSelection();
      expect(c.selectedIndices, isEmpty);
      expect(c.selectedIndex, isNull);

      c.select(0);
      c.invertSelection();
      expect(c.selectedIndices, {1, 2});
    });

    test('removeSelected removes all highlighted rows', () {
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(const [
        Track(path: '/a.mp3'),
        Track(path: '/b.mp3'),
        Track(path: '/c.mp3'),
      ]);
      c.selectAll();
      c.invertSelection(); // empty
      c.select(0);
      c.selectAll();
      c.removeSelected();
      expect(c.playlist.tracks, isEmpty);
    });

    test('sortBy title / artist / duration / path and reverse', () {
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(const [
        Track(
          path: '/z.mp3',
          title: 'Zoo',
          artist: 'B',
          duration: Duration(seconds: 30),
        ),
        Track(
          path: '/a.mp3',
          title: 'Alpha',
          artist: 'C',
          duration: Duration(seconds: 10),
        ),
        Track(
          path: '/m.mp3',
          title: 'Mid',
          artist: 'A',
          duration: Duration(seconds: 20),
        ),
      ]);
      c.select(0); // Zoo

      c.sortBy(PlaylistSortKey.title);
      expect(c.playlist.tracks.map((t) => t.title), ['Alpha', 'Mid', 'Zoo']);
      expect(c.selectedIndex, 2);

      c.sortBy(PlaylistSortKey.artist);
      expect(c.playlist.tracks.map((t) => t.artist), ['A', 'B', 'C']);

      c.sortBy(PlaylistSortKey.duration);
      expect(
        c.playlist.tracks.map((t) => t.duration!.inSeconds),
        [10, 20, 30],
      );

      c.sortBy(PlaylistSortKey.path);
      expect(c.playlist.tracks.map((t) => t.path), ['/a.mp3', '/m.mp3', '/z.mp3']);

      c.reverseTracks();
      expect(c.playlist.tracks.map((t) => t.path), ['/z.mp3', '/m.mp3', '/a.mp3']);
    });
  });

  group('altered current playlist', () {
    const tracks = [
      Track(path: '/a.mp3', title: 'Alpha', duration: Duration(seconds: 10)),
      Track(path: '/b.mp3', title: 'Bravo', duration: Duration(seconds: 20)),
      Track(path: '/c.mp3', title: 'Charlie', duration: Duration(seconds: 30)),
    ];

    /// A playlist as a load leaves it: three tracks, an origin, unaltered.
    PlaylistController loaded({String origin = '/music/list.m3u'}) {
      final c = PlaylistController(store: MemoryStore());
      c.setTracks(tracks, sourcePath: origin);
      expect(c.altered, isFalse, reason: 'a load sets the baseline');
      return c;
    }

    Future<String> writePlaylistFile(
      String prefix, {
      List<Track> contents = tracks,
    }) async {
      final dir = await Directory.systemTemp.createTemp(prefix);
      final path = p.join(dir.path, 'list.m3u');
      await File(path).writeAsString(const M3uCodec().encode(contents));
      return path;
    }

    test('adding tracks raises it', () {
      final c = loaded();
      c.addTracks(const [Track(path: '/d.mp3')]);
      expect(c.altered, isTrue);
    });

    test('removing one track raises it', () {
      final c = loaded();
      c.removeAt(1);
      expect(c.altered, isTrue);
    });

    test('removing the selection raises it', () {
      final c = loaded();
      c.select(0);
      expect(c.altered, isFalse);
      c.removeSelected();
      expect(c.altered, isTrue);
    });

    test('reordering raises it', () {
      final c = loaded();
      c.move(0, 3);
      expect(c.playlist.tracks.map((t) => t.path), [
        '/b.mp3',
        '/c.mp3',
        '/a.mp3',
      ]);
      expect(c.altered, isTrue);
    });

    test('sorting raises it', () {
      final c = loaded();
      c.sortBy(PlaylistSortKey.duration);
      expect(c.altered, isFalse, reason: 'already in duration order');

      c.reverseTracks();
      expect(c.altered, isTrue);

      final other = loaded();
      other.sortBy(PlaylistSortKey.title);
      expect(other.altered, isFalse, reason: 'already in title order');
      other.move(0, 3);
      other.sortBy(PlaylistSortKey.title);
      expect(other.altered, isTrue);
    });

    test('reversing raises it', () {
      final c = loaded();
      c.reverseTracks();
      expect(c.altered, isTrue);
    });

    test('clearing tracks raises it', () {
      final c = loaded();
      c.clear();
      expect(c.altered, isTrue);
    });

    test('a clear leaves no origin, so there is nowhere to save straight to',
        () {
      final c = loaded();
      c.clear();
      // Clear starts a new, empty current playlist rather than emptying the
      // file the listener loaded: the confirmation's save has to ask where.
      expect(c.playlist.sourcePath, isNull);
      expect(c.playlist.tracks, isEmpty);
      expect(c.altered, isTrue);
    });

    test('nothing changed means nothing raised', () {
      final c = loaded();

      c.addTracks(const []);
      c.removeAt(9);
      c.removeSelected(); // nothing selected
      c.move(1, 2); // dropped back where it was picked up
      c.sortBy(PlaylistSortKey.path); // already in path order
      expect(c.altered, isFalse);
      expect(c.playlist.tracks.map((t) => t.path), [
        '/a.mp3',
        '/b.mp3',
        '/c.mp3',
      ]);

      final empty = PlaylistController(store: MemoryStore());
      empty.clear();
      empty.reverseTracks();
      expect(empty.altered, isFalse);
    });

    test('selecting never raises it', () {
      final c = loaded();

      c.select(1);
      c.selectAll();
      c.invertSelection();
      c.setSelectedIndices(const [0, 2], primary: 2);

      expect(c.selectedIndices, {0, 2});
      expect(c.altered, isFalse);
    });

    test('patching a track from a metadata probe never raises it', () {
      final c = loaded();

      expect(
        c.updateTrackByPath(
          '/b.mp3',
          (t) => t.copyWith(duration: const Duration(seconds: 90)),
        ),
        isTrue,
      );

      // Durations are filled in after every load; raising here would mark a
      // freshly loaded playlist as changed.
      expect(c.playlist.tracks[1].duration, const Duration(seconds: 90));
      expect(c.altered, isFalse);
    });

    test('opening a playlist file leaves it unaltered, however many times',
        () async {
      final first = await writePlaylistFile('tramp_altered_open_');
      final second = await writePlaylistFile('tramp_altered_open2_');
      final c = PlaylistController(store: MemoryStore());

      await c.openPlaylistFile(first);
      expect(c.altered, isFalse);
      await c.openPlaylistFile(second);
      expect(c.altered, isFalse);
      await c.openPlaylistFile(first);
      expect(c.altered, isFalse);
    });

    test('loading a playlist over an altered one sets a fresh baseline',
        () async {
      final path = await writePlaylistFile('tramp_altered_reload_');
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(const [Track(path: '/ad-hoc.mp3')]);
      expect(c.altered, isTrue);

      await c.openPlaylistFile(path);

      expect(c.altered, isFalse);
      expect(c.playlist.sourcePath, path);
    });

    test('restoring the last playlist leaves it unaltered', () async {
      final path = await writePlaylistFile('tramp_altered_restore_');
      final c = PlaylistController(store: MemoryStore()..last = path);

      await c.restoreLastPlaylist();

      expect(c.playlist.tracks, hasLength(3));
      expect(c.altered, isFalse);
    });

    test('writing the whole list to a file lowers it', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_altered_save_');
      final path = p.join(dir.path, 'out.m3u');
      final c = PlaylistController(store: MemoryStore());
      c.addTracks(tracks);
      expect(c.altered, isTrue);

      await c.savePlaylistFile(path);

      expect(c.altered, isFalse);
      expect(c.playlist.sourcePath, path);

      // And it goes straight back up on the next change.
      c.removeAt(0);
      expect(c.altered, isTrue);
      await c.savePlaylistFile(path);
      expect(c.altered, isFalse);
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
