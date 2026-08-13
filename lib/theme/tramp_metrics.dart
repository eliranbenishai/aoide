import 'package:flutter/painting.dart';

/// Fixed logical canvases at the mockup's classic×3 grid (825×…).
///
/// Every dimension inside a panel is authored against these, and the zoom
/// factor is applied once at the root of the stack. There is deliberately no
/// per-widget scaling.
abstract final class TrampMetrics {
  static const mainPlayer = Size(825, 348);
  static const equalizer = Size(825, 348);
  /// Wider than the mockup's 825 because the Playlist Manager sets the
  /// collection panel *beside* the track list rather than inside its budget:
  /// 240 panel + 8 divider + 825 leaves the right panel at the width the
  /// mockup footer was authored against.
  static const playlistDefault = Size(1073, 696);
  static const settings = Size(520, 420);
  static const about = Size(480, 360);

  /// Pixel size of a logical canvas at a discrete zoom step.
  static Size zoomed(Size logical, int zoomPercent) {
    final z = zoomPercent / 100.0;
    return Size(logical.width * z, logical.height * z);
  }

  /// Rounded 75% seed used as the native unmapped default (GTK/Win32/Cocoa).
  ///
  /// About and settings are smaller than the EQ/main seed (619×261). Using
  /// that seed for them leaves a black FlView rectangle around the chrome.
  static Size nativeUnmappedSeed(Size logical) {
    final pixel = zoomed(logical, 75);
    return Size(pixel.width.roundToDouble(), pixel.height.roundToDouble());
  }

  /// Narrowest playlist window that still fits the footer chrome (buttons +
  /// TOTAL) without horizontal overflow. Spacing may collapse; controls do not.
  static const playlistMin = Size(640, 280);

  /// Narrowest the listener may drag the playlist collection panel.
  static const playlistCollectionMinWidth = 180.0;

  /// The draggable divider between the collection panel and the track list.
  static const playlistDividerWidth = 8.0;

  /// Narrowest playlist window while the collection panel is shown.
  ///
  /// Derived, not authored: [playlistMin] is the narrowest the *footer chrome*
  /// fits, and the footer now lives in the right panel. The collection panel
  /// and the divider are therefore added on top of that budget rather than
  /// sharing it — squeezing the track list below [playlistMin] would overflow
  /// the footer buttons.
  static final playlistMinWithCollection = Size(
    playlistMin.width + playlistDividerWidth + playlistCollectionMinWidth,
    playlistMin.height,
  );

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
