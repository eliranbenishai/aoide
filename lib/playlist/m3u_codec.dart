import 'package:path/path.dart' as p;

import '../domain/track.dart';

class M3uCodec {
  const M3uCodec();

  List<Track> parse(String contents, {required String playlistFilePath}) {
    final dir = p.dirname(playlistFilePath);
    final tracks = <Track>[];
    Duration? pendingDuration;
    String? pendingTitle;
    String? pendingArtist;

    for (final rawLine in contents.split(RegExp(r'\r?\n'))) {
      final line = rawLine.trim();
      if (line.isEmpty) continue;
      if (line.startsWith('#EXTINF:')) {
        final rest = line.substring('#EXTINF:'.length);
        final comma = rest.indexOf(',');
        final durationPart = comma >= 0 ? rest.substring(0, comma) : rest;
        final meta = comma >= 0 ? rest.substring(comma + 1).trim() : '';
        pendingDuration =
            Duration(seconds: int.tryParse(durationPart.trim()) ?? 0);
        if (meta.contains(' - ')) {
          final parts = meta.split(' - ');
          pendingArtist = parts.first.trim();
          pendingTitle = parts.sublist(1).join(' - ').trim();
        } else if (meta.isNotEmpty) {
          pendingTitle = meta;
        }
        continue;
      }
      if (line.startsWith('#')) continue;

      final trackPath = p.isAbsolute(line)
          ? p.normalize(line)
          : p.normalize(p.join(dir, line));
      tracks.add(Track(
        path: trackPath,
        title: pendingTitle,
        artist: pendingArtist,
        duration: pendingDuration,
      ));
      pendingDuration = null;
      pendingTitle = null;
      pendingArtist = null;
    }
    return tracks;
  }

  String encode(List<Track> tracks) {
    final buf = StringBuffer('#EXTM3U\n');
    for (final t in tracks) {
      final secs = t.duration?.inSeconds;
      final label = [
        if (t.artist != null && t.artist!.isNotEmpty) t.artist,
        if (t.title != null && t.title!.isNotEmpty) t.title,
      ].whereType<String>().join(' - ');
      if (secs != null) {
        buf.writeln('#EXTINF:$secs,${label.isEmpty ? t.displayTitle : label}');
      }
      buf.writeln(t.path);
    }
    return buf.toString();
  }
}
