import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:media_kit/media_kit.dart';
import 'package:window_manager/window_manager.dart';

import 'platform/libmpv_bundle.dart';
import 'playback/fake_player_engine.dart';
import 'playback/player_engine.dart';
import 'ui/session/session_client.dart';
import 'ui/session/session_host.dart';
import 'ui/session/session_messages.dart';
import 'ui/session/session_quit.dart';

Future<void> main(List<String> args) async {
  await runZonedGuarded(() async {
    await _main(args);
  }, (error, stack) {
    debugPrint('Tramp zone error: $error\n$stack');
  });
}

Future<void> _main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();

  final windowController = await WindowController.fromCurrentEngine();
  final role = parseWindowRole(windowController.arguments);

  if (role == WindowRole.main) {
    PlayerEngine? engine;
    try {
      MediaKit.ensureInitialized();
      // Fail fast in debug/profile if packaging still points at slim libmpv.
      await LibmpvBundle.verify(enforce: !kReleaseMode);
    } catch (error, stack) {
      if (!kReleaseMode && !trampAutoQuitRequested()) {
        rethrow;
      }
      // Release: keep the chrome up so a missing DLL is not a silent exit.
      debugPrint(
        'media_kit init failed, using FakePlayerEngine: $error\n$stack',
      );
      engine = FakePlayerEngine();
    }
    runApp(SessionHostApp(launchArgs: args, engine: engine));
    return;
  }

  runApp(
    SessionClientApp(
      role: role,
      windowController: windowController,
    ),
  );
}
