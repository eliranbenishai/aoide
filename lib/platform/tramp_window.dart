import 'dart:io';

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

/// True when [actual] is within [tolerance] px of [expected] on both axes.
///
/// Used after [resizeTrampWindow] to detect hosts (notably Linux/GTK) that
/// acknowledge `setSize` but keep the window at a stale default.
bool trampWindowSizeMatches(
  Size actual,
  Size expected, {
  double tolerance = 2,
}) {
  return (actual.width - expected.width).abs() <= tolerance &&
      (actual.height - expected.height).abs() <= tolerance;
}

/// Frameless window sized for the active zoom step.
Future<void> configureTrampWindow({
  required Size size,
  required Size minimumSize,
}) async {
  final options = WindowOptions(
    size: size,
    minimumSize: minimumSize,
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

/// Applies a new pixel size to the live window.
///
/// The minimum is set before the size so growing is never rejected for sitting
/// below a stale floor, and shrinking is never clamped by the previous step's
/// larger minimum.
///
/// When [pinSize] is true (main / EQ / settings), also sets the maximum to
/// [size] so GTK cannot keep a larger default (e.g. 1280×720). Playlist passes
/// false and clears any prior max hint.
///
/// On Linux, verifies [windowManager.getSize] and nudges (`size+1` then
/// `size`) if the window stayed oversized — a known `window_manager`/GTK flake.
Future<void> resizeTrampWindow({
  required Size size,
  required Size minimumSize,
  bool pinSize = false,
}) async {
  await windowManager.setMinimumSize(minimumSize);
  if (pinSize) {
    await windowManager.setMaximumSize(size);
  } else {
    // Clear GDK_HINT_MAX_SIZE (plugin treats negative as unlimited).
    await windowManager.setMaximumSize(const Size(-1, -1));
  }
  await windowManager.setSize(size);

  if (!Platform.isLinux) return;

  for (var attempt = 0; attempt < 3; attempt++) {
    // Let configure-event / compositor catch up before measuring.
    await Future<void>.delayed(const Duration(milliseconds: 32));
    final actual = await windowManager.getSize();
    if (trampWindowSizeMatches(actual, size)) return;
    // Nudge forces GTK to re-apply geometry (see window_manager#311 pattern).
    await windowManager.setSize(Size(size.width + 1, size.height + 1));
    await windowManager.setSize(size);
  }
}

Future<void> setTrampWindowResizable(bool resizable) async {
  await windowManager.setResizable(resizable);
}
