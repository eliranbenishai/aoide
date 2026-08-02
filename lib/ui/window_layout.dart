import 'dart:math' as math;

import 'package:flutter/painting.dart';

import '../domain/tramp_settings.dart';
import '../theme/tramp_metrics.dart';

/// Pure window- and stack-sizing rules for the two lower-region modes
/// (ADR 0003): equalizer mode is a fixed stack scaled only by the zoom step;
/// playlist mode freely resizes around the fixed main canvas.
///
/// Persisted playlist sizes are stored *logical* (zoom-independent) so a size
/// saved at one zoom step restores proportionally at another.

/// Fixed window size in equalizer mode: the exact panel stack at [factor].
///
/// When [collapsed] the equalizer is a windowshade, so the lower region snaps
/// to the title-bar strip height rather than the full panel.
Size eqModeWindowSize(double factor, {bool collapsed = false}) => Size(
      (TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2) * factor,
      (TrampMetrics.frame * 2 +
              TrampMetrics.mainPlayer.height +
              TrampMetrics.gutter +
              (collapsed
                  ? TrampMetrics.titleBar
                  : TrampMetrics.equalizer.height)) *
          factor,
    );

/// Smallest playlist-mode window: the whole main player plus the minimum well.
Size playlistModeMinimumSize(double factor) => Size(
      (TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2) * factor,
      (TrampMetrics.frame * 2 +
              TrampMetrics.mainPlayer.height +
              TrampMetrics.gutter +
              TrampMetrics.minLowerRegion) *
          factor,
    );

/// Default playlist-mode window when no size has been persisted yet: main
/// width, tall well.
Size defaultPlaylistModeWindowSize(double factor) => Size(
      (TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2) * factor,
      (TrampMetrics.frame * 2 +
              TrampMetrics.mainPlayer.height +
              TrampMetrics.gutter +
              TrampMetrics.defaultPlaylistWellHeight) *
          factor,
    );

/// Playlist-mode window size at [factor]: the persisted logical size scaled
/// up and clamped to the mode minimum, or the default tall size when either
/// stored dimension is missing.
Size playlistModeWindowSize({
  required double factor,
  double? storedWidth,
  double? storedHeight,
}) {
  final min = playlistModeMinimumSize(factor);
  if (storedWidth == null || storedHeight == null) {
    final fallback = defaultPlaylistModeWindowSize(factor);
    return Size(
      math.max(fallback.width, min.width),
      math.max(fallback.height, min.height),
    );
  }
  return Size(
    math.max(storedWidth * factor, min.width),
    math.max(storedHeight * factor, min.height),
  );
}

/// Zoom-independent logical form of a live playlist-mode window size, for
/// persisting in settings.
Size logicalPlaylistWindowSize(Size windowSize, double factor) =>
    Size(windowSize.width / factor, windowSize.height / factor);

/// Where the window should go for a lower-region mode, and whether the user
/// may drag it elsewhere.
class WindowModeTarget {
  const WindowModeTarget({
    required this.size,
    required this.minimumSize,
    required this.resizable,
  });

  final Size size;
  final Size minimumSize;
  final bool resizable;
}

/// The window target for [lowerRegion] at [factor]: equalizer mode snaps to
/// the fixed stack and forbids edge resize; playlist mode restores the stored
/// logical size (or the default) and allows free resize down to the mode
/// minimum.
WindowModeTarget windowModeTarget({
  required LowerRegion lowerRegion,
  required double factor,
  double? storedPlaylistWidth,
  double? storedPlaylistHeight,
  bool equalizerCollapsed = false,
}) {
  if (lowerRegion == LowerRegion.equalizer) {
    final size = eqModeWindowSize(factor, collapsed: equalizerCollapsed);
    return WindowModeTarget(
      size: size,
      minimumSize: size,
      resizable: false,
    );
  }
  return WindowModeTarget(
    size: playlistModeWindowSize(
      factor: factor,
      storedWidth: storedPlaylistWidth,
      storedHeight: storedPlaylistHeight,
    ),
    minimumSize: playlistModeMinimumSize(factor),
    resizable: true,
  );
}

/// Logical geometry of the scaled panel stack for a given window content
/// area (window size minus the frame).
class PanelStackLayout {
  const PanelStackLayout({
    required this.logicalWidth,
    required this.lowerHeight,
  });

  /// Logical width of the stack: the main player width, or wider when the
  /// playlist fills a freely resized window.
  final double logicalWidth;

  /// Logical height of the region below the gutter.
  final double lowerHeight;

  /// Logical height of the whole stack.
  double get logicalHeight =>
      TrampMetrics.mainPlayer.height + TrampMetrics.gutter + lowerHeight;

  /// On-screen size of the stack host at [factor].
  Size hostSize(double factor) =>
      Size(logicalWidth * factor, logicalHeight * factor);
}

/// Computes the stack layout for [contentSize] in scaled pixels.
///
/// Equalizer mode is exact — the fixed canvas sizes, ignoring any extra space
/// (the window is snapped to the stack, so there should be none). Playlist
/// mode fills: the stack widens and the well grows to consume the content
/// area, never shrinking below the fixed canvases.
PanelStackLayout panelStackLayout({
  required LowerRegion lowerRegion,
  required double factor,
  required Size contentSize,
  bool equalizerCollapsed = false,
}) {
  if (lowerRegion == LowerRegion.equalizer) {
    return PanelStackLayout(
      logicalWidth: TrampMetrics.mainPlayer.width,
      lowerHeight: equalizerCollapsed
          ? TrampMetrics.titleBar
          : TrampMetrics.equalizer.height,
    );
  }
  final logicalWidth = math.max(
    TrampMetrics.mainPlayer.width,
    contentSize.width / factor,
  );
  final lowerScaled = math.max(
    TrampMetrics.minLowerRegion * factor,
    contentSize.height -
        (TrampMetrics.mainPlayer.height + TrampMetrics.gutter) * factor,
  );
  return PanelStackLayout(
    logicalWidth: logicalWidth,
    lowerHeight: lowerScaled / factor,
  );
}
