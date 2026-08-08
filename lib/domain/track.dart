import 'package:path/path.dart' as p;

class Track {
  const Track({
    required this.path,
    this.title,
    this.artist,
    this.album,
    this.duration,
  });

  final String path;
  final String? title;
  final String? artist;
  final String? album;
  final Duration? duration;

  String get displayTitle =>
      (title != null && title!.trim().isNotEmpty) ? title!.trim() : p.basename(path);

  Track copyWith({
    String? path,
    String? title,
    String? artist,
    String? album,
    Duration? duration,
  }) {
    return Track(
      path: path ?? this.path,
      title: title ?? this.title,
      artist: artist ?? this.artist,
      album: album ?? this.album,
      duration: duration ?? this.duration,
    );
  }

  Map<String, dynamic> toJson() => {
        'path': path,
        'title': title,
        'artist': artist,
        'album': album,
        'durationMs': duration?.inMilliseconds,
      };

  factory Track.fromJson(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('Track.path');
    }
    final ms = json['durationMs'];
    return Track(
      path: path,
      title: json['title'] as String?,
      artist: json['artist'] as String?,
      album: json['album'] as String?,
      duration: ms is num ? Duration(milliseconds: ms.toInt()) : null,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is Track &&
      other.path == path &&
      other.title == title &&
      other.artist == artist &&
      other.album == album &&
      other.duration == duration;

  @override
  int get hashCode => Object.hash(path, title, artist, album, duration);
}
