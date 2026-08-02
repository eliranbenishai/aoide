import 'package:flutter/widgets.dart';
import 'package:media_kit/media_kit.dart';
import 'package:window_manager/window_manager.dart';

import 'app.dart';
import 'platform/tramp_window.dart';
import 'ui/window_layout.dart';
import 'ui/zoom/zoom_controller.dart';

Future<void> main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  MediaKit.ensureInitialized();
  await windowManager.ensureInitialized();

  // Display work-area APIs are not reliable before the window exists; use a
  // common Full-HD fallback. ZoomController.workArea can be updated later.
  const workArea = Size(1920, 1080);
  final initialPercent = ZoomController.bestInitialPercent(workArea);
  final probe = ZoomController(workArea: workArea);
  // Open at the playlist-mode default; once settings load the app snaps to
  // the restored region and any persisted playlist size.
  await configureTrampWindow(
    size: defaultPlaylistModeWindowSize(initialPercent / 100),
    minimumSize: probe.minimumWindowSizeFor(initialPercent),
  );
  runApp(TrampApp(launchArgs: args));
}
