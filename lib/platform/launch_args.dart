import 'package:path/path.dart' as p;

import 'file_open.dart';

class LaunchAction {
  const LaunchAction({this.openPlaylist, this.openTracks = const []});

  final String? openPlaylist;
  final List<String> openTracks;
}

LaunchAction parseLaunchArgs(List<String> args) {
  String? playlist;
  final tracks = <String>[];

  for (final arg in args) {
    if (!_isLikelyFilePath(arg)) continue;

    if (isPlaylistPath(arg)) {
      playlist ??= arg;
      continue;
    }

    if (isAudioPath(arg)) {
      tracks.add(arg);
    }
  }

  if (playlist != null) {
    return LaunchAction(openPlaylist: playlist);
  }

  return LaunchAction(openTracks: tracks);
}

bool _isLikelyFilePath(String arg) {
  if (arg.isEmpty || arg.startsWith('-')) return false;
  final ext = p.extension(arg);
  return ext.isNotEmpty;
}
