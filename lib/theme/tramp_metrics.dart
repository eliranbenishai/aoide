import 'package:flutter/painting.dart';

/// Fixed logical canvases at the mockup's classic×3 grid (825×…).
///
/// Every dimension inside a panel is authored against these, and the zoom
/// factor is applied once at the root of the stack. There is deliberately no
/// per-widget scaling.
abstract final class TrampMetrics {
  static const mainPlayer = Size(825, 348);
  static const equalizer = Size(825, 348);
  static const playlistDefault = Size(825, 696);

  /// Black gutter between stacked panels (legacy single-window chrome).
  static const gutter = 6.0;

  /// Black frame around the whole window (legacy single-window chrome).
  static const frame = 6.0;

  /// Default height given to the lower region at 100%.
  static const minLowerRegion = 240.0;

  /// Default playlist well height at 100% when no persisted size exists.
  static const defaultPlaylistWellHeight = 400.0;

  /// Height of a panel title bar, and therefore of a collapsed equalizer.
  static const titleBar = 42.0;
}
