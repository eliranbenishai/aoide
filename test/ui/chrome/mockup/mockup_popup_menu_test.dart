import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/mockup/mockup_popup_menu.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

Widget _harness({
  required double zoom,
  required Widget child,
}) {
  return MaterialApp(
    home: ZoomScope(
      factor: zoom,
      devicePixelRatio: 1,
      child: child,
    ),
  );
}

List<PopupMenuEntry<String>> get _fiveItems => const [
      PopupMenuItem(value: 'a', child: Text('Always on top')),
      PopupMenuItem(value: 'b', child: Text('Look packs…')),
      PopupMenuItem(value: 'c', child: Text('Track info')),
      PopupMenuItem(value: 'd', child: Text('About Tramp')),
      PopupMenuItem(value: 'e', child: Text('Quit')),
    ];

void main() {
  testWidgets('below placement opens under the anchor when space allows',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 900));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      _harness(
        zoom: 1,
        child: Stack(
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
    // Short surface at 100% zoom: 5 items (~256px) will not fit under a top
    // anchor — open beside so the trigger stays visible.
    await tester.binding.setSurfaceSize(const Size(800, 280));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      _harness(
        zoom: 1,
        child: Stack(
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
                        items: _fiveItems,
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
    expect(menuLeft, greaterThanOrEqualTo(button.right));
  });

  testWidgets('zoomed menu fits under the anchor on a short surface',
      (tester) async {
    // Main-player-like window at 75% zoom. Anchor is inside the same scale
    // transform as ZoomedCanvas; menu must use that factor so it fits under.
    const zoom = 0.75;
    await tester.binding.setSurfaceSize(const Size(619, 261));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      _harness(
        zoom: zoom,
        child: Transform.scale(
          scale: zoom,
          alignment: Alignment.topLeft,
          child: SizedBox(
            width: 825,
            height: 348,
            child: Stack(
              children: [
                Positioned(
                  left: 22,
                  top: 60,
                  child: Builder(
                    builder: (buttonContext) => SizedBox(
                      key: const Key('anchor'),
                      width: 26,
                      height: 26,
                      child: GestureDetector(
                        onTap: () async {
                          final box =
                              buttonContext.findRenderObject()! as RenderBox;
                          await showMockupMenu<String>(
                            context: buttonContext,
                            anchor: box,
                            placement: MockupMenuPlacement.below,
                            items: _fiveItems,
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
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('anchor')));
    await tester.pumpAndSettle();

    final button = tester.getRect(find.byKey(const Key('anchor')));
    final first = tester.getRect(find.text('Always on top'));
    expect(first.top, greaterThanOrEqualTo(button.bottom));
    expect(first.left, lessThan(button.right + 1));
  });

  testWidgets('above placement opens over the anchor when space allows',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(800, 900));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      _harness(
        zoom: 1,
        child: Stack(
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
