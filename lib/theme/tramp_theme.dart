import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

import 'tramp_colors.dart';

ThemeData buildTrampTheme() {
  final base = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    scaffoldBackgroundColor: TrampColors.metalMid,
    colorScheme: const ColorScheme.dark(
      primary: TrampColors.lcdPhosphor,
      secondary: TrampColors.fillAccent,
      surface: TrampColors.metalFace,
      onPrimary: TrampColors.lcdBackground,
      onSurface: TrampColors.metalDeep,
    ),
  );
  return base.copyWith(
    textTheme: GoogleFonts.ibmPlexMonoTextTheme(base.textTheme).apply(
      bodyColor: TrampColors.metalDeep,
      displayColor: TrampColors.metalDeep,
    ),
  );
}
