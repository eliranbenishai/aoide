import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('paper/ink tokens match locked prototype W', () {
    expect(TrampColors.surface, const Color(0xFFF2EBE0));
    expect(TrampColors.ink, const Color(0xFF1A1410));
    expect(TrampColors.accent, const Color(0xFFC43C17));
    expect(TrampColors.muted, const Color(0xFF6A5E52));
    expect(TrampColors.transportWash, const Color(0xFFE8DCC8));
  });
}
