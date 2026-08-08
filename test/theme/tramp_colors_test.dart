import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('chrome surfaces are dark mockup shell, not light metal', () {
    expect(TrampColors.panelBottom, MockupTokens.shellMid);
    expect(TrampColors.buttonTop, MockupTokens.shellHi);
    expect(TrampColors.buttonTop.computeLuminance(), lessThan(0.05));
  });

  test('phosphor is cyan, not chartreuse', () {
    expect(TrampColors.phosphor, MockupTokens.phos);
    // Cyan phosphor is strongly blue-biased against red.
    expect(TrampColors.phosphor.b, greaterThan(TrampColors.phosphor.r));
  });

  test('rail accent is magenta', () {
    expect(TrampColors.railAccent, MockupTokens.accent);
    expect(TrampColors.railAccent.r, greaterThan(TrampColors.phosphor.r));
  });

  test('frame is pure black and the well is near-black', () {
    expect(TrampColors.frame, const Color(0xFF000000));
    expect(TrampColors.wellDeep, MockupTokens.well);
    expect(TrampColors.lcdGlass, MockupTokens.shellDeep);
  });

  test('every token is fully opaque', () {
    for (final c in TrampColors.all) {
      expect(c.a, 1.0, reason: '$c must be opaque');
    }
  });
}
