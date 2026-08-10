import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_popup_menu.dart';

void main() {
  testWidgets('below placement opens under the anchor when space allows',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 900));
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

    final button = tester.getRect(find.byKey(const Key('anchor')));
    final menuTop = tester.getRect(find.text('Alpha')).top;
    expect(menuTop, greaterThanOrEqualTo(button.bottom));
  });

  testWidgets('below placement opens beside the anchor when space is tight',
      (tester) async {
    // Short surface: 5 default menu items (~256px) will not fit under a top
    // anchor — must not cover the button (Material would otherwise clamp y→0).
    await tester.binding.setSurfaceSize(const Size(800, 280));
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
                          PopupMenuItem(value: 'a', child: Text('Always on top')),
                          PopupMenuItem(value: 'b', child: Text('Look packs…')),
                          PopupMenuItem(value: 'c', child: Text('Track info')),
                          PopupMenuItem(value: 'd', child: Text('About Tramp')),
                          PopupMenuItem(value: 'e', child: Text('Quit')),
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

    final button = tester.getRect(find.byKey(const Key('anchor')));
    final menuLeft = tester.getRect(find.text('Always on top')).left;
    // Beside: menu clears the button so the lit trigger stays visible.
    expect(menuLeft, greaterThanOrEqualTo(button.right));
  });

  testWidgets('above placement opens over the anchor when space allows',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 900));
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
