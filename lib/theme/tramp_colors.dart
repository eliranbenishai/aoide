import 'package:flutter/material.dart';

abstract final class TrampColors {
  static const metalHi = Color(0xFFE4E4E4);
  static const metalFace = Color(0xFFB8B8B8);
  static const metalMid = Color(0xFF9A9A9A);
  static const metalShadow = Color(0xFF6E6E6E);
  static const metalDeep = Color(0xFF4A4A4A);
  static const groove = Color(0xFF3A3A3A);

  static const lcdBackground = Color(0xFF0A1A0A);
  static const lcdPhosphor = Color(0xFF33FF33);
  static const lcdPhosphorDim = Color(0xFF1A8A1A);
  static const lcdPeak = Color(0xFFCCFF33);

  static const fillAccent = Color(0xFF2ECC40);
  static const windowClose = Color(0xFFC44C4C);

  static const skinBorder = Color(0xFF555555);
  static const borderWidth = 1.0;

  // Compatibility aliases while migrating call sites in later tasks:
  static const surface = metalFace;
  static const ink = metalDeep;
  static const accent = fillAccent;
  static const muted = metalShadow;
  static const transportWash = metalHi;
  static const playlistTop = metalHi;
  static const playlistBottom = metalFace;
  static const brandAccent = lcdPeak;
  static const minimize = metalShadow;
}
