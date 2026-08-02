import 'package:flutter/widgets.dart';
import 'package:media_kit/media_kit.dart';
import 'package:window_manager/window_manager.dart';

import 'app.dart';
import 'platform/tramp_window.dart';
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
  await configureTrampWindow(
    size: probe.windowSizeFor(initialPercent),
    minimumSize: probe.minimumWindowSizeFor(initialPercent),
  );
  runApp(TrampApp(launchArgs: args));
}
