import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/track.dart';

/// The persisted form of an **altered current playlist**: the whole track list,
/// plus the origin it was loaded from when it had one.
///
/// Only an altered list is kept here. An unaltered one already comes back
/// through the last-playlist path in `PlaylistStore`, and coming back that way
/// is what leaves it unaltered.
class AlteredPlaylist {
  const AlteredPlaylist({this.tracks = const [], this.sourcePath});

  /// What "no altered current playlist was kept" reads as — and what an
  /// unreadable file falls back to, so a bad file costs the pile rather than
  /// the launch.
  static const empty = AlteredPlaylist();

  final List<Track> tracks;
  final String? sourcePath;

  /// Nothing worth restoring. A list with no tracks and no origin is exactly
  /// what a listener sees when Tramp opens with nothing loaded, so writing one
  /// and restoring one are the same thing.
  bool get isEmpty => tracks.isEmpty && sourcePath == null;

  Map<String, dynamic> toJson() => {
        'sourcePath': sourcePath,
        'tracks': [for (final track in tracks) track.toJson()],
      };

  factory AlteredPlaylist.fromJson(Map<String, dynamic> json) {
    final source = json['sourcePath'];
    return AlteredPlaylist(
      sourcePath: source is String && source.isNotEmpty ? source : null,
      tracks: _decodeTracks(json['tracks']),
    );
  }

  static List<Track> _decodeTracks(Object? raw) {
    if (raw is! List) return const [];
    final tracks = <Track>[];
    for (final item in raw) {
      if (item is! Map) continue;
      try {
        tracks.add(Track.fromJson(Map<String, dynamic>.from(item)));
      } on FormatException {
        // One unreadable row must not cost the listener the rest of the pile.
        continue;
      }
    }
    return tracks;
  }
}

abstract class AlteredPlaylistStore {
  /// The altered current playlist kept from the last session, or
  /// [AlteredPlaylist.empty] when there is none — which is also the answer for
  /// a file that cannot be read.
  Future<AlteredPlaylist> read();

  Future<void> write(AlteredPlaylist playlist);

  /// Forgets the kept list, once the current playlist is no longer altered.
  Future<void> clear();
}

/// One JSON file in the app support dir, beside `settings.json`.
///
/// `session.json` is deliberately not extended: it means "last session", and it
/// is the one store whose reader has no error handling. This follows
/// `FileSettingsStore` instead and falls back to an empty playlist on any
/// decode failure, so a truncated or hand-edited file costs the kept list
/// rather than startup.
class FileAlteredPlaylistStore implements AlteredPlaylistStore {
  FileAlteredPlaylistStore({required this.supportDir});

  static const fileName = 'altered_playlist.json';

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, fileName));
  }

  @override
  Future<AlteredPlaylist> read() async {
    try {
      final file = await _file();
      if (!await file.exists()) return AlteredPlaylist.empty;
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return AlteredPlaylist.empty;
      return AlteredPlaylist.fromJson(Map<String, dynamic>.from(decoded));
    } catch (_) {
      // Deliberate catch-all: on-disk JSON is untrusted input, and an empty
      // playlist is recoverable where a throw on the startup path is not.
      return AlteredPlaylist.empty;
    }
  }

  @override
  Future<void> write(AlteredPlaylist playlist) async {
    final file = await _file();
    await file.writeAsString(jsonEncode(playlist.toJson()));
  }

  @override
  Future<void> clear() async {
    final file = await _file();
    if (await file.exists()) await file.delete();
  }
}
