import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_palette.dart';
import 'package:tramp/theme/look_paint.dart';
import 'package:tramp/theme/mockup_tokens.dart';

void main() {
  final builtin = BuiltinLook.resolved.palette;

  test('builtin title-bar stops match mockup literals', () {
    expect(LookPaint.titleBarStops(builtin), const [
      Color(0xFF3C4356),
      Color(0xFF2C3241),
      Color(0xFF1D222C),
      Color(0xFF12151C),
    ]);
  });

  test('builtin win-btn faces match mockup literals', () {
    expect(LookPaint.winBtnIdle(builtin), const [
      Color(0xFF454D60),
      Color(0xFF2F3543),
      Color(0xFF20242E),
    ]);
    expect(LookPaint.winBtnPressed(builtin), const [
      Color(0xFF2F3543),
      Color(0xFF20242E),
      Color(0xFF161920),
    ]);
    expect(LookPaint.winBtnCloseIdle(builtin), const [
      Color(0xFF9C2A60),
      Color(0xFF79204A),
      Color(0xFF4A1129),
    ]);
    expect(LookPaint.winBtnClosePressed(builtin), const [
      Color(0xFF79204A),
      Color(0xFF4A1129),
      Color(0xFF2E0A1A),
    ]);
  });

  test('builtin button faces match mockup literals', () {
    expect(LookPaint.buttonIdle(builtin), const [
      Color(0xFF3F4657),
      Color(0xFF2B313E),
      Color(0xFF1E222C),
    ]);
    expect(LookPaint.buttonPressed(builtin), const [
      Color(0xFF2B313E),
      Color(0xFF1E222C),
      Color(0xFF161A22),
    ]);
  });

  test('lit button mid stop is palette phosphor', () {
    final alt = _palette(
      phosphorDefault: const Color(0xFFFFB000),
      phosphorHot: const Color(0xFFFFE08A),
      phosphorDim: const Color(0xFF8A6200),
      phosphorDeep: const Color(0xFF3D2A00),
    );
    expect(LookPaint.buttonOn(alt)[1], const Color(0xFFFFB000));
    expect(
      LookPaint.buttonOnInk(alt),
      LookPaint.buttonOnInk(
        _palette(phosphorDeep: const Color(0xFF3D2A00)),
      ),
    );
    expect(LookPaint.buttonOnInk(alt), isNot(const Color(0xFF04222B)));
    expect(_srgb8(LookPaint.phosphorBloom(alt).r), 0xFF);
    expect(_srgb8(LookPaint.phosphorBloom(alt).g), 0xB0);
  });

  test('accent blooms follow accentDefault', () {
    final alt = _palette(accentDefault: const Color(0xFFFF5A1F));
    expect(_srgb8(LookPaint.accentBloom(alt).r), 0xFF);
    expect(_srgb8(LookPaint.accentBloom(alt).g), 0x5A);
    expect(_srgb8(LookPaint.accentBloom(alt).b), 0x1F);
  });

  test('builtin lit mid is mockup phosphor token', () {
    expect(LookPaint.buttonOn(builtin)[1], MockupTokens.phos);
  });

  test('builtin lit button extras match mockup literals', () {
    expect(LookPaint.buttonOn(builtin)[0], const Color(0xFFA9F4FF));
    expect(LookPaint.buttonOn(builtin)[2], const Color(0xFF128FA8));
    expect(LookPaint.buttonOnInk(builtin), const Color(0xFF04222B));
    expect(LookPaint.buttonOnFoot(builtin), const Color(0xFF054658));
    expect(LookPaint.hoverLiftTarget(builtin), const Color(0xFFE8F0FF));
    expect(LookPaint.screenWash(builtin), const [
      Color(0xFF0F1C2A),
      Color(0xFF071018),
      Color(0xFF04070C),
    ]);
    expect(LookPaint.sliderFillHi(builtin), const Color(0xFFCBF9FF));
    expect(LookPaint.sliderFillLo(builtin), const Color(0xFF0F7F96));
    expect(LookPaint.closeGlyphInk(builtin), const Color(0xFFFFD6E8));
    expect(LookPaint.accentHot(builtin), const Color(0xFFFFD6EA));
    expect(LookPaint.plateFace(builtin), const Color(0xFF1E222C));
    expect(LookPaint.coolSheen(builtin), const Color(0xFFE2ECFF));
  });

  test('plate face follows shell mid under alternate look', () {
    final alt = _palette(shellMid: const Color(0xFF1C1812));
    expect(LookPaint.plateFace(alt), const Color(0xFF201D18));
  });
}

int _srgb8(double channel) => (channel * 255.0).round().clamp(0, 255);

LookPalette _palette({
  Color? shellMid,
  Color? phosphorDefault,
  Color? phosphorHot,
  Color? phosphorDim,
  Color? phosphorDeep,
  Color? accentDefault,
}) {
  final b = BuiltinLook.resolved.palette;
  return LookPalette(
    shellHighlight: b.shellHighlight,
    shellBase: b.shellBase,
    shellMid: shellMid ?? b.shellMid,
    shellLow: b.shellLow,
    shellDeep: b.shellDeep,
    inkDefault: b.inkDefault,
    inkDim: b.inkDim,
    inkFaint: b.inkFaint,
    phosphorDefault: phosphorDefault ?? b.phosphorDefault,
    phosphorHot: phosphorHot ?? b.phosphorHot,
    phosphorDim: phosphorDim ?? b.phosphorDim,
    phosphorDeep: phosphorDeep ?? b.phosphorDeep,
    accentDefault: accentDefault ?? b.accentDefault,
    accentDim: b.accentDim,
    well: b.well,
  );
}
