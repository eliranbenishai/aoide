import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

Future<void> configureTrampWindow() async {
  const options = WindowOptions(
    size: Size(720, 520),
    minimumSize: Size(480, 360),
    center: true,
    backgroundColor: Colors.transparent,
    skipTaskbar: false,
    titleBarStyle: TitleBarStyle.hidden,
    title: 'Tramp',
  );
  await windowManager.waitUntilReadyToShow(options, () async {
    await windowManager.setAsFrameless();
    await windowManager.show();
    await windowManager.focus();
  });
}
