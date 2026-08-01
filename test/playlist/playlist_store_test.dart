import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playlist/playlist_store.dart';

void main() {
  test('FilePlaylistStore round-trips path', () async {
    final dir = await Directory.systemTemp.createTemp('tramp_store_');
    final store = FilePlaylistStore(supportDir: () async => dir);
    await store.writeLastPlaylistPath('/x.m3u');
    expect(await store.readLastPlaylistPath(), '/x.m3u');
  });
}
