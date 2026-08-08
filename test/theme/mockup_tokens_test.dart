import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('palette matches the mockup design table', () {
    expect(MockupTokens.shellHi, const Color(0xFF323744));
    expect(MockupTokens.shell, const Color(0xFF262B38));
    expect(MockupTokens.shellMid, const Color(0xFF1A1D26));
    expect(MockupTokens.shellLo, const Color(0xFF12141A));
    expect(MockupTokens.shellDeep, const Color(0xFF0A0B0E));

    expect(MockupTokens.ink, const Color(0xFFE8EAF0));
    expect(MockupTokens.inkDim, const Color(0xFF8B919E));
    expect(MockupTokens.inkFaint, const Color(0xFF5B6270));

    expect(MockupTokens.phos, const Color(0xFF3DE7FF));
    expect(MockupTokens.phosHot, const Color(0xFFB8F6FF));
    expect(MockupTokens.phosDim, const Color(0xFF1A7A88));
    expect(MockupTokens.phosDeep, const Color(0xFF0D3D46));

    expect(MockupTokens.accent, const Color(0xFFFF3D9A));
    expect(MockupTokens.accentDim, const Color(0xFF8A2258));

    expect(MockupTokens.well, const Color(0xFF050608));
  });

  test('TrampColors phosphor/accent facade the mockup tokens', () {
    expect(TrampColors.phosphor, MockupTokens.phos);
    expect(TrampColors.phosphorDim, MockupTokens.phosDim);
    expect(TrampColors.railAccent, MockupTokens.accent);
    expect(TrampColors.label, MockupTokens.ink);
  });

  test('every mockup token is fully opaque', () {
    for (final c in MockupTokens.all) {
      expect(c.a, 1.0, reason: '$c must be opaque');
    }
  });
}
