import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/launch_args.dart';

void main() {
  test('playlist path wins as openPlaylist', () {
    final a = parseLaunchArgs([r'D:\x.m3u8']);
    expect(a.openPlaylist, r'D:\x.m3u8');
    expect(a.openTracks, isEmpty);
  });

  test('audio paths collected', () {
    final a = parseLaunchArgs([r'D:\a.mp3', r'D:\b.flac']);
    expect(a.openTracks, [r'D:\a.mp3', r'D:\b.flac']);
    expect(a.openPlaylist, isNull);
  });

  test('playlist wins over audio paths in same launch', () {
    final a = parseLaunchArgs([r'D:\a.mp3', r'D:\x.m3u', r'D:\b.flac']);
    expect(a.openPlaylist, r'D:\x.m3u');
    expect(a.openTracks, isEmpty);
  });

  test('ignores flag-like arguments', () {
    final a = parseLaunchArgs(['--verbose', r'D:\a.mp3']);
    expect(a.openTracks, [r'D:\a.mp3']);
  });
}
