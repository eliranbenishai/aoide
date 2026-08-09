import 'package:flutter/painting.dart';

import '../look/look_palette.dart';
import '../look/resolved_look.dart';
import 'mockup_tokens.dart';

/// Chrome color names used by existing widgets.
///
/// Prefer [TrampColors.of] with a [ResolvedLook] from [LookScope]. Static
/// consts remain as a builtin facade for theme bootstrap and token tests.
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

  /// Map legacy chrome names onto a resolved look palette.
  static TrampColorSet of(ResolvedLook look) => TrampColorSet.fromPalette(
        look.palette,
      );
}

/// Instance colors derived from a [LookPalette] (legacy [TrampColors] names).
class TrampColorSet {
  const TrampColorSet({
    required this.panelBottom,
    required this.bevelHi,
    required this.bevelLo,
    required this.buttonTop,
    required this.buttonBottom,
    required this.wellDeep,
    required this.lcdGlass,
    required this.phosphor,
    required this.phosphorDim,
    required this.railAccent,
    required this.label,
    required this.labelDim,
  });

  factory TrampColorSet.fromPalette(LookPalette palette) => TrampColorSet(
        panelBottom: palette.shellMid,
        bevelHi: palette.inkDim,
        bevelLo: palette.shellDeep,
        buttonTop: palette.shellHighlight,
        buttonBottom: palette.shellLow,
        wellDeep: palette.well,
        lcdGlass: palette.shellDeep,
        phosphor: palette.phosphorDefault,
        phosphorDim: palette.phosphorDim,
        railAccent: palette.accentDefault,
        label: palette.inkDefault,
        labelDim: palette.inkDim,
      );

  final Color panelBottom;
  final Color bevelHi;
  final Color bevelLo;
  final Color buttonTop;
  final Color buttonBottom;
  final Color wellDeep;
  final Color lcdGlass;
  final Color phosphor;
  final Color phosphorDim;
  final Color railAccent;
  final Color label;
  final Color labelDim;
}
