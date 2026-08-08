import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:media_kit/media_kit.dart';
import 'package:window_manager/window_manager.dart';

import 'platform/libmpv_bundle.dart';
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
    // Fail fast in debug/profile if packaging still points at slim libmpv.
    await LibmpvBundle.verify(enforce: !kReleaseMode);
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
