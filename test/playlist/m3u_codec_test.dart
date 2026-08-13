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

  group('track lines written by another platform', () {
    final albumDir =
        p.join(p.separator, 'mnt', 'share', 'Enigma', '1990 - MCMXC a.D');
    final playlistFile = p.join(albumDir, 'Enigma - M C M X C a. D.m3u');
    final realTrack = p.join(albumDir, '01 - The Voice Of Enigma.flac');

    test('a Windows UNC line finds the file beside the playlist', () {
      final raw = '#EXTM3U\n'
          '#EXTINF:141,Enigma - The Voice Of Enigma\n'
          r'\\eliranas\NAS\Media\Music\Enigma\1990 - MCMXC a.D\01 - The Voice Of Enigma.flac'
          '\n';
      final codec = M3uCodec(exists: {realTrack}.contains);

      final tracks = codec.parse(raw, playlistFilePath: playlistFile);

      expect(tracks.single.path, realTrack);
    });

    test('a line under a subfolder keeps that subfolder', () {
      final discTrack = p.join(albumDir, 'Disc 2', '03 - Callas Went Away.flac');
      final raw = '#EXTM3U\n'
          r'\\eliranas\NAS\Music\Enigma\1990 - MCMXC a.D\Disc 2\03 - Callas Went Away.flac'
          '\n';
      final codec = M3uCodec(exists: {discTrack}.contains);

      final tracks = codec.parse(raw, playlistFilePath: playlistFile);

      expect(tracks.single.path, discTrack);
    });

    test('an absolute line whose mount has moved finds the track again', () {
      final stale = p.join(p.separator, 'run', 'user', '1000', 'kio-fuse-XkqMpT',
          'Enigma', '1990 - MCMXC a.D', '01 - The Voice Of Enigma.flac');
      final raw = '#EXTM3U\n$stale\n';
      final codec = M3uCodec(exists: {realTrack}.contains);

      final tracks = codec.parse(raw, playlistFilePath: playlistFile);

      expect(tracks.single.path, realTrack);
    });

    test('an absolute line that exists is left where it points', () {
      final elsewhere = p.join(
          p.separator, 'music', 'singles', '01 - The Voice Of Enigma.flac');
      final raw = '#EXTM3U\n$elsewhere\n';
      final codec = M3uCodec(exists: {elsewhere, realTrack}.contains);

      final tracks = codec.parse(raw, playlistFilePath: playlistFile);

      expect(tracks.single.path, elsewhere);
    });

    test('a line that resolves nowhere still yields a track', () {
      final raw = '#EXTM3U\n'
          '#EXTINF:141,Enigma - The Voice Of Enigma\n'
          r'\\eliranas\NAS\Media\gone.flac'
          '\n';
      final codec = M3uCodec(exists: (_) => false);

      final tracks = codec.parse(raw, playlistFilePath: playlistFile);

      expect(tracks, hasLength(1));
      expect(tracks.single.path, contains('gone.flac'));
      expect(tracks.single.title, 'The Voice Of Enigma');
    });
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
