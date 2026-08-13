import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/saved_playlist.dart';
import 'package:tramp/playlist/playlist_collection_controller.dart';
import 'package:tramp/playlist/playlist_collection_store.dart';

class MemoryCollectionStore implements PlaylistCollectionStore {
  List<SavedPlaylist> index = const [];
  Map<String, List<String>> trackSets = const {};
  int indexReads = 0;
  int indexWrites = 0;
  int trackSetReads = 0;
  int trackSetWrites = 0;

  @override
  Future<List<SavedPlaylist>> readIndex() async {
    indexReads++;
    return List<SavedPlaylist>.of(index);
  }

  @override
  Future<void> writeIndex(List<SavedPlaylist> entries) async {
    indexWrites++;
    index = List<SavedPlaylist>.of(entries);
  }

  @override
  Future<Map<String, List<String>>> readTrackSets() async {
    trackSetReads++;
    return Map<String, List<String>>.of(trackSets);
  }

  @override
  Future<void> writeTrackSets(Map<String, List<String>> sets) async {
    trackSetWrites++;
    trackSets = Map<String, List<String>>.of(sets);
  }
}

/// A store whose index cannot be read — stands in for a file the listener (or a
/// crash) left malformed on disk.
class UnreadableCollectionStore extends MemoryCollectionStore {
  @override
  Future<List<SavedPlaylist>> readIndex() async {
    indexReads++;
    throw const FormatException('unexpected end of input');
  }
}

void main() {
  late Directory dir;

  setUp(() async {
    dir = await Directory.systemTemp.createTemp('tramp_collection_');
  });

  tearDown(() async {
    if (await dir.exists()) await dir.delete(recursive: true);
  });

  /// Writes a playlist file the listener owns and returns its path.
  Future<String> writePlaylist(
    String name, {
    List<String> lines = const [],
  }) async {
    final file = File(p.join(dir.path, name));
    await file.writeAsString(['#EXTM3U', ...lines].join('\n'));
    return file.path;
  }

  Future<String> writeTwoTrackPlaylist(String name) => writePlaylist(
        name,
        lines: [
          '#EXTINF:65,Artist A - Alpha',
          p.join(dir.path, 'a.mp3'),
          '#EXTINF:125,Artist B - Bravo',
          p.join(dir.path, 'b.mp3'),
        ],
      );

  group('add', () {
    test('keeps a reference with the figures the panel paints', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('road trip.m3u');

      final entry = await controller.add(path);

      expect(entry, isNotNull);
      expect(controller.entries, hasLength(1));
      expect(entry!.path, normalizePlaylistPath(path));
      expect(entry.displayName, 'road trip');
      expect(entry.trackCount, 2);
      expect(entry.totalDuration, const Duration(seconds: 190));
      expect(entry.modified, isNotNull);
      expect(controller.selectedPath, entry.path);
      expect(store.indexWrites, 1);
    });

    test('never copies, moves, or rewrites the listener\'s file', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('keep me.m3u');
      final before = await File(path).readAsString();
      final modifiedBefore = await File(path).lastModified();
      final neighboursBefore =
          dir.listSync().map((e) => e.path).toList()..sort();

      await controller.add(path);

      expect(await File(path).exists(), isTrue);
      expect(await File(path).readAsString(), before);
      expect(await File(path).lastModified(), modifiedBefore);
      expect(
        dir.listSync().map((e) => e.path).toList()..sort(),
        neighboursBefore,
        reason: 'adding must not write anything beside the playlist file',
      );
    });

    test('a path already kept selects its entry instead of twinning it',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final first = await writeTwoTrackPlaylist('first.m3u');
      final second = await writeTwoTrackPlaylist('second.m3u');
      await controller.add(first);
      await controller.add(second);
      controller.select(second);
      final writesBefore = store.indexWrites;

      final again = await controller.add(first);

      expect(controller.entries, hasLength(2));
      expect(again!.path, normalizePlaylistPath(first));
      expect(controller.selectedPath, normalizePlaylistPath(first));
      expect(store.indexWrites, writesBefore, reason: 'nothing changed');
    });

    test('another spelling of the same file is the same entry', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);

      // Same file, reached through a redundant `.` and a `..` hop.
      final roundabout = p.join(dir.path, 'nested', '..', '.', 'driving.m3u');
      final again = await controller.add(roundabout);

      expect(controller.entries, hasLength(1));
      expect(again!.path, controller.entries.single.path);
    });

    test('an override names the entry without touching the file', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('dt-2019-03.m3u');

      final entry = await controller.add(path, name: 'Driving Tunes');

      expect(entry!.displayName, 'Driving Tunes');
      expect(p.basename(entry.path), 'dt-2019-03.m3u');
      expect(await File(path).exists(), isTrue);
    });

    test('a playlist file that cannot be read is reported, not kept', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);

      final entry = await controller.add(p.join(dir.path, 'gone.m3u'));

      expect(entry, isNull);
      expect(controller.entries, isEmpty);
      expect(controller.lastError, isNotNull);
      expect(store.indexWrites, 0);
    });

    test('caches the entry\'s normalized track paths for later figures',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writePlaylist(
        'mixed.m3u',
        lines: [
          '#EXTINF:10,Relative',
          'sub/./one.mp3',
          '#EXTINF:20,Absolute',
          p.join(dir.path, 'two.mp3'),
        ],
      );

      await controller.add(path);

      expect(store.trackSetWrites, 1);
      expect(
        await controller.readTrackSets(),
        {
          normalizePlaylistPath(path): [
            normalizePlaylistPath(p.join(dir.path, 'sub', 'one.mp3')),
            normalizePlaylistPath(p.join(dir.path, 'two.mp3')),
          ],
        },
      );
    });
  });

  group('remove', () {
    test('drops the entry and never touches disk', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('tidy me.m3u');
      final before = await File(path).readAsString();
      final modifiedBefore = await File(path).lastModified();
      await controller.add(path);

      await controller.remove(path);

      expect(controller.entries, isEmpty);
      expect(controller.selectedPath, isNull);
      expect(store.index, isEmpty);
      expect(await File(path).exists(), isTrue);
      expect(await File(path).readAsString(), before);
      expect(await File(path).lastModified(), modifiedBefore);
    });

    test('drops the entry\'s cached track paths with it', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final kept = await writeTwoTrackPlaylist('kept.m3u');
      final dropped = await writeTwoTrackPlaylist('dropped.m3u');
      await controller.add(kept);
      await controller.add(dropped);

      await controller.remove(dropped);

      expect(
        (await controller.readTrackSets()).keys,
        [normalizePlaylistPath(kept)],
      );
    });

    test('an unknown path changes nothing', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      await controller.add(await writeTwoTrackPlaylist('one.m3u'));
      final writesBefore = store.indexWrites;

      await controller.remove(p.join(dir.path, 'never-added.m3u'));

      expect(controller.entries, hasLength(1));
      expect(store.indexWrites, writesBefore);
    });
  });

  group('bootstrap', () {
    test('reads the index and orders entries by display name', () async {
      final store = MemoryCollectionStore()
        ..index = [
          SavedPlaylist(path: p.join(dir.path, 'zulu.m3u')),
          SavedPlaylist(path: p.join(dir.path, 'alpha.m3u')),
          SavedPlaylist(path: p.join(dir.path, 'x.m3u'), name: 'Mike'),
        ];
      final controller = PlaylistCollectionController(store: store);

      await controller.bootstrap();

      expect(
        controller.entries.map((e) => e.displayName),
        ['alpha', 'Mike', 'zulu'],
      );
    });

    test('leaves the companion track-set file unread', () async {
      final store = MemoryCollectionStore()
        ..index = [SavedPlaylist(path: p.join(dir.path, 'a.m3u'))];
      final controller = PlaylistCollectionController(store: store);

      await controller.bootstrap();

      expect(
        store.trackSetReads,
        0,
        reason: 'track sets are for About figures, not for startup',
      );
    });

    test('an unreadable index costs the collection, not the launch', () async {
      final store = UnreadableCollectionStore();
      final controller = PlaylistCollectionController(store: store);

      await controller.bootstrap();

      expect(controller.entries, isEmpty);
      expect(controller.lastError, isNotNull);
    });
  });

  group('select', () {
    test('highlights a kept entry and clears on null', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('pick me.m3u');
      await controller.add(path);

      controller.select(null);
      expect(controller.selectedPath, isNull);

      controller.select(path);
      expect(controller.selectedPath, normalizePlaylistPath(path));
    });

    test('a path outside the collection clears the highlight', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      await controller.add(await writeTwoTrackPlaylist('only.m3u'));
      expect(controller.selectedPath, isNotNull);

      // What the host does when the listener opens a playlist file straight
      // from disk: no row is the loaded one any more.
      controller.select(p.join(dir.path, 'stranger.m3u'));

      expect(controller.selectedPath, isNull);
    });
  });

  test('entries stay alphabetical as the listener adds playlists', () async {
    final store = MemoryCollectionStore();
    final controller = PlaylistCollectionController(store: store);
    await controller.add(await writeTwoTrackPlaylist('work.m3u'));
    await controller.add(await writeTwoTrackPlaylist('driving.m3u'));
    await controller.add(await writeTwoTrackPlaylist('sunday.m3u'));

    expect(
      controller.entries.map((e) => e.displayName),
      ['driving', 'sunday', 'work'],
    );
  });

  test('notifies once per change so the host broadcasts once', () async {
    final store = MemoryCollectionStore();
    final controller = PlaylistCollectionController(store: store);
    var notifications = 0;
    controller.addListener(() => notifications++);

    await controller.add(await writeTwoTrackPlaylist('a.m3u'));
    expect(notifications, 1);

    await controller.remove(p.join(dir.path, 'a.m3u'));
    expect(notifications, 2);
  });
}
