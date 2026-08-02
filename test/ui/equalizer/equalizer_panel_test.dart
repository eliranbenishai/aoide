import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/equalizer/equalizer_panel.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';
import 'package:tramp/ui/skin/skin_image.dart';

import '../../support/test_fonts.dart';

Finder skinImage(String asset) => find.byWidgetPredicate(
      (w) => w is SkinImage && w.asset == asset,
    );

class MemorySettingsStore implements SettingsStore {
  TrampSettings stored = TrampSettings.defaults;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async => stored = settings;
}

void main() {
  late EqualizerController controller;

  setUpAll(loadTrampFonts);

  setUp(() {
    controller = EqualizerController(
      store: MemorySettingsStore(),
      sink: const NoopEqualizerSink(),
    );
  });

  Future<void> pump(
    WidgetTester tester, {
    bool collapsed = false,
    VoidCallback? onCollapse,
    VoidCallback? onClose,
  }) async {
    // Default test surface is 800×600; the panel canvas is 812 wide.
    await tester.binding.setSurfaceSize(const Size(1024, 768));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: EqualizerPanel(
            controller: controller,
            collapsed: collapsed,
            draggableTitle: false,
            onCollapse: onCollapse ?? () {},
            onClose: onClose ?? () {},
          ),
        ),
      ),
    );
  }

  testWidgets('holds the locked canvas size', (tester) async {
    await pump(tester);
    expect(
      tester.getSize(find.byType(EqualizerPanel)),
      TrampMetrics.equalizer,
    );
    expect(tester.takeException(), isNull);
  });

  testWidgets('wears the branded graphite equalizer face', (tester) async {
    // TRAMP EQUALIZER and the band labels are now baked into the face art, so
    // they are asserted via the skin image, not as Text widgets.
    await pump(tester);
    expect(skinImage(GraphiteSkin.equalizerFace), findsOneWidget);
    expect(find.textContaining('WINAMP'), findsNothing);
  });

  testWidgets('renders all ten band faders', (tester) async {
    await pump(tester);
    for (var i = 0; i < 10; i++) {
      expect(find.byKey(Key('eq-band-$i')), findsOneWidget, reason: 'band $i');
    }
  });

  testWidgets('ON toggles the controller', (tester) async {
    await pump(tester);
    expect(controller.settings.enabled, isFalse);
    await tester.tap(find.byKey(const Key('eq-on')));
    await tester.pump();
    expect(controller.settings.enabled, isTrue);
  });

  testWidgets('AUTO toggles the controller', (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('eq-auto')));
    await tester.pump();
    expect(controller.settings.auto, isTrue);
  });

  testWidgets('dragging a band slider changes that gain only', (tester) async {
    await pump(tester);
    await tester.drag(find.byKey(const Key('eq-band-0')), const Offset(0, -40));
    await tester.pumpAndSettle();
    expect(controller.settings.gains[0], greaterThan(0));
    expect(controller.settings.gains[1], 0);
  });

  testWidgets('gain values are printed with one decimal and a sign',
      (tester) async {
    controller.setGain(0, 3.5);
    await pump(tester);
    expect(find.text('+3.5'), findsOneWidget);
  });

  testWidgets('collapse and close report to their callbacks', (tester) async {
    var collapses = 0;
    var closes = 0;
    await pump(
      tester,
      onCollapse: () => collapses++,
      onClose: () => closes++,
    );
    await tester.tap(find.byKey(const Key('eq-collapse')));
    await tester.tap(find.byKey(const Key('eq-close')));
    expect(collapses, 1);
    expect(closes, 1);
  });

  testWidgets('collapsed shows only the shade title strip', (tester) async {
    await pump(tester, collapsed: true);
    expect(
      tester.getSize(find.byType(EqualizerPanel)).height,
      TrampMetrics.titleBar,
    );
    expect(find.byKey(const Key('eq-band-0')), findsNothing);
    // The windowshade uses the dedicated shade face, not the full panel face.
    expect(skinImage(GraphiteSkin.equalizerShadeFace), findsOneWidget);
    expect(skinImage(GraphiteSkin.equalizerFace), findsNothing);
  });

  testWidgets('presets menu lists the built-in curves and applies one',
      (tester) async {
    await pump(tester);
    await tester.tap(find.byKey(const Key('eq-presets')));
    await tester.pumpAndSettle();
    expect(find.text('Rock'), findsOneWidget);
    await tester.tap(find.text('Rock'));
    await tester.pumpAndSettle();
    expect(controller.settings.presetName, 'Rock');
  });
}
