import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/saved_playlist.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/playlist/playlist_collection_controller.dart';
import 'package:tramp/playlist/playlist_collection_store.dart';

void main() {
  late Directory supportDir;
  late Directory musicDir;
  late FilePlaylistCollectionStore store;

  setUp(() async {
    supportDir = await Directory.systemTemp.createTemp('tramp_support_');
    musicDir = await Directory.systemTemp.createTemp('tramp_music_');
    store = FilePlaylistCollectionStore(supportDir: () async => supportDir);
  });

  tearDown(() async {
    for (final dir in [supportDir, musicDir]) {
      if (await dir.exists()) await dir.delete(recursive: true);
    }
  });

  Future<String> writePlaylist(String name) async {
    final file = File(p.join(musicDir.path, name));
    await file.writeAsString([
      '#EXTM3U',
      '#EXTINF:65,Artist A - Alpha',
      p.join(musicDir.path, 'a.mp3'),
    ].join('\n'));
    return file.path;
  }

  File indexFile() => File(
        p.join(supportDir.path, FilePlaylistCollectionStore.indexFileName),
      );

  File trackSetsFile() => File(
        p.join(supportDir.path, FilePlaylistCollectionStore.trackSetsFileName),
      );

  test('reads empty before anything has been kept', () async {
    expect(await store.readIndex(), isEmpty);
    expect(await store.readTrackSets(), isEmpty);
  });

  test('round-trips the index through a real directory', () async {
    final entry = SavedPlaylist(
      path: p.join(musicDir.path, 'driving.m3u'),
      name: 'Driving Tunes',
      trackCount: 12,
      totalDuration: const Duration(minutes: 47, seconds: 5),
      modified: DateTime.fromMillisecondsSinceEpoch(1723000000000),
    );

    await store.writeIndex([entry]);

    expect(await store.readIndex(), [entry]);
  });

  test('round-trips the companion track sets', () async {
    final sets = {
      normalizePlaylistPath(p.join(musicDir.path, 'driving.m3u')): [
        normalizePlaylistPath(p.join(musicDir.path, 'a.mp3')),
        normalizePlaylistPath(p.join(musicDir.path, 'b.mp3')),
      ],
    };

    await store.writeTrackSets(sets);

    expect(await store.readTrackSets(), sets);
  });

  test('keeps the two files apart, so startup reads stay small', () async {
    final controller = PlaylistCollectionController(store: store);
    await controller.add(await writePlaylist('driving.m3u'));

    expect(await indexFile().exists(), isTrue);
    expect(await trackSetsFile().exists(), isTrue);
    final index = jsonDecode(await indexFile().readAsString()) as Map;
    expect(index['entries'], hasLength(1));
    expect(
      (index['entries'] as List).single,
      isNot(contains('tracks')),
      reason: 'the index paints the panel; track paths live in the companion',
    );
  });

  group('malformed files', () {
    test('a malformed index reads as an empty collection', () async {
      await indexFile().writeAsString('{"entries": [');

      expect(await store.readIndex(), isEmpty);
    });

    test('a malformed index does not stop the collection bootstrapping',
        () async {
      await indexFile().writeAsString('not json at all');
      final controller = PlaylistCollectionController(store: store);

      await controller.bootstrap();

      expect(controller.entries, isEmpty);
    });

    test('an entry with no path is skipped, the rest survive', () async {
      final good = normalizePlaylistPath(p.join(musicDir.path, 'good.m3u'));
      await indexFile().writeAsString(jsonEncode({
        'entries': [
          {'trackCount': 3},
          {'path': good, 'trackCount': 3},
        ],
      }));

      final entries = await store.readIndex();

      expect(entries.map((e) => e.path), [good]);
    });

    test('a malformed companion file reads as no track sets', () async {
      await trackSetsFile().writeAsString('[]');

      expect(await store.readTrackSets(), isEmpty);
    });
  });

  test('the collection survives a restart', () async {
    final path = await writePlaylist('road trip.m3u');
    final first = PlaylistCollectionController(store: store);
    await first.add(path);

    final next = PlaylistCollectionController(
      store: FilePlaylistCollectionStore(supportDir: () async => supportDir),
    );
    await next.bootstrap();

    expect(next.entries, hasLength(1));
    expect(next.entries.single.path, normalizePlaylistPath(path));
    expect(next.entries.single.trackCount, 1);
    expect(next.entries.single.displayName, 'road trip');
  });

  test('resetting settings leaves the collection intact', () async {
    final path = await writePlaylist('keepsake.m3u');
    final collection = PlaylistCollectionController(store: store);
    await collection.add(path);

    // What Reset Settings does: rewrite settings.json with the defaults. It
    // touches nothing else, exactly as it already spares installed skins.
    final settings = FileSettingsStore(supportDir: () async => supportDir);
    await settings.write(
      TrampSettings.defaults.copyWith(playlistCollectionWidth: 320),
    );
    await settings.write(TrampSettings.defaults);

    final after = PlaylistCollectionController(store: store);
    await after.bootstrap();

    expect(after.entries.map((e) => e.path), [normalizePlaylistPath(path)]);
    expect(await after.readTrackSets(), isNotEmpty);
    expect(
      (await settings.read()).playlistCollectionWidth,
      TrampSettings.defaultPlaylistCollectionWidth,
      reason: 'the preference did reset — only content survived',
    );
  });
}
