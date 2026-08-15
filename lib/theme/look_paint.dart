import 'package:flutter/painting.dart';

import '../look/look_palette.dart';

/// Derives mockup chrome paints from a [LookPalette].
///
/// Channel lifts / scales are calibrated so [BuiltinLook] / [MockupTokens]
/// reproduce the hardcoded mockup literals (title-bar face, `.btn`, `.wbtn`)
/// bit-for-bit.
abstract final class LookPaint {
  /// Title-bar vertical gradient (`.tbar`).
  static List<Color> titleBarStops(LookPalette p) => [
        _lift(p.shellHighlight, 10, 12, 18),
        _lift(p.shellBase, 6, 7, 9),
        _lift(p.shellMid, 3, 5, 6),
        _lift(p.shellLow, 0, 1, 2),
      ];

  /// Idle window-button face (`.wbtn`).
  static List<Color> winBtnIdle(LookPalette p) => [
        _lift(p.shellHighlight, 19, 22, 28),
        _lift(p.shellBase, 9, 10, 11),
        _lift(p.shellMid, 6, 7, 8),
      ];

  /// Pressed window-button face.
  static List<Color> winBtnPressed(LookPalette p) => [
        _lift(p.shellBase, 9, 10, 11),
        _lift(p.shellMid, 6, 7, 8),
        _lift(p.shellLow, 4, 5, 6),
      ];

  /// Idle close-button face — per-channel shade of accent (mockup `.wbtn.close`).
  static List<Color> winBtnCloseIdle(LookPalette p) => [
        _scale(p.accentDefault, 0.6118, 0.6885, 0.6234),
        _scale(p.accentDefault, 0.4745, 0.5246, 0.4805),
        _scale(p.accentDefault, 0.2902, 0.2787, 0.2662),
      ];

  /// Pressed close-button face.
  static List<Color> winBtnClosePressed(LookPalette p) => [
        _scale(p.accentDefault, 0.4745, 0.5246, 0.4805),
        _scale(p.accentDefault, 0.2902, 0.2787, 0.2662),
        _scale(p.accentDefault, 0.1804, 0.1639, 0.1688),
      ];

  /// Idle raised control (`.btn`).
  static List<Color> buttonIdle(LookPalette p) => [
        _lift(p.shellHighlight, 13, 15, 19),
        _lift(p.shellBase, 5, 6, 6),
        _lift(p.shellMid, 4, 5, 6),
      ];

  /// Pressed raised control.
  static List<Color> buttonPressed(LookPalette p) => [
        _lift(p.shellBase, 5, 6, 6),
        _lift(p.shellMid, 4, 5, 6),
        _lift(p.shellLow, 4, 6, 8),
      ];

  /// Lit control fill (`.btn--on`).
  static List<Color> buttonOn(LookPalette p) => [
        _lift(p.phosphorHot, -15, -2, 0),
        p.phosphorDefault,
        _lift(p.phosphorDim, -8, 21, 32),
      ];

  /// Deep ink on lit label/icon faces (`#04222B` from builtin phosphorDeep).
  static Color buttonOnInk(LookPalette p) =>
      _scale(p.phosphorDeep, 0.3077, 0.5574, 0.6143);

  /// Bottom inset on lit buttons (`#054658` from builtin phosphorDeep).
  static Color buttonOnFoot(LookPalette p) => _lift(p.phosphorDeep, -8, 9, 18);

  /// Top lip on lit buttons (`#F0FDFF` from builtin phosphorHot).
  static Color buttonOnLip(LookPalette p) => _lift(p.phosphorHot, 56, 7, 0);

  /// Soft outer bloom behind lit buttons / grip rails.
  static Color phosphorBloom(LookPalette p, [int alpha = 0x4D]) =>
      p.phosphorDefault.withValues(alpha: alpha / 255);

  /// Magenta under-rail / LED outer glow.
  static Color accentBloom(LookPalette p, [int alpha = 0x59]) =>
      p.accentDefault.withValues(alpha: alpha / 255);

  /// Lit LED hot center (`#FFD6EA` on builtin).
  static Color accentHot(LookPalette p) => _tintTowardWhite(
        p.accentDefault,
        greenPull: 41 / 194,
        bluePull: 21 / 101,
      );

  /// Title wordmark fill (near ink, slightly cooler in the mockup).
  static Color wordmark(LookPalette p) => _lift(p.inkDefault, 2, 8, 15);

  /// Role title in the title strip (`#C8D6EB` @ 55% on builtin).
  static Color windowName(LookPalette p) =>
      _lift(p.inkDefault, -32, -20, -5).withValues(alpha: 0x8C / 255);

  /// Default glyph / icon ink (`.wbtn` / transport).
  static Color glyphInk(LookPalette p, [int alpha = 0xD1]) =>
      _lift(p.inkDefault, -18, -8, 5).withValues(alpha: alpha / 255);

  /// Close glyph tint (`#FFD6E8` on builtin accent).
  static Color closeGlyphInk(LookPalette p) => _tintTowardWhite(
        p.accentDefault,
        greenPull: 41 / 194,
        bluePull: 23 / 101,
      );

  /// Off-state button label (`#C4D2E8` @ 72% on builtin).
  static Color buttonLabelIdle(LookPalette p) =>
      _lift(p.inkDefault, -36, -24, -8).withValues(alpha: 0xB8 / 255);

  /// Hover lift target (steel sheen `#E8F0FF` on builtin ink).
  static Color hoverLiftTarget(LookPalette p) => _lift(p.inkDefault, 0, 6, 15);

  /// Cool chrome sheen (`#E2ECFF` on builtin) for lips / screen rims.
  static Color coolSheen(LookPalette p) => _lift(p.inkDefault, -6, 2, 15);

  /// Brushed plate / rail face (`.plate` / `.rail` — `#1E222C` on builtin).
  static Color plateFace(LookPalette p) => _lift(p.shellMid, 4, 5, 6);

  /// Unlit LED face highlights (mockup `.led`).
  static Color idleLedHi(LookPalette p) => _lift(p.shellBase, 23, 24, 24);
  static Color idleLedLo(LookPalette p) => _lift(p.shellMid, 8, 9, 9);

  /// Lit LED rim stroke (`#5A0F32` from builtin accentDim).
  static Color litLedRim(LookPalette p) => _lift(p.accentDim, -56, -19, -38);

  /// Display-well radial wash (`.screen`).
  static List<Color> screenWash(LookPalette p) => [
        _lift(p.well, 10, 22, 34),
        _lift(p.well, 2, 10, 16),
        _lift(p.well, -1, 1, 4),
      ];

  /// Slider fill top stop (`#CBF9FF` from builtin phosphorHot).
  static Color sliderFillHi(LookPalette p) => _lift(p.phosphorHot, 19, 3, 0);

  /// Slider fill bottom stop (`#0F7F96` from builtin phosphorDim).
  static Color sliderFillLo(LookPalette p) => _lift(p.phosphorDim, -11, 5, 14);

  static int _srgb8(double channel) =>
      (channel * 255.0).round().clamp(0, 255);

  static Color _lift(Color base, int dr, int dg, int db) {
    return Color.fromARGB(
      _srgb8(base.a),
      (_srgb8(base.r) + dr).clamp(0, 255),
      (_srgb8(base.g) + dg).clamp(0, 255),
      (_srgb8(base.b) + db).clamp(0, 255),
    );
  }

  static Color _scale(
    Color accent,
    double rScale,
    double gScale,
    double bScale,
  ) {
    return Color.fromARGB(
      255,
      (_srgb8(accent.r) * rScale).round().clamp(0, 255),
      (_srgb8(accent.g) * gScale).round().clamp(0, 255),
      (_srgb8(accent.b) * bScale).round().clamp(0, 255),
    );
  }

  /// Tint [color] toward white with independent G/B pull factors (R stays max).
  static Color _tintTowardWhite(
    Color color, {
    required double greenPull,
    required double bluePull,
  }) {
    return Color.fromARGB(
      255,
      255,
      (255 - (255 - _srgb8(color.g)) * greenPull).round().clamp(0, 255),
      (255 - (255 - _srgb8(color.b)) * bluePull).round().clamp(0, 255),
    );
  }
}
