import 'package:flutter/material.dart';

import 'tramp_colors.dart';

ThemeData buildTrampTheme() {
  final base = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    scaffoldBackgroundColor: TrampColors.frame,
    colorScheme: const ColorScheme.dark(
      primary: TrampColors.phosphor,
      secondary: TrampColors.railAccent,
      surface: TrampColors.panelBottom,
      onPrimary: TrampColors.lcdGlass,
      onSurface: TrampColors.label,
    ),
  );

  return base.copyWith(
    textTheme: base.textTheme.apply(
      fontFamily: 'BarlowSemiCondensed',
      bodyColor: TrampColors.label,
      displayColor: TrampColors.label,
    ),
  );
}
