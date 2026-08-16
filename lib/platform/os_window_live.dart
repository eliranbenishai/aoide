// ignore_for_file: invalid_use_of_internal_member
// ignore_for_file: implementation_imports

import 'dart:io';
import 'dart:ui' show FlutterView, Size;

import 'package:flutter/src/widgets/_window.dart';

import 'os_window.dart';
import 'os_window_native_io.dart';

OsWindow createLiveOsWindow({
  required Size size,
  required String title,
  bool skipTaskbar = true,
  bool decorated = false,
  bool resizable = false,
}) {
  final controller = RegularWindowController(
    size: size,
    title: title,
  );
  final native = ioOsWindowNative(controller);
  native.setDecorated(decorated);
  native.setSkipTaskbar(skipTaskbar);
  native.setResizable(resizable);
  native.hide();
  return OsWindow.live(native: native, controller: controller);
}

FlutterView liveViewOf(Object controller) {
  return (controller as RegularWindowController).rootView;
}

OsWindowNative ioOsWindowNative(RegularWindowController controller) {
  if (Platform.isLinux) {
    return LinuxOsWindowNative(controller);
  }
  if (Platform.isWindows) {
    return Win32OsWindowNative(controller);
  }
  if (Platform.isMacOS) {
    return MacOsWindowNative(controller);
  }
  throw UnsupportedError('OsWindow: ${Platform.operatingSystem}');
}
