import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

abstract class PlaylistStore {
  Future<String?> readLastPlaylistPath();
  Future<void> writeLastPlaylistPath(String? path);
}

class FilePlaylistStore implements PlaylistStore {
  FilePlaylistStore({required this.supportDir});

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    await dir.create(recursive: true);
    return File(p.join(dir.path, 'session.json'));
  }

  @override
  Future<String?> readLastPlaylistPath() async {
    final f = await _file();
    if (!await f.exists()) return null;
    final map = jsonDecode(await f.readAsString()) as Map<String, dynamic>;
    return map['lastPlaylistPath'] as String?;
  }

  @override
  Future<void> writeLastPlaylistPath(String? path) async {
    final f = await _file();
    await f.writeAsString(jsonEncode({'lastPlaylistPath': path}));
  }
}
