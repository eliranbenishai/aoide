import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

/// Playback resume snapshot persisted beside settings (not wiped by Reset).
class SessionResume {
  const SessionResume({
    this.playingIndex,
    this.positionMs = 0,
    this.wasPlaying = false,
  });

  static const empty = SessionResume();

  final int? playingIndex;
  final int positionMs;
  final bool wasPlaying;

  Map<String, dynamic> toJson() => {
        'playingIndex': playingIndex,
        'positionMs': positionMs,
        'wasPlaying': wasPlaying,
      };

  factory SessionResume.fromJson(Map<String, dynamic> json) {
    return SessionResume(
      playingIndex: (json['playingIndex'] as num?)?.toInt(),
      positionMs: (json['positionMs'] as num?)?.toInt() ?? 0,
      wasPlaying: json['wasPlaying'] == true,
    );
  }
}

abstract class SessionResumeStore {
  Future<SessionResume> read();
  Future<void> write(SessionResume resume);
}

class FileSessionResumeStore implements SessionResumeStore {
  FileSessionResumeStore({required this.supportDir});

  final Future<Directory> Function() supportDir;

  Future<File> _file() async {
    final dir = await supportDir();
    return File(p.join(dir.path, 'session_resume.json'));
  }

  @override
  Future<SessionResume> read() async {
    try {
      final file = await _file();
      if (!await file.exists()) return SessionResume.empty;
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return SessionResume.empty;
      return SessionResume.fromJson(Map<String, dynamic>.from(decoded));
    } catch (_) {
      return SessionResume.empty;
    }
  }

  @override
  Future<void> write(SessionResume resume) async {
    final file = await _file();
    await file.parent.create(recursive: true);
    await file.writeAsString(jsonEncode(resume.toJson()));
  }
}
