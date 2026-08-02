import 'package:flutter/painting.dart';

import 'tramp_colors.dart';

/// Bundled type. Both families ship as assets: runtime font fetching breaks
/// offline rendering and makes golden tests non-deterministic.
abstract final class TrampText {
  static const _chrome = 'BarlowSemiCondensed';
  static const _mono = 'IBMPlexMono';

  /// Chrome button labels: OPEN, ZOOM, PRESETS, ON, AUTO, EQ, PL.
  static const chromeLabel = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 11,
    height: 1,
    letterSpacing: 0.6,
    color: TrampColors.label,
  );

  static const chromeLabelDim = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 11,
    height: 1,
    letterSpacing: 0.6,
    color: TrampColors.labelDim,
  );

  /// The TRAMP wordmark between the title-bar rails.
  static const wordmark = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w700,
    fontSize: 15,
    height: 1,
    letterSpacing: 3.2,
    color: TrampColors.label,
  );

  /// Equalizer frequency and dB scale labels.
  static const eqScale = TextStyle(
    fontFamily: _chrome,
    fontWeight: FontWeight.w600,
    fontSize: 9,
    height: 1,
    color: TrampColors.label,
  );

  /// Track title, bitrate, indicators.
  static const lcd = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w500,
    fontSize: 11,
    height: 1.1,
    color: TrampColors.phosphor,
  );

  static const lcdDim = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w500,
    fontSize: 11,
    height: 1.1,
    color: TrampColors.phosphorDim,
  );

  /// The large elapsed-time readout.
  static const lcdLarge = TextStyle(
    fontFamily: _mono,
    fontWeight: FontWeight.w600,
    fontSize: 24,
    height: 1,
    color: TrampColors.phosphor,
  );
}
