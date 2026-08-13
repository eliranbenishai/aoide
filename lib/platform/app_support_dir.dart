import 'dart:io';

import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';

/// Tramp's GTK application id, mirroring `APPLICATION_ID` in
/// `linux/CMakeLists.txt`. The two must agree.
const trampApplicationId = 'com.tramp.tramp';

/// Directory name `path_provider` used before it learned to read the
/// application id — `basename(/proc/self/exe)`, i.e. `BINARY_NAME`.
const _legacyLinuxSupportDirName = 'tramp';

/// Where Tramp keeps everything it remembers: settings, the playlist
/// collection, an altered current playlist, resume state, spins, and installed
/// skins.
///
/// On Linux this is resolved **here** rather than left to `path_provider`,
/// which asks the running process for its GTK application id over FFI into
/// libgio and silently falls back to the executable name when that lookup
/// returns null — no library, or no `GApplication` registered yet at the moment
/// it is first called. Those two answers are different directories, so a lookup
/// that failed once would strand every skin and playlist the listener had, with
/// no error and nothing to see. Pinning the id makes the answer the same on
/// every launch and in every engine.
///
/// Windows and macOS keep `path_provider`: both derive the path from packaging
/// metadata fixed at build time, so neither can move underneath a running app.
Future<Directory> trampSupportDirectory() async {
  if (!Platform.isLinux) {
    final directory = await getApplicationSupportDirectory();
    await directory.create(recursive: true);
    return directory;
  }
  final directory = Directory(
    resolveLinuxSupportPath(
      environment: Platform.environment,
      exists: (path) => Directory(path).existsSync(),
    ),
  );
  await directory.create(recursive: true);
  return directory;
}

/// The pure rule behind [trampSupportDirectory] on Linux, split out so it can
/// be exercised without a filesystem or a real environment.
///
/// Prefers the application-id directory, but adopts the legacy executable-name
/// one when that is the directory that actually holds data — otherwise pinning
/// the id would itself strand anyone whose data landed there under the old
/// behaviour.
String resolveLinuxSupportPath({
  required Map<String, String> environment,
  required bool Function(String path) exists,
}) {
  final dataHome = _linuxDataHome(environment);
  final pinned = p.join(dataHome, trampApplicationId);
  if (exists(pinned)) return pinned;
  final legacy = p.join(dataHome, _legacyLinuxSupportDirName);
  if (exists(legacy)) return legacy;
  return pinned;
}

/// `$XDG_DATA_HOME`, or the spec's `$HOME/.local/share` default. A relative
/// `XDG_DATA_HOME` is ignored, as the spec requires.
String _linuxDataHome(Map<String, String> environment) {
  final xdg = environment['XDG_DATA_HOME'];
  if (xdg != null && xdg.isNotEmpty && p.isAbsolute(xdg)) return xdg;
  final home = environment['HOME'];
  if (home != null && home.isNotEmpty) {
    return p.join(home, '.local', 'share');
  }
  // No HOME at all is not a situation with a good answer; the current
  // directory at least keeps the app running instead of throwing at startup.
  return p.join(Directory.current.path, '.local', 'share');
}
