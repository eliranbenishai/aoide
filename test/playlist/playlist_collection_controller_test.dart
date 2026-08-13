import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/saved_playlist.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/m3u_codec.dart';
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

/// Records which playlist files were read. The controller parses exactly once
/// per read, so this is how a test proves the validation pass left a file alone.
class CountingM3uCodec extends M3uCodec {
  final List<String> parsed = [];

  int get parses => parsed.length;

  @override
  List<Track> parse(String contents, {required String playlistFilePath}) {
    parsed.add(playlistFilePath);
    return super.parse(contents, playlistFilePath: playlistFilePath);
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

  /// Rewrites a playlist the way another program would, and moves its
  /// modification time on. Explicit, because a rewrite inside the same
  /// millisecond leaves the stamp where it was and Tramp would rightly see no
  /// edit at all.
  Future<void> editPlaylistElsewhere(
    String path, {
    List<String> lines = const [],
  }) async {
    final file = File(path);
    final before = await file.lastModified();
    await file.writeAsString(['#EXTM3U', ...lines].join('\n'));
    await file.setLastModified(before.add(const Duration(seconds: 2)));
  }

  /// A support directory of its own, so a file-backed store can be pointed at it
  /// without seeing the listener's playlists.
  Future<Directory> supportDir() =>
      Directory(p.join(dir.path, 'support')).create(recursive: true);

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

  group('addWritten', () {
    test('keeps a reference to a playlist Tramp has just written', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('kept pile.m3u');

      final entry = await controller.addWritten(path);

      expect(entry, isNotNull);
      expect(controller.entries, hasLength(1));
      expect(entry!.displayName, 'kept pile');
      expect(entry.trackCount, 2);
      expect(entry.totalDuration, const Duration(seconds: 190));
      expect(entry.modified, isNotNull);
      expect(controller.selectedPath, entry.path);
      expect(
        (await controller.readTrackSets())[entry.path],
        hasLength(2),
        reason: 'the About figures need the new entry\'s track set too',
      );
    });

    test('saving over a kept playlist updates that entry, never twins it',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writePlaylist(
        'driving.m3u',
        lines: ['#EXTINF:60,One', p.join(dir.path, 'one.mp3')],
      );
      await controller.add(path);
      expect(controller.entries.single.trackCount, 1);

      // The listener saves a longer current playlist straight over it.
      await editPlaylistElsewhere(
        path,
        lines: [
          '#EXTINF:60,One',
          p.join(dir.path, 'one.mp3'),
          '#EXTINF:30,Two',
          p.join(dir.path, 'two.mp3'),
          '#EXTINF:30,Three',
          p.join(dir.path, 'three.mp3'),
        ],
      );
      await controller.addWritten(path);

      expect(controller.entries, hasLength(1), reason: 'one file, one row');
      final entry = controller.entries.single;
      expect(entry.trackCount, 3);
      expect(entry.totalDuration, const Duration(seconds: 120));
      expect(controller.selectedPath, entry.path);
      expect((await controller.readTrackSets())[entry.path], hasLength(3));
    });

    test('the figures it writes are current, not the next validation pass\'s',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writePlaylist(
        'work.m3u',
        lines: ['#EXTINF:60,One', p.join(dir.path, 'one.mp3')],
      );
      await controller.add(path);
      await editPlaylistElsewhere(
        path,
        lines: [
          '#EXTINF:60,One',
          p.join(dir.path, 'one.mp3'),
          '#EXTINF:60,Two',
          p.join(dir.path, 'two.mp3'),
        ],
      );

      await controller.addWritten(path);

      // The stamp went with the figures, so the pass that runs later reads no
      // external edit and leaves the entry exactly as this left it.
      final after = controller.entries.single;
      await controller.validateReferences();
      expect(controller.entries.single, after);
      expect(store.index.single.trackCount, 2);
    });

    test('a saved-over entry keeps the name the listener gave it', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('dt-2019-03.m3u');
      await controller.add(path, name: 'Driving Tunes');

      await editPlaylistElsewhere(path);
      await controller.addWritten(path);

      expect(controller.entries.single.displayName, 'Driving Tunes');
      expect(controller.entries.single.trackCount, 0);
    });

    test('saving over a playlist whose file had gone missing re-enables it',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('back again.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();
      expect(controller.isDisabled(path), isTrue);

      await writeTwoTrackPlaylist('back again.m3u');
      await controller.addWritten(path);

      expect(controller.isDisabled(path), isFalse);
      expect(controller.entries, hasLength(1));
    });

    test('a file that cannot be read is reported, not kept', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);

      final entry = await controller.addWritten(p.join(dir.path, 'gone.m3u'));

      expect(entry, isNull);
      expect(controller.entries, isEmpty);
      expect(controller.lastError, isNotNull);
      expect(store.indexWrites, 0);
    });
  });

  group('createFromSelection', () {
    /// Five tracks a listener could pull a shorter playlist out of.
    List<Track> fiveTracks() => [
          for (final title in ['Alpha', 'Bravo', 'Charlie', 'Delta', 'Echo'])
            Track(
              path: p.join(dir.path, '${title.toLowerCase()}.mp3'),
              title: title,
              duration: const Duration(seconds: 30),
            ),
        ];

    /// The track titles the file at [path] ended up holding, in file order.
    Future<List<String?>> writtenTitles(String path) async {
      final contents = await File(path).readAsString();
      return const M3uCodec()
          .parse(contents, playlistFilePath: path)
          .map((t) => t.title)
          .toList();
    }

    test('writes the selected tracks in the order the listener sees them',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = p.join(dir.path, 'a few.m3u');

      // Deliberately gapped and deliberately out of order: the modifier click
      // builds selections that are neither a range nor sorted.
      final entry = await controller.createFromSelection(
        path,
        fiveTracks(),
        {3, 0, 2},
      );

      expect(await writtenTitles(path), ['Alpha', 'Charlie', 'Delta']);
      expect(entry, isNotNull);
      expect(entry!.trackCount, 3);
      expect(entry.displayName, 'a few');
      expect(controller.entries, hasLength(1));
      expect(controller.selectedPath, entry.path);
      expect(
        (await controller.readTrackSets())[entry.path],
        hasLength(3),
        reason: 'the About figures need the new entry\'s track set too',
      );
    });

    test('an empty selection writes nothing and keeps nothing', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = p.join(dir.path, 'nothing.m3u');

      final entry = await controller.createFromSelection(
        path,
        fiveTracks(),
        const {},
      );

      expect(entry, isNull);
      expect(File(path).existsSync(), isFalse);
      expect(controller.entries, isEmpty);
      expect(store.indexWrites, 0);
    });

    test('rows the selection outran are ignored, not written as gaps',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = p.join(dir.path, 'stale.m3u');

      await controller.createFromSelection(path, fiveTracks(), {1, 9, -1});

      expect(await writtenTitles(path), ['Bravo']);
    });

    test('writing over a kept playlist updates that entry, never twins it',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path, name: 'Driving Tunes');
      expect(controller.entries.single.trackCount, 2);

      await controller.createFromSelection(path, fiveTracks(), {0, 1, 2, 3});

      expect(controller.entries, hasLength(1), reason: 'one file, one row');
      final entry = controller.entries.single;
      expect(entry.trackCount, 4, reason: 'the figures moved with the file');
      expect(entry.displayName, 'Driving Tunes', reason: 'not a rename');
    });

    test('a path that cannot be written is reported, not kept', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);

      final entry = await controller.createFromSelection(
        p.join(dir.path, 'no such folder', 'out.m3u'),
        fiveTracks(),
        {0},
      );

      expect(entry, isNull);
      expect(controller.entries, isEmpty);
      expect(controller.lastError, isNotNull);
      expect(store.indexWrites, 0);
    });
  });

  group('rename', () {
    test('the row reads the new name and the file keeps its own', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('dt-2019-03.m3u');
      final contentsBefore = await File(path).readAsString();
      await controller.add(path);
      expect(controller.entries.single.displayName, 'dt-2019-03');

      await controller.rename(path, 'Driving Tunes');

      expect(controller.entries.single.displayName, 'Driving Tunes');
      // The listener's file is exactly where it was, called what they called
      // it, with what they put in it — this is the whole promise of ADR 0008.
      expect(File(path).existsSync(), isTrue);
      expect(await File(path).readAsString(), contentsBefore);
      expect(
        dir.listSync().whereType<File>().map((f) => p.basename(f.path)),
        ['dt-2019-03.m3u'],
        reason: 'no second file, and no renamed one either',
      );
      expect(controller.entries.single.path, normalizePlaylistPath(path));
    });

    test('the collection re-sorts under the name the listener chose', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      await controller.add(await writeTwoTrackPlaylist('anthems.m3u'));
      await controller.add(await writeTwoTrackPlaylist('work.m3u'));
      expect(
        controller.entries.map((e) => e.displayName),
        ['anthems', 'work'],
      );

      await controller.rename(p.join(dir.path, 'anthems.m3u'), 'Zed Songs');

      expect(
        controller.entries.map((e) => e.displayName),
        ['work', 'Zed Songs'],
      );
    });

    test('it survives a restart', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('dt-2019-03.m3u');
      await controller.add(path);
      await controller.rename(path, 'Driving Tunes');

      final reopened = PlaylistCollectionController(store: store);
      await reopened.bootstrap();

      expect(reopened.entries.single.displayName, 'Driving Tunes');
    });

    test('clearing it falls back to the filename rather than a blank row',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('dt-2019-03.m3u');
      await controller.add(path, name: 'Driving Tunes');

      await controller.rename(path, '');

      expect(controller.entries.single.displayName, 'dt-2019-03');
      expect(controller.entries.single.name, isNull);

      // Whitespace is not a name either, and neither is null.
      await controller.rename(path, 'Driving Tunes');
      await controller.rename(path, '   ');
      expect(controller.entries.single.displayName, 'dt-2019-03');
      await controller.rename(path, 'Driving Tunes');
      await controller.rename(path, null);
      expect(controller.entries.single.displayName, 'dt-2019-03');
    });

    test('two entries may read the same name without either being lost',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final first = await writeTwoTrackPlaylist('dt-2019-03.m3u');
      final second = await writeTwoTrackPlaylist('dt-2021-08.m3u');
      await controller.add(first);
      await controller.add(second);

      await controller.rename(first, 'Driving Tunes');
      await controller.rename(second, 'Driving Tunes');

      // Identity is the path, so a shared display name merges nothing.
      expect(controller.entries, hasLength(2));
      expect(
        controller.entries.map((e) => e.displayName),
        ['Driving Tunes', 'Driving Tunes'],
      );
      expect(
        controller.entries.map((e) => e.path),
        [normalizePlaylistPath(first), normalizePlaylistPath(second)],
        reason: 'the tie is broken by path, so the order is still stable',
      );
      await controller.remove(first);
      expect(controller.entries.single.path, normalizePlaylistPath(second));
    });

    test('a disabled entry can be renamed without reading its file', () async {
      final store = MemoryCollectionStore();
      final codec = CountingM3uCodec();
      final controller =
          PlaylistCollectionController(store: store, codec: codec);
      final path = await writeTwoTrackPlaylist('gone.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();
      expect(controller.isDisabled(path), isTrue);
      final parsesBefore = codec.parses;

      await controller.rename(path, 'Still Mine');

      expect(controller.entries.single.displayName, 'Still Mine');
      expect(controller.isDisabled(path), isTrue, reason: 'still missing');
      expect(
        codec.parses,
        parsesBefore,
        reason: 'renaming a row never opens the playlist file',
      );
    });

    test('renaming an entry the collection does not hold changes nothing',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      await controller.add(await writeTwoTrackPlaylist('driving.m3u'));
      final writesBefore = store.indexWrites;

      await controller.rename(p.join(dir.path, 'never-kept.m3u'), 'Nope');
      // And a rename to the name it already has is not a change either.
      await controller.rename(p.join(dir.path, 'driving.m3u'), null);

      expect(store.indexWrites, writesBefore);
      expect(controller.entries.single.displayName, 'driving');
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

  group('validation', () {
    test('a missing file makes its entry a disabled playlist', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);

      await File(path).delete();
      await controller.validateReferences();

      expect(controller.isDisabled(path), isTrue);
      expect(controller.disabledPaths, {normalizePlaylistPath(path)});
      expect(
        controller.entries,
        hasLength(1),
        reason: 'the entry survives; the file is what is gone',
      );
      expect(
        controller.entries.single.trackCount,
        2,
        reason: 'figures are kept — a disabled playlist still counts',
      );
    });

    test('a file that comes back re-enables its entry', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();
      expect(controller.isDisabled(path), isTrue);

      // The drive is remounted. Nothing else happens: no add, no select, no
      // remove — only the next pass.
      await writeTwoTrackPlaylist('driving.m3u');
      await controller.validateReferences();

      expect(controller.isDisabled(path), isFalse);
      expect(controller.disabledPaths, isEmpty);
    });

    test('a collection where every file is missing still lists every entry',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final paths = [
        await writeTwoTrackPlaylist('driving.m3u'),
        await writeTwoTrackPlaylist('sunday.m3u'),
        await writeTwoTrackPlaylist('work.m3u'),
      ];
      for (final path in paths) {
        await controller.add(path);
      }

      for (final path in paths) {
        await File(path).delete();
      }
      await controller.validateReferences();

      expect(
        controller.entries.map((e) => e.displayName),
        ['driving', 'sunday', 'work'],
      );
      expect(controller.disabledPaths, hasLength(3));
      expect(
        store.index,
        hasLength(3),
        reason: 'nor is anything dropped on disk',
      );
    });

    test('an edit elsewhere refreshes count, duration, and track set',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('work.m3u');
      await controller.add(path);
      expect(controller.entries.single.trackCount, 2);

      await editPlaylistElsewhere(
        path,
        lines: ['#EXTINF:30,Artist C - Charlie', p.join(dir.path, 'c.mp3')],
      );
      await controller.validateReferences();

      final entry = controller.entries.single;
      expect(entry.trackCount, 1);
      expect(entry.totalDuration, const Duration(seconds: 30));
      expect(
        await controller.readTrackSets(),
        {
          normalizePlaylistPath(path): [
            normalizePlaylistPath(p.join(dir.path, 'c.mp3')),
          ],
        },
      );
      expect(
        store.index.single.trackCount,
        1,
        reason: 'the refreshed figures are what a restart reads back',
      );
    });

    test('only entries whose files actually changed are re-read', () async {
      final codec = CountingM3uCodec();
      final store = MemoryCollectionStore();
      final controller =
          PlaylistCollectionController(store: store, codec: codec);
      final untouched = await writeTwoTrackPlaylist('untouched.m3u');
      final edited = await writeTwoTrackPlaylist('edited.m3u');
      await controller.add(untouched);
      await controller.add(edited);
      final readsOnAdd = codec.parses;

      await editPlaylistElsewhere(edited, lines: [p.join(dir.path, 'c.mp3')]);
      await controller.validateReferences();

      expect(
        codec.parsed.skip(readsOnAdd),
        [normalizePlaylistPath(edited)],
        reason: 'the untouched playlist is never opened',
      );
    });

    test('a pass over an unchanged collection recomputes nothing', () async {
      final codec = CountingM3uCodec();
      final store = MemoryCollectionStore();
      final controller =
          PlaylistCollectionController(store: store, codec: codec);
      await controller.add(await writeTwoTrackPlaylist('driving.m3u'));
      await controller.add(await writeTwoTrackPlaylist('work.m3u'));
      final reads = codec.parses;
      final indexWrites = store.indexWrites;
      final trackSetWrites = store.trackSetWrites;
      var notifications = 0;
      controller.addListener(() => notifications++);

      await controller.validateReferences();
      await controller.validateReferences();

      expect(
        codec.parses,
        reads,
        reason: 'a stamp read back must compare equal to the one stored',
      );
      expect(store.indexWrites, indexWrites);
      expect(store.trackSetWrites, trackSetWrites);
      expect(
        notifications,
        0,
        reason: 'nothing changed, so the host has nothing to broadcast',
      );
    });

    test('a pass on the launch after a restart reads no playlist file',
        () async {
      final support = await supportDir();
      final store = FilePlaylistCollectionStore(
        supportDir: () async => support,
      );
      final kept = PlaylistCollectionController(store: store);
      await kept.add(await writeTwoTrackPlaylist('driving.m3u'));

      // Next launch: the stamps have been through playlists.json, which is the
      // only place millisecond precision could have been lost.
      final codec = CountingM3uCodec();
      final relaunched =
          PlaylistCollectionController(store: store, codec: codec);
      await relaunched.bootstrap();
      await relaunched.validateReferences();

      expect(
        codec.parses,
        0,
        reason: 'a launch must not read every playlist as an external edit',
      );
      expect(
        relaunched.entries.single.modified,
        kept.entries.single.modified,
        reason: 'the stamp the index can hold is the only stamp Tramp keeps',
      );
      expect(relaunched.disabledPaths, isEmpty);
      expect(relaunched.entries.single.trackCount, 2);
    });

    test('a file that is there but unreadable keeps its figures', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);

      // Something took the name and is not a playlist any more.
      await File(path).delete();
      await Directory(path).create();
      await controller.validateReferences();

      expect(
        controller.isDisabled(path),
        isFalse,
        reason: 'the path is not gone',
      );
      expect(controller.entries.single.trackCount, 2);
      expect(controller.lastError, isNotNull);
    });

    test('disabled state never reaches the index on disk', () async {
      final support = await supportDir();
      final store = FilePlaylistCollectionStore(
        supportDir: () async => support,
      );
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();
      expect(controller.isDisabled(path), isTrue);

      final index = File(
        p.join(support.path, FilePlaylistCollectionStore.indexFileName),
      );
      final raw = await index.readAsString();
      expect(raw, isNot(contains('disabled')));
      expect(raw, isNot(contains('missing')));

      final reopened = PlaylistCollectionController(store: store);
      await reopened.bootstrap();
      expect(reopened.entries, hasLength(1));
      expect(
        reopened.disabledPaths,
        isEmpty,
        reason: 'nothing is disabled until a pass has looked',
      );

      await reopened.validateReferences();
      expect(reopened.isDisabled(path), isTrue);
    });
  });

  group('loading a reference', () {
    test('a disabled entry does nothing and does not throw', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();

      await expectLater(controller.resolveForLoad(path), completion(isNull));

      expect(controller.entries, hasLength(1));
      expect(controller.isDisabled(path), isTrue);
      expect(
        store.indexWrites,
        1,
        reason: 'a load that cannot happen writes nothing',
      );
    });

    test('a file that vanished since the last pass disables its entry',
        () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);
      await controller.validateReferences();
      expect(controller.isDisabled(path), isFalse);

      await File(path).delete();

      expect(await controller.resolveForLoad(path), isNull);
      expect(controller.isDisabled(path), isTrue);
    });

    test('a live entry resolves to itself', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);

      final entry = await controller.resolveForLoad(path);

      expect(entry, isNotNull);
      expect(entry!.path, normalizePlaylistPath(path));
      expect(await controller.resolveForLoad(p.join(dir.path, 'never.m3u')),
          isNull);
    });

    test('a re-add of a file that came back re-enables its entry', () async {
      final store = MemoryCollectionStore();
      final controller = PlaylistCollectionController(store: store);
      final path = await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);
      await File(path).delete();
      await controller.validateReferences();
      expect(controller.isDisabled(path), isTrue);

      await writeTwoTrackPlaylist('driving.m3u');
      await controller.add(path);

      expect(controller.entries, hasLength(1));
      expect(controller.isDisabled(path), isFalse);
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
