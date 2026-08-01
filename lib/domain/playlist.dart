import 'track.dart';

class Playlist {
  const Playlist({this.sourcePath, this.tracks = const []});

  final String? sourcePath;
  final List<Track> tracks;

  Playlist copyWith({String? sourcePath, List<Track>? tracks}) {
    return Playlist(
      sourcePath: sourcePath ?? this.sourcePath,
      tracks: tracks ?? this.tracks,
    );
  }
}
