import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/track.dart';

bool _fileExists(String path) => File(path).existsSync();

class M3uCodec {
  const M3uCodec({this.exists = _fileExists});

  /// How the codec asks whether a candidate path is really a file.
  ///
  /// Injected so path resolution can be exercised without a filesystem.
  final bool Function(String path) exists;

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

      final trackPath = _resolve(line, dir);
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

  /// Where [line] actually points on *this* machine.
  ///
  /// A playlist file is a document other tools and other platforms write, so a
  /// track line is a hint rather than an address: the same album is `\\server\`
  /// on Windows and a mount point here, and a mount point that moves leaves
  /// every absolute line stale. What survives all of that is that the tracks
  /// sit beside the playlist, so an unusable line is re-read as a path relative
  /// to the playlist's own directory.
  ///
  /// The longest tail of the line that lands on a real file wins, which keeps
  /// an album's `Disc 2/` intact rather than flattening it to a filename. The
  /// walk costs one probe per path segment and is only paid by a line that
  /// failed outright, so a playlist whose paths are good never pays it.
  String _resolve(String line, String dir) {
    final direct =
        p.isAbsolute(line) ? p.normalize(line) : p.normalize(p.join(dir, line));
    if (exists(direct)) return direct;
    final segments = _segments(line);
    for (var take = segments.length; take >= 1; take--) {
      final candidate = p.normalize(
        p.join(dir, p.joinAll(segments.sublist(segments.length - take))),
      );
      if (candidate != direct && exists(candidate)) return candidate;
    }
    return direct;
  }

  /// [line] split on either platform's separator, so a Windows or UNC path
  /// yields its parts here rather than one long filename.
  static List<String> _segments(String line) => [
        for (final segment in line.split(RegExp(r'[\\/]+')))
          if (segment.isNotEmpty && segment != '.') segment,
      ];

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
