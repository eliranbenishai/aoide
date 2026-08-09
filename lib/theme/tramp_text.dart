import 'package:flutter/painting.dart';

import '../look/resolved_look.dart';

/// Bundled type. Both families ship as assets: runtime font fetching breaks
/// offline rendering and makes golden tests non-deterministic.
///
/// Styles take a [ResolvedLook] so chrome/LCD family names come from the
/// active look pack (builtin → TrampCondensed / TrampMono).
abstract final class TrampText {
  /// Chrome button labels: the playlist toolbar's LOAD, SAVE, ADD.
  static TextStyle chromeLabel(ResolvedLook look) => TextStyle(
        fontFamily: look.chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: 11,
        height: 1,
        letterSpacing: 0.6,
        color: look.palette.inkDefault,
      );

  /// Equalizer frequency and dB scale labels.
  static TextStyle eqScale(ResolvedLook look) => TextStyle(
        fontFamily: look.chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: 9,
        height: 1,
        color: look.palette.inkDefault,
      );

  /// Track title, bitrate, indicators.
  static TextStyle lcd(ResolvedLook look) => TextStyle(
        fontFamily: look.lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 11,
        height: 1.1,
        color: look.palette.phosphorDefault,
      );

  static TextStyle lcdDim(ResolvedLook look) => TextStyle(
        fontFamily: look.lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 11,
        height: 1.1,
        color: look.palette.phosphorDim,
      );

  /// The large elapsed-time readout.
  static TextStyle lcdLarge(ResolvedLook look) => TextStyle(
        fontFamily: look.lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 24,
        height: 1,
        color: look.palette.phosphorDefault,
      );
}
