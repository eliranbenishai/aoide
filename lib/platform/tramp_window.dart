import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../theme/tramp_metrics.dart';

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

/// Applies a new zoom step to the live window.
///
/// The minimum is set before the size so growing is never rejected for sitting
/// below a stale floor, and shrinking is never clamped by the previous step's
/// larger minimum.
Future<void> resizeTrampWindow({
  required Size size,
  required Size minimumSize,
}) async {
  await windowManager.setMinimumSize(minimumSize);
  await windowManager.setSize(size);
}

/// Default playlist-mode window size when no persisted dimensions exist.
Size defaultPlaylistWindowSize(int zoomPercent) {
  final f = zoomPercent / 100;
  final width =
      (TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2) * f;
  final height = (TrampMetrics.frame * 2 +
          TrampMetrics.mainPlayer.height +
          TrampMetrics.gutter +
          TrampMetrics.defaultPlaylistWellHeight) *
      f;
  return Size(width, height);
}

Future<void> setTrampWindowResizable(bool resizable) async {
  await windowManager.setResizable(resizable);
}
