import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('classic chrome tokens are metal + LCD (not paper/ink)', () {
    expect(TrampColors.metalFace, isNot(const Color(0xFFF2EBE0)));
    expect(TrampColors.lcdBackground.value & 0x00FF00, greaterThan(0));
    expect(TrampColors.lcdPhosphor.green, greaterThan(TrampColors.lcdPhosphor.red));
    expect(TrampColors.metalMid, isNot(TrampColors.metalFace));
    expect(TrampColors.metalShadow, isNot(TrampColors.metalFace));
  });
}
