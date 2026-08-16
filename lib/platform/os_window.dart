// ignore_for_file: invalid_use_of_internal_member
// ignore_for_file: implementation_imports

import 'dart:ui' show FlutterView, Offset, Size;

import 'package:flutter/src/foundation/_features.dart' show isWindowingEnabled;
import 'package:flutter/widgets.dart';
import 'package:window_manager/window_manager.dart' show ResizeEdge;

import 'os_window_live.dart';
import 'os_window_native.dart';

export 'os_window_native.dart';

/// Turn on Flutter's experimental windowing types before [WidgetsFlutterBinding].
void enableTrampWindowing() {
  isWindowingEnabled = true;
}

/// One extra product OS window on the **same** Flutter engine as main.
///
/// Does not rebuild widgets when the HWND moves — do not wrap [attach] in a
/// [ListenableBuilder] on the windowing controller.
class OsWindow {
  OsWindow._({
    required this.native,
    this.controller,
  });

  /// Test / fake window — no Flutter [View].
  factory OsWindow.test(OsWindowNative native) => OsWindow._(native: native);

  factory OsWindow.live({
    required OsWindowNative native,
    required Object controller,
  }) =>
      OsWindow._(native: native, controller: controller);

  /// Live windowing-API window (Linux/Win/macOS).
  factory OsWindow.create({
    required Size size,
    required String title,
    bool skipTaskbar = true,
    bool decorated = false,
    bool resizable = false,
  }) {
    return createLiveOsWindow(
      size: size,
      title: title,
      skipTaskbar: skipTaskbar,
      decorated: decorated,
      resizable: resizable,
    );
  }

  final OsWindowNative native;
  final Object? controller;

  FlutterView get view {
    final live = controller;
    if (live == null) {
      throw StateError('OsWindow.test has no FlutterView');
    }
    return liveViewOf(live);
  }

  /// Attach [child] to this window without listening to move/configure.
  Widget attach(Widget child) {
    return View(view: view, child: child);
  }

  Offset getPosition() => native.getPosition();

  Size getSize() => native.getSize();

  void startDrag() => native.startDrag();

  void startResize(ResizeEdge edge) => native.startResize(edge);

  void raise({bool focus = true}) => native.raise(focus: focus);

  void destroy() {
    native.destroy();
  }

  /// Apply a dock frame. [positionOnly] is the drag path: move, do not resize.
  void applyFrame({
    required double left,
    required double top,
    required double width,
    required double height,
    required bool visible,
    required bool alwaysOnTop,
    bool positionOnly = false,
  }) {
    if (positionOnly) {
      native.setPosition(left, top);
      return;
    }
    native.setSize(width, height);
    native.setPosition(left, top);
    native.setAlwaysOnTop(alwaysOnTop);
    if (visible) {
      native.show();
    } else {
      native.hide();
    }
  }
}
