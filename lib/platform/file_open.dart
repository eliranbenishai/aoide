import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:path/path.dart' as p;

import '../domain/track.dart';

const audioExtensions = {
  '.mp3',
  '.m4a',
  '.aac',
  '.flac',
  '.wav',
  '.ogg',
  '.opus',
};

const playlistExtensions = {'.m3u', '.m3u8'};

bool isPlaylistPath(String path) =>
    playlistExtensions.contains(p.extension(path).toLowerCase());

bool isAudioPath(String path) =>
    audioExtensions.contains(p.extension(path).toLowerCase());

List<String> _collectAudioFiles(Directory dir) {
  final paths = <String>[];
  for (final entity in dir.listSync(recursive: true, followLinks: false)) {
    if (entity is File && isAudioPath(entity.path)) {
      paths.add(entity.path);
    }
  }
  paths.sort(
    (a, b) =>
        p.basename(a).toLowerCase().compareTo(p.basename(b).toLowerCase()),
  );
  return paths;
}

List<Track> tracksFromPaths(List<String> paths) {
  final audioPaths = <String>[];
  for (final path in paths) {
    final type = FileSystemEntity.typeSync(path, followLinks: false);
    if (type == FileSystemEntityType.directory) {
      audioPaths.addAll(_collectAudioFiles(Directory(path)));
    } else if (type == FileSystemEntityType.file && isAudioPath(path)) {
      audioPaths.add(path);
    }
  }
  audioPaths.sort(
    (a, b) =>
        p.basename(a).toLowerCase().compareTo(p.basename(b).toLowerCase()),
  );
  return audioPaths.map((path) => Track(path: path)).toList();
}

/// Maps [file_picker]'s missing-dialog-tool exception to a clearer [StateError].
///
/// Linux pickers shell out to `qarma`, `kdialog`, or `zenity`. Distrobox images
/// often omit all three. Returns null when [error] is unrelated.
StateError? linuxFileDialogMissingToolError(Object error) {
  final text = error.toString();
  if (!text.contains("Couldn't find the executable")) return null;
  return StateError(
    'Linux file dialogs need zenity or kdialog on PATH '
    '(Fedora/Distrobox: sudo dnf install -y zenity). $error',
  );
}

Future<T> _linuxFileDialog<T>(Future<T> Function() action) async {
  try {
    return await action();
  } on Exception catch (error) {
    if (!Platform.isLinux) rethrow;
    final mapped = linuxFileDialogMissingToolError(error);
    if (mapped == null) rethrow;
    throw mapped;
  }
}

Future<List<String>?> pickAudioFiles() async {
  return _linuxFileDialog(() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions:
          audioExtensions.map((ext) => ext.substring(1)).toList(),
      allowMultiple: true,
      dialogTitle: 'Add audio files',
    );
    if (result == null) return null;
    return result.paths.whereType<String>().toList();
  });
}

Future<String?> pickPlaylistFile() async {
  return _linuxFileDialog(() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['m3u', 'm3u8'],
      dialogTitle: 'Open playlist',
    );
    if (result == null || result.files.isEmpty) return null;
    return result.files.first.path;
  });
}

Future<String?> pickSavePlaylistPath() async {
  return _linuxFileDialog(
    () => FilePicker.platform.saveFile(
      dialogTitle: 'Save playlist',
      fileName: 'playlist.m3u',
      type: FileType.custom,
      allowedExtensions: ['m3u'],
    ),
  );
}

Future<String?> pickFolder() async {
  return _linuxFileDialog(
    () => FilePicker.platform.getDirectoryPath(dialogTitle: 'Open folder'),
  );
}
