import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/playlist.dart';
import 'package:tramp/domain/track.dart';

void main() {
  test('copyWith replaces tracks immutably', () {
    const a = Track(path: '/a.mp3');
    const b = Track(path: '/b.mp3');
    final p = Playlist(tracks: [a]);
    final next = p.copyWith(tracks: [a, b], sourcePath: '/list.m3u');
    expect(p.tracks, hasLength(1));
    expect(next.tracks, [a, b]);
    expect(next.sourcePath, '/list.m3u');
  });
}
