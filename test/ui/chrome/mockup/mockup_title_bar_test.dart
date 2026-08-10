import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_palette.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/theme/look_paint.dart';
import 'package:tramp/ui/chrome/mockup/mockup_title_bar.dart';
import 'package:tramp/ui/docking/dock_drag_area.dart';
import '../../../support/look_harness.dart';

void main() {
  testWidgets('window buttons fire when a drag region wraps the title strip',
      (tester) async {
    var minimize = 0;
    var zoomOut = 0;
    var zoomIn = 0;
    var close = 0;
    var nativeStarts = 0;

    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) => wrapWithLook(child ?? const SizedBox.shrink()),
        home: Scaffold(
          body: MockupTitleBar(
            windowName: 'Main Player',
            onMinimize: () => minimize++,
            onZoomOut: () => zoomOut++,
            onZoomIn: () => zoomIn++,
            onClose: () => close++,
            wrapDragRegion: (region) => DockDragArea(
              zoom: 1,
              logicalTopLeft: () => Offset.zero,
              onMove: (_, {required shiftUndock, required ended}) {},
              onNativeDragStarted: () => nativeStarts++,
              startDragging: () async {},
              child: region,
            ),
          ),
        ),
      ),
    );

    await tester.tap(find.bySemanticsLabel('Minimize'));
    await tester.tap(find.bySemanticsLabel('Zoom out'));
    await tester.tap(find.bySemanticsLabel('Zoom in'));
    await tester.tap(find.bySemanticsLabel('Close'));
    await tester.pump();

    expect(minimize, 1);
    expect(zoomOut, 1);
    expect(zoomIn, 1);
    expect(close, 1);
    expect(nativeStarts, 0, reason: 'button taps must not start a window drag');
  });

  testWidgets('window name color follows look ink', (tester) async {
    final b = BuiltinLook.resolved.palette;
    final look = ResolvedLook(
      id: 'probe',
      name: 'Probe',
      palette: LookPalette(
        shellHighlight: b.shellHighlight,
        shellBase: b.shellBase,
        shellMid: b.shellMid,
        shellLow: b.shellLow,
        shellDeep: b.shellDeep,
        inkDefault: const Color(0xFFFFCC99),
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
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink(), look: look),
        home: const Scaffold(
          body: MockupTitleBar(
            windowName: 'Main Player',
            showBrand: false,
            showZoom: false,
          ),
        ),
      ),
    );

    final text = tester.widget<Text>(find.text('MAIN PLAYER'));
    expect(text.style?.color, LookPaint.windowName(look.palette));
  });
}
