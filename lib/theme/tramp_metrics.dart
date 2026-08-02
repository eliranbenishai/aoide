import 'package:flutter/painting.dart';

/// Fixed logical canvases, derived by halving the reference mockup's measured
/// panel bounds (main 1625x484, equalizer 1625x411).
///
/// Every dimension inside a panel is authored against these, and the zoom
/// factor is applied once at the root of the stack. There is deliberately no
/// per-widget scaling.
abstract final class TrampMetrics {
  static const mainPlayer = Size(812, 242);
  static const equalizer = Size(812, 206);

  /// Black gutter between stacked panels.
  static const gutter = 6.0;

  /// Black frame around the whole window.
  static const frame = 6.0;

  /// Default height given to the lower region at 100%.
  static const minLowerRegion = 240.0;

  /// Default playlist well height at 100% when no persisted size exists.
  static const defaultPlaylistWellHeight = 400.0;

  /// Height of a panel title bar, and therefore of a collapsed equalizer.
  static const titleBar = 35.0;
}
