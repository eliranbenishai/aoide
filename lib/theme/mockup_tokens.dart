import 'package:flutter/painting.dart';

/// Mockup palette from `player-mockup-2.html` `:root` / design palette table.
///
/// Flutter constants mirror the CSS tokens — do not invent a parallel theme.
abstract final class MockupTokens {
  static const shellHi = Color(0xFF323744);
  static const shell = Color(0xFF262B38);
  static const shellMid = Color(0xFF1A1D26);
  static const shellLo = Color(0xFF12141A);
  static const shellDeep = Color(0xFF0A0B0E);

  static const ink = Color(0xFFE8EAF0);
  static const inkDim = Color(0xFF8B919E);
  static const inkFaint = Color(0xFF5B6270);

  static const phos = Color(0xFF3DE7FF);
  static const phosHot = Color(0xFFB8F6FF);
  static const phosDim = Color(0xFF1A7A88);
  static const phosDeep = Color(0xFF0D3D46);

  static const accent = Color(0xFFFF3D9A);
  static const accentDim = Color(0xFF8A2258);

  static const well = Color(0xFF050608);

  static const all = <Color>[
    shellHi,
    shell,
    shellMid,
    shellLo,
    shellDeep,
    ink,
    inkDim,
    inkFaint,
    phos,
    phosHot,
    phosDim,
    phosDeep,
    accent,
    accentDim,
    well,
  ];
}
