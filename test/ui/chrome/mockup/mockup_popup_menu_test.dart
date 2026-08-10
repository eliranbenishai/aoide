import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_popup_menu.dart';

void main() {
  testWidgets('below placement opens under the anchor', (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 600));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: Stack(
          children: [
            Positioned(
              left: 40,
              top: 40,
              child: Builder(
                builder: (buttonContext) => SizedBox(
                  key: const Key('anchor'),
                  width: 48,
                  height: 48,
                  child: GestureDetector(
                    onTap: () async {
                      final box =
                          buttonContext.findRenderObject()! as RenderBox;
                      await showMockupMenu<String>(
                        context: buttonContext,
                        anchor: box,
                        placement: MockupMenuPlacement.below,
                        items: const [
                          PopupMenuItem(value: 'a', child: Text('Alpha')),
                          PopupMenuItem(value: 'b', child: Text('Beta')),
                        ],
                      );
                    },
                    child: const ColoredBox(color: Color(0xFF888888)),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('anchor')));
    await tester.pumpAndSettle();

    final buttonBottom = tester.getRect(find.byKey(const Key('anchor'))).bottom;
    final menuTop = tester.getRect(find.text('Alpha')).top;
    expect(menuTop, greaterThanOrEqualTo(buttonBottom));
  });

  testWidgets('above placement opens over the anchor', (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 600));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        home: Stack(
          children: [
            Positioned(
              left: 40,
              bottom: 40,
              child: Builder(
                builder: (buttonContext) => SizedBox(
                  key: const Key('anchor'),
                  width: 48,
                  height: 48,
                  child: GestureDetector(
                    onTap: () async {
                      final box =
                          buttonContext.findRenderObject()! as RenderBox;
                      await showMockupMenu<String>(
                        context: buttonContext,
                        anchor: box,
                        placement: MockupMenuPlacement.above,
                        items: const [
                          PopupMenuItem(value: 'a', child: Text('Alpha')),
                          PopupMenuItem(value: 'b', child: Text('Beta')),
                        ],
                      );
                    },
                    child: const ColoredBox(color: Color(0xFF888888)),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('anchor')));
    await tester.pumpAndSettle();

    final buttonTop = tester.getRect(find.byKey(const Key('anchor'))).top;
    final menuBottom = tester.getRect(find.text('Beta')).bottom;
    expect(menuBottom, lessThanOrEqualTo(buttonTop));
  });
}
