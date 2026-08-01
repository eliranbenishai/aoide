import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/m3u_codec.dart';

void main() {
  const codec = M3uCodec();

  test('parses EXTINF and relative paths', () {
    const raw = '''
#EXTM3U
#EXTINF:221,Wire Garden - Static Hymn
tracks/static.mp3
# comment
other.flac
''';
    final playlist = p.join('music', 'lists', 'go.m3u');
    final tracks = codec.parse(
      raw,
      playlistFilePath: playlist,
    );
    expect(tracks, hasLength(2));
    expect(
      tracks[0].path,
      p.normalize(p.join('music', 'lists', 'tracks', 'static.mp3')),
    );
    expect(tracks[0].title, 'Static Hymn');
    expect(tracks[0].artist, 'Wire Garden');
    expect(tracks[0].duration, const Duration(seconds: 221));
    expect(
      tracks[1].path,
      p.normalize(p.join('music', 'lists', 'other.flac')),
    );
  });

  test('encode round-trips absolute paths', () {
    final absolutePath = p.normalize(p.join(p.current, 'a.mp3'));
    final out = codec.encode([
      Track(
        path: absolutePath,
        title: 'A',
        artist: 'X',
        duration: const Duration(seconds: 10),
      ),
    ]);
    expect(out.split('\n').first, '#EXTM3U');
    expect(out, contains('#EXTINF:10,X - A'));
    expect(out, contains(absolutePath));
  });
}
