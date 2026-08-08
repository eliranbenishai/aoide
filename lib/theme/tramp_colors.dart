import 'package:flutter/painting.dart';

import 'mockup_tokens.dart';

/// Chrome color names used by existing widgets.
///
/// Values are sourced from [MockupTokens] so there is a single palette.
/// Legacy graphite field names remain as a facade until chrome is rebuilt.
abstract final class TrampColors {
  /// Outer border and the gutter between stacked panels.
  static const frame = Color(0xFF000000);

  static const panelBottom = MockupTokens.shellMid;
  static const bevelHi = MockupTokens.inkDim;
  static const bevelLo = MockupTokens.shellDeep;

  static const buttonTop = MockupTokens.shellHi;
  static const buttonBottom = MockupTokens.shellLo;

  static const wellDeep = MockupTokens.well;
  static const lcdGlass = MockupTokens.shellDeep;

  /// Lit phosphor: LCD text, spectrum bars, slider fills, active toggles.
  static const phosphor = MockupTokens.phos;
  static const phosphorDim = MockupTokens.phosDim;

  /// Magenta accent (title-bar grips, hot UI accents).
  static const railAccent = MockupTokens.accent;

  static const label = MockupTokens.ink;
  static const labelDim = MockupTokens.inkDim;

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
