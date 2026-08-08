import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/widgets.dart';
import 'package:media_kit/media_kit.dart';
import 'package:window_manager/window_manager.dart';

import 'ui/session/session_client.dart';
import 'ui/session/session_host.dart';
import 'ui/session/session_messages.dart';

Future<void> main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();

  final windowController = await WindowController.fromCurrentEngine();
  final role = parseWindowRole(windowController.arguments);

  if (role == WindowRole.main) {
    MediaKit.ensureInitialized();
    runApp(SessionHostApp(launchArgs: args));
    return;
  }

  runApp(
    SessionClientApp(
      role: role,
      windowController: windowController,
    ),
  );
}
