import 'dart:async';

import 'package:flutter/foundation.dart';

import '../domain/equalizer_settings.dart';
import '../platform/settings_store.dart';

/// Where equalizer curves become audible on the playback path.
///
/// Production uses `MpvEqualizerSink` (mpv `af` / lavfi equalizer) once the
/// full-libmpv measurement gate (`tool/eq_measure.dart`) is green.
/// [NoopEqualizerSink] remains for tests and hosts without a real player.
/// Never trust set-property return codes alone — slim libmpv historically
/// reported success while no-oping.
abstract class EqualizerSink {
  Future<void> apply(EqualizerSettings settings);
}

class NoopEqualizerSink implements EqualizerSink {
  const NoopEqualizerSink();

  @override
  Future<void> apply(EqualizerSettings settings) async {}
}

class EqualizerController extends ChangeNotifier {
  EqualizerController({
    required SettingsStore store,
    required EqualizerSink sink,
  })  : _store = store,
        _sink = sink;

  final SettingsStore _store;
  final EqualizerSink _sink;

  EqualizerSettings _settings = EqualizerSettings.flat;

  EqualizerSettings get settings => _settings;

  List<String> get presetNames => EqualizerPresets.builtIn.keys.toList();

  Future<void> load() async {
    final persisted = await _store.read();
    _settings = persisted.equalizerCurve;
    notifyListeners();
    unawaited(_sink.apply(_settings));
  }

  void setEnabled(bool value) => _apply(_settings.copyWith(enabled: value));

  void setAuto(bool value) => _apply(_settings.copyWith(auto: value));

  void setGain(int band, double gain) => _apply(_settings.withGain(band, gain));

  void setPreamp(double value) => _apply(_settings.withPreamp(value));

  void resetFlat() => _apply(
        EqualizerSettings.flat.copyWith(
          enabled: _settings.enabled,
          auto: _settings.auto,
          presetName: 'Flat',
        ),
      );

  void applyPreset(String name) {
    final curve = EqualizerPresets.builtIn[name];
    if (curve == null) return;
    _apply(_settings.copyWith(gains: List<double>.of(curve), presetName: name));
  }

  void _apply(EqualizerSettings next) {
    if (next == _settings) return;
    _settings = next;
    notifyListeners();
    unawaited(_sink.apply(next));
    unawaited(_persist());
  }

  Future<void> _persist() async {
    final current = await _store.read();
    await _store.write(current.copyWith(equalizerCurve: _settings));
  }
}
