import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/altered_playlist_store.dart';

void main() {
  late Directory supportDir;
  late FileAlteredPlaylistStore store;

  const pile = [
    Track(path: '/music/a.mp3', title: 'Alpha', artist: 'A'),
    Track(
      path: '/music/b.mp3',
      title: 'Bravo',
      album: 'Bees',
      year: 1994,
      duration: Duration(seconds: 212),
    ),
  ];

  setUp(() async {
    supportDir = await Directory.systemTemp.createTemp('tramp_support_');
    store = FileAlteredPlaylistStore(supportDir: () async => supportDir);
  });

  tearDown(() async {
    if (await supportDir.exists()) await supportDir.delete(recursive: true);
  });

  File keptFile() =>
      File(p.join(supportDir.path, FileAlteredPlaylistStore.fileName));

  test('reads empty before anything has been kept', () async {
    final kept = await store.read();

    expect(kept.isEmpty, isTrue);
    expect(kept.tracks, isEmpty);
    expect(kept.sourcePath, isNull);
  });

  test('an ad-hoc pile round-trips through a real directory', () async {
    await store.write(const AlteredPlaylist(tracks: pile));

    final kept = await store.read();

    expect(kept.isEmpty, isFalse);
    expect(kept.tracks, pile);
    expect(kept.sourcePath, isNull, reason: 'a dropped pile has no origin');
  });

  test('the origin comes back with it', () async {
    const origin = '/music/driving.m3u';

    await store.write(
      const AlteredPlaylist(tracks: pile, sourcePath: origin),
    );

    expect((await store.read()).sourcePath, origin);
  });

  test('nothing is kept beside the last-session file', () async {
    await store.write(const AlteredPlaylist(tracks: pile));

    expect(await keptFile().exists(), isTrue);
    expect(
      await File(p.join(supportDir.path, 'session.json')).exists(),
      isFalse,
      reason: '"last session" is a different store, with no error handling',
    );
  });

  test('clearing forgets the kept list', () async {
    await store.write(const AlteredPlaylist(tracks: pile));

    await store.clear();

    expect(await keptFile().exists(), isFalse);
    expect((await store.read()).isEmpty, isTrue);
  });

  test('clearing what was never kept is not an error', () async {
    await store.clear();

    expect((await store.read()).isEmpty, isTrue);
  });

  group('malformed files', () {
    test('a truncated file reads as an empty playlist', () async {
      await keptFile().writeAsString('{"tracks": [');

      final kept = await store.read();

      expect(kept.isEmpty, isTrue);
    });

    test('a file that is not an object at all reads as empty', () async {
      await keptFile().writeAsString('[]');

      expect((await store.read()).isEmpty, isTrue);
    });

    test('a directory where the file should be reads as empty', () async {
      await Directory(keptFile().path).create(recursive: true);

      expect((await store.read()).isEmpty, isTrue);
    });

    test('a track with no path is skipped, the rest of the pile survives',
        () async {
      await keptFile().writeAsString(jsonEncode({
        'sourcePath': '/music/driving.m3u',
        'tracks': [
          {'title': 'no path at all'},
          {'path': '/music/b.mp3', 'title': 'Bravo'},
        ],
      }));

      final kept = await store.read();

      expect(kept.tracks.map((t) => t.path), ['/music/b.mp3']);
      expect(kept.sourcePath, '/music/driving.m3u');
    });
  });
}
