import 'package:flutter/painting.dart';

/// Graphite chrome palette. Every value is sampled from the reference mockup at
/// `docs/mockups/graphite-chrome.png`.
abstract final class TrampColors {
  /// Outer border and the gutter between stacked panels.
  static const frame = Color(0xFF000000);

  static const panelBottom = Color(0xFF1D2128);
  static const bevelHi = Color(0xFF555B65);
  static const bevelLo = Color(0xFF0B0E12);

  static const buttonTop = Color(0xFF363B45);
  static const buttonBottom = Color(0xFF22262E);

  static const wellDeep = Color(0xFF010306);
  static const lcdGlass = Color(0xFF03060A);

  /// Lit phosphor: LCD text, spectrum bars, slider fills, active toggles.
  static const phosphor = Color(0xFFCFEA45);
  static const phosphorDim = Color(0xFF5C7022);

  /// Chrome accent — deliberately warmer than [phosphor] so the display reads
  /// as a screen rather than as paint.
  static const railAccent = Color(0xFFFEE670);

  static const label = Color(0xFFC9CED3);
  static const labelDim = Color(0xFF979DA6);

  static const all = <Color>[
    frame,
    panelBottom,
    bevelHi,
    bevelLo,
    buttonTop,
    buttonBottom,
    wellDeep,
    lcdGlass,
    phosphor,
    phosphorDim,
    railAccent,
    label,
    labelDim,
  ];
}
