import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';

class MemorySettingsStore implements SettingsStore {
  MemorySettingsStore([this.stored = TrampSettings.defaults]);

  TrampSettings stored;
  int writes = 0;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async {
    stored = settings;
    writes++;
  }
}

class RecordingSink implements EqualizerSink {
  final applied = <EqualizerSettings>[];

  @override
  Future<void> apply(EqualizerSettings settings) async =>
      applied.add(settings);
}

void main() {
  test('starts flat', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    expect(c.settings, EqualizerSettings.flat);
  });

  test('load adopts persisted state', () async {
    const stored = EqualizerSettings(
      enabled: true,
      auto: true,
      preamp: 4,
      gains: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
      presetName: 'Rock',
    );
    final c = EqualizerController(
      store: MemorySettingsStore(
        TrampSettings.defaults.copyWith(equalizerCurve: stored),
      ),
      sink: NoopEqualizerSink(),
    );
    await c.load();
    expect(c.settings, stored);
  });

  test('setGain updates, notifies and persists', () async {
    final store = MemorySettingsStore();
    final c = EqualizerController(
      store: store,
      sink: NoopEqualizerSink(),
      applyDebounce: Duration.zero,
      persistDebounce: Duration.zero,
    );
    var notifications = 0;
    c.addListener(() => notifications++);

    c.setGain(2, 6);
    expect(c.settings.gains[2], 6);
    expect(notifications, 1);
    await Future<void>.delayed(Duration.zero);
    expect(store.stored.equalizerCurve.gains[2], 6);
  });

  test('applyPreset sets the curve and records the name', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
      applyDebounce: Duration.zero,
      persistDebounce: Duration.zero,
    );
    c.applyPreset('Rock');
    expect(c.settings.gains, EqualizerPresets.builtIn['Rock']);
    expect(c.settings.presetName, 'Rock');
  });

  test('an unknown preset name is ignored', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
      applyDebounce: Duration.zero,
      persistDebounce: Duration.zero,
    );
    c.applyPreset('Nope');
    expect(c.settings, EqualizerSettings.flat);
  });

  test('editing a band clears the preset name', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
      applyDebounce: Duration.zero,
      persistDebounce: Duration.zero,
    );
    c.applyPreset('Rock');
    c.setGain(0, 1);
    expect(c.settings.presetName, isNull);
  });

  test('every change reaches the sink', () async {
    final sink = RecordingSink();
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: sink,
      applyDebounce: Duration.zero,
      persistDebounce: Duration.zero,
    );
    c.setEnabled(true);
    c.setPreamp(3);
    await c.flush();
    expect(sink.applied, isNotEmpty);
    expect(sink.applied.last.preamp, 3);
  });

  test('rapid setGain coalesces sink applies when debounced', () async {
    final sink = RecordingSink();
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: sink,
      applyDebounce: const Duration(milliseconds: 30),
      persistDebounce: const Duration(hours: 1),
    );
    c.setEnabled(true);
    for (var i = 0; i < 10; i++) {
      c.setGain(0, i.toDouble());
    }
    expect(sink.applied, isEmpty);
    await Future<void>.delayed(const Duration(milliseconds: 50));
    expect(sink.applied, hasLength(1));
    expect(sink.applied.single.gains[0], 9);
  });

  test('preset names are the built-in curves', () {
    final c = EqualizerController(
      store: MemorySettingsStore(),
      sink: NoopEqualizerSink(),
    );
    expect(c.presetNames, EqualizerPresets.builtIn.keys.toList());
  });
}
