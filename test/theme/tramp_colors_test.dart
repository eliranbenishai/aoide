import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('panel face is dark graphite, not light metal', () {
    // The old palette was #B8B8B8. Anything bright here means the light-metal
    // chrome came back.
    expect(TrampColors.panelTop, const Color(0xFF2C3039));
    expect(TrampColors.panelBottom, const Color(0xFF1D2128));
    expect(TrampColors.panelTop.computeLuminance(), lessThan(0.05));
  });

  test('phosphor is chartreuse, not pure green', () {
    expect(TrampColors.phosphor, const Color(0xFFCFEA45));
    // Pure green (#33FF33) has a blue channel equal to its red channel; the
    // chartreuse phosphor is strongly red-biased against blue.
    expect(TrampColors.phosphor.red, greaterThan(TrampColors.phosphor.blue));
  });

  test('rail accent is warmer than the phosphor', () {
    expect(TrampColors.railAccent, const Color(0xFFFEE670));
    expect(TrampColors.railAccent.red, greaterThan(TrampColors.phosphor.red));
  });

  test('frame is pure black and the well is near-black', () {
    expect(TrampColors.frame, const Color(0xFF000000));
    expect(TrampColors.wellDeep, const Color(0xFF010306));
    expect(TrampColors.lcdGlass, const Color(0xFF03060A));
  });

  test('every token is fully opaque', () {
    for (final c in TrampColors.all) {
      expect(c.alpha, 0xFF, reason: '$c must be opaque');
    }
  });
}
