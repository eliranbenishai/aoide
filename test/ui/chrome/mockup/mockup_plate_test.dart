import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_palette.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/theme/look_paint.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';

import '../../../support/look_harness.dart';

void main() {
  testWidgets('plate face follows look shell mid', (tester) async {
    final b = BuiltinLook.resolved.palette;
    final look = ResolvedLook(
      id: 'probe',
      name: 'Probe',
      palette: LookPalette(
        shellHighlight: b.shellHighlight,
        shellBase: b.shellBase,
        shellMid: const Color(0xFF1C1812),
        shellLow: b.shellLow,
        shellDeep: b.shellDeep,
        inkDefault: b.inkDefault,
        inkDim: b.inkDim,
        inkFaint: b.inkFaint,
        phosphorDefault: b.phosphorDefault,
        phosphorHot: b.phosphorHot,
        phosphorDim: b.phosphorDim,
        phosphorDeep: b.phosphorDeep,
        accentDefault: b.accentDefault,
        accentDim: b.accentDim,
        well: b.well,
      ),
      materials: BuiltinLook.resolved.materials,
      chromeFamily: 'TrampCondensed',
      lcdFamily: 'TrampMono',
    );

    await tester.pumpWidget(
      Directionality(
        textDirection: TextDirection.ltr,
        child: wrapWithLook(
          const Center(
            child: SizedBox(
              width: 80,
              height: 40,
              child: MockupPlate(),
            ),
          ),
          look: look,
        ),
      ),
    );

    final face = tester.widget<ColoredBox>(find.byType(ColoredBox).first);
    expect(face.color, LookPaint.plateFace(look.palette));
    expect(face.color, isNot(LookPaint.plateFace(BuiltinLook.resolved.palette)));
  });
}
