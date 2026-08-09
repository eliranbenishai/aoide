import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_title_bar.dart';
import 'package:tramp/ui/docking/dock_drag_area.dart';

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
}
