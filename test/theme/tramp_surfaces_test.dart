import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/theme/tramp_surfaces.dart';

void main() {
  test('raised panel is one smooth two-stop gradient', () {
    final d = TrampSurfaces.raisedPanel();
    final g = d.gradient as LinearGradient;
    // Exactly two stops: the mockup's visible mid-gradient stop is a rendering
    // artifact, not a design feature.
    expect(g.colors, [TrampColors.panelTop, TrampColors.panelBottom]);
    expect(g.stops, isNull);
    expect(g.begin, Alignment.topCenter);
    expect(g.end, Alignment.bottomCenter);
  });

  test('raised panel bevels highlight the top and shadow the bottom', () {
    final border = TrampSurfaces.raisedPanel().border! as Border;
    expect(border.top.color, TrampColors.bevelHi);
    expect(border.bottom.color, TrampColors.bevelLo);
  });

  test('pressed button inverts the raised button gradient', () {
    final raised = TrampSurfaces.raisedButton().gradient! as LinearGradient;
    final pressed = TrampSurfaces.pressedButton().gradient! as LinearGradient;
    expect(pressed.colors, raised.colors.reversed.toList());
  });

  test('inset well reverses the bevel direction', () {
    final border = TrampSurfaces.insetWell().border! as Border;
    expect(border.top.color, TrampColors.bevelLo);
    expect(border.bottom.color, TrampColors.bevelHi);
  });

  test('lcd glass fills with glass colour and has no gradient', () {
    final d = TrampSurfaces.lcdGlass();
    expect(d.color, TrampColors.lcdGlass);
    expect(d.gradient, isNull);
  });

  test('bevel width is configurable for device-pixel snapping', () {
    final border = TrampSurfaces.raisedButton(bevel: 2).border! as Border;
    expect(border.top.width, 2);
  });
}
