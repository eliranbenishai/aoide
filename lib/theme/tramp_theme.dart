import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

import 'tramp_colors.dart';

ThemeData buildTrampTheme() {
  final base = ThemeData(
    useMaterial3: true,
    brightness: Brightness.light,
    scaffoldBackgroundColor: TrampColors.surface,
    colorScheme: const ColorScheme.light(
      primary: TrampColors.ink,
      secondary: TrampColors.accent,
      surface: TrampColors.surface,
      onPrimary: TrampColors.surface,
      onSurface: TrampColors.ink,
    ),
  );
  return base.copyWith(
    textTheme: GoogleFonts.ibmPlexMonoTextTheme(base.textTheme).apply(
      bodyColor: TrampColors.ink,
      displayColor: TrampColors.ink,
    ),
    primaryTextTheme: GoogleFonts.syneTextTheme(base.primaryTextTheme),
  );
}
