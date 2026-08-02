import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:tramp/app.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/platform/os_media_controls_stub.dart';
import 'package:tramp/platform/settings_store.dart';

import 'support/test_fonts.dart';

class _MemorySettingsStore implements SettingsStore {
  _MemorySettingsStore([this._settings = TrampSettings.defaults]);

  TrampSettings _settings;

  @override
  Future<TrampSettings> read() async => _settings;

  @override
  Future<void> write(TrampSettings settings) async => _settings = settings;
}

void main() {
  setUpAll(loadTrampFonts);

  late List<MethodCall> windowCalls;

  setUp(() {
    windowCalls = [];
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(const MethodChannel('window_manager'),
            (call) async {
      windowCalls.add(call);
      return null;
    });
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(const MethodChannel('window_manager'), null);
  });

  List<String> methods() => windowCalls.map((call) => call.method).toList();

  MethodCall lastCall(String method) =>
      windowCalls.lastWhere((call) => call.method == method);

  testWidgets('TrampApp shows chrome shell with brand', (WidgetTester tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 900));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      TrampApp(
        engine: FakePlayerEngine(),
        osMediaControls: NoOpOsMediaControls(),
        settingsStore: _MemorySettingsStore(),
      ),
    );
    await tester.pump();
    expect(find.textContaining('TRAMP'), findsWidgets);
  });

  testWidgets('startup applies playlist mode: resizable, default tall size',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(824, 660));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      TrampApp(
        engine: FakePlayerEngine(),
        osMediaControls: NoOpOsMediaControls(),
        settingsStore: _MemorySettingsStore(),
      ),
    );
    await tester.pumpAndSettle();

    expect(methods(), containsAllInOrder(['setResizable', 'setMinimumSize', 'setBounds']));
    expect(lastCall('setResizable').arguments, {'isResizable': true});
    expect(lastCall('setMinimumSize').arguments['width'], 824);
    expect(lastCall('setMinimumSize').arguments['height'], 500);
    expect(lastCall('setBounds').arguments['width'], 824);
    expect(lastCall('setBounds').arguments['height'], 660);
  });

  testWidgets('switching to equalizer snaps to the fixed stack, back restores',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(824, 660));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      TrampApp(
        engine: FakePlayerEngine(),
        osMediaControls: NoOpOsMediaControls(),
        settingsStore: _MemorySettingsStore(),
      ),
    );
    await tester.pumpAndSettle();
    windowCalls.clear();

    await tester.tap(find.byKey(const Key('player-eq')));
    await tester.pumpAndSettle();

    expect(methods(), containsAllInOrder(['setResizable', 'setMinimumSize', 'setBounds']));
    expect(lastCall('setResizable').arguments, {'isResizable': false});
    expect(lastCall('setMinimumSize').arguments['width'], 824);
    expect(lastCall('setMinimumSize').arguments['height'], 466);
    expect(lastCall('setBounds').arguments['width'], 824);
    expect(lastCall('setBounds').arguments['height'], 466);

    windowCalls.clear();
    await tester.tap(find.byKey(const Key('player-pl')));
    await tester.pumpAndSettle();

    expect(lastCall('setResizable').arguments, {'isResizable': true});
    expect(lastCall('setBounds').arguments['width'], 824);
    expect(lastCall('setBounds').arguments['height'], 660);
  });

  testWidgets('a persisted playlist window size is restored across modes',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(1200, 900));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      TrampApp(
        engine: FakePlayerEngine(),
        osMediaControls: NoOpOsMediaControls(),
        settingsStore: _MemorySettingsStore(
          const TrampSettings(
            zoomPercent: 100,
            lowerRegion: LowerRegion.playlist,
            playlistWindowWidth: 900,
            playlistWindowHeight: 700,
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(lastCall('setBounds').arguments['width'], 900);
    expect(lastCall('setBounds').arguments['height'], 700);

    await tester.tap(find.byKey(const Key('player-eq')));
    await tester.pumpAndSettle();
    expect(lastCall('setBounds').arguments['width'], 824);
    expect(lastCall('setBounds').arguments['height'], 466);

    await tester.tap(find.byKey(const Key('player-pl')));
    await tester.pumpAndSettle();
    expect(lastCall('setBounds').arguments['width'], 900);
    expect(lastCall('setBounds').arguments['height'], 700);
  });
}
