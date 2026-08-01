import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/platform/file_open.dart';

void main() {
  test('tracksFromPaths returns only audio files from paths', () async {
    final dir = await Directory.systemTemp.createTemp('tramp_file_open_');
    final mp3A = p.join(dir.path, 'a.mp3');
    final mp3B = p.join(dir.path, 'b.mp3');
    final txt = p.join(dir.path, 'notes.txt');
    await File(mp3A).writeAsString('');
    await File(mp3B).writeAsString('');
    await File(txt).writeAsString('ignore');

    final tracks = tracksFromPaths([mp3A, mp3B, txt]);

    expect(tracks, hasLength(2));
    expect(tracks.map((t) => t.path), orderedEquals([mp3A, mp3B]));
  });
}
