import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_materials.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/equalizer/mockup_equalizer.dart';
import 'package:tramp/ui/session/session_messages.dart';
import 'package:tramp/ui/windows/equalizer_window.dart';

import '../../support/test_fonts.dart';
import '../../support/look_harness.dart';

final _altLook = ResolvedLook(
  id: 'alt',
  name: 'Alt',
  palette: BuiltinLook.resolved.palette,
  materials: const LookMaterials(
    bevelLightOpacity: 0.15,
    bevelSoftOpacity: 0.06,
    spectrumStops: [
      Color(0xFFFF0000),
      Color(0xFFFF0000),
      Color(0xFFFF0000),
      Color(0xFFFF0000),
    ],
    railStops: [
      Color(0xFFFF0000),
      Color(0xFFFF0000),
      Color(0xFFFF0000),
    ],
  ),
  chromeFamily: 'TrampCondensed',
  lcdFamily: 'TrampMono',
);

class MemorySettingsStore implements SettingsStore {
  TrampSettings stored = TrampSettings.defaults;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async => stored = settings;
}

/// Applies EQ session commands the way [SessionHostApp] does.
void applyEqCommand(EqualizerController eq, SessionCommand command) {
  switch (command) {
    case EqGainCommand(:final band, :final gain):
      eq.setGain(band, gain);
    case EqPreampCommand(:final preamp):
      eq.setPreamp(preamp);
    case EqEnabledCommand(:final enabled):
      eq.setEnabled(enabled);
    case EqAutoCommand(:final enabled):
      eq.setAuto(enabled);
    case ApplyPresetCommand(:final name):
      eq.applyPreset(name);
    default:
      break;
  }
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

  Future<void> pumpEq(
    WidgetTester tester, {
    required List<SessionCommand> commands,
    EqualizerSettings? settings,
    bool shaded = false,
    ResolvedLook? look,
    List<VoidCallback>? collapses,
    List<VoidCallback>? closes,
  }) async {
    await tester.binding.setSurfaceSize(const Size(900, 400));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: wrapWithLook(
            EqualizerWindow(
              settings: settings ?? controller.settings,
              shaded: shaded,
              draggableTitle: false,
              onSessionCommand: commands.add,
              onCollapse: () => collapses?.add(() {}),
              onClose: () => closes?.add(() {}),
            ),
            look: look,
          ),
        ),
      ),
    );
  }

  testWidgets('holds the locked canvas size', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    expect(
      tester.getSize(find.byType(EqualizerWindow)),
      TrampMetrics.equalizer,
    );
    expect(tester.getSize(find.byType(MockupEqualizer)), MockupEqualizer.bodySize);
    expect(tester.takeException(), isNull);
  });

  testWidgets('shade collapses to title bar height', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands, shaded: true);

    expect(
      tester.getSize(find.byType(EqualizerWindow)).height,
      TrampMetrics.titleBar,
    );
    expect(find.byType(MockupEqualizer), findsNothing);
  });

  testWidgets('On sends EqEnabledCommand and updates controller', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    await tester.tap(find.byKey(const Key('eq-on')));
    await tester.pump();

    expect(commands, hasLength(1));
    expect(commands.single, isA<EqEnabledCommand>());
    expect((commands.single as EqEnabledCommand).enabled, isTrue);

    applyEqCommand(controller, commands.single);
    expect(controller.settings.enabled, isTrue);
  });

  testWidgets('Auto sends EqAutoCommand and updates controller', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    await tester.tap(find.byKey(const Key('eq-auto')));
    await tester.pump();

    expect(commands.single, isA<EqAutoCommand>());
    applyEqCommand(controller, commands.single);
    expect(controller.settings.auto, isTrue);
  });

  testWidgets('band drag sends EqGainCommand to controller', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    final band = find.byKey(const Key('eq-band-0'));
    final center = tester.getCenter(band);
    // Drag toward top → positive gain.
    await tester.dragFrom(center, const Offset(0, -60));
    await tester.pump();

    expect(commands, isNotEmpty);
    expect(commands.last, isA<EqGainCommand>());
    final cmd = commands.last as EqGainCommand;
    expect(cmd.band, 0);
    expect(cmd.gain, greaterThan(0));

    applyEqCommand(controller, cmd);
    expect(controller.settings.gains[0], cmd.gain);
    expect(controller.settings.presetName, isNull);
  });

  testWidgets('preamp drag sends EqPreampCommand to controller', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    final preamp = find.byKey(const Key('eq-preamp'));
    await tester.dragFrom(tester.getCenter(preamp), const Offset(0, 50));
    await tester.pump();

    expect(commands.last, isA<EqPreampCommand>());
    final cmd = commands.last as EqPreampCommand;
    expect(cmd.preamp, lessThan(0));

    applyEqCommand(controller, cmd);
    expect(controller.settings.preamp, cmd.preamp);
  });

  testWidgets('preset menu sends ApplyPresetCommand to controller', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(tester, commands: commands);

    await tester.tap(find.byKey(const Key('eq-presets')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Rock').last);
    await tester.pumpAndSettle();

    expect(commands.single, isA<ApplyPresetCommand>());
    expect((commands.single as ApplyPresetCommand).name, 'Rock');

    applyEqCommand(controller, commands.single);
    expect(controller.settings.presetName, 'Rock');
    expect(controller.settings.gains, EqualizerPresets.builtIn['Rock']);
  });

  testWidgets('collapse invokes shade callback', (tester) async {
    final commands = <SessionCommand>[];
    var collapsed = 0;
    await tester.binding.setSurfaceSize(const Size(900, 400));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: EqualizerWindow(
            settings: EqualizerSettings.flat,
            draggableTitle: false,
            onSessionCommand: commands.add,
            onCollapse: () => collapsed++,
          ),
        ),
      ),
    );

    await tester.tap(find.bySemanticsLabel('Collapse'));
    await tester.pump();
    expect(collapsed, 1);
  });

  testWidgets('band track repaints when look materials change', (tester) async {
    final commands = <SessionCommand>[];
    final settings = EqualizerSettings.flat.copyWith(
      gains: [6.0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    );
    await pumpEq(tester, commands: commands, settings: settings);

    final trackPaint = find.descendant(
      of: find.byKey(const Key('eq-band-0')),
      matching: find.byType(CustomPaint),
    );
    final builtinPainter = tester.widget<CustomPaint>(trackPaint).painter!;

    await pumpEq(tester, commands: commands, settings: settings, look: _altLook);
    final altPainter = tester.widget<CustomPaint>(trackPaint).painter!;

    expect(altPainter.shouldRepaint(builtinPainter), isTrue);
  });

  testWidgets('curve caption shows preset name', (tester) async {
    final commands = <SessionCommand>[];
    await pumpEq(
      tester,
      commands: commands,
      settings: EqualizerSettings.flat.copyWith(
        enabled: true,
        gains: EqualizerPresets.builtIn['Pop'],
        presetName: 'Pop',
      ),
    );
    expect(find.textContaining('POP'), findsWidgets);
  });
}
