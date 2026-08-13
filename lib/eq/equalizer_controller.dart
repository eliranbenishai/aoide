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
    this.applyDebounce = const Duration(milliseconds: 40),
    this.persistDebounce = const Duration(milliseconds: 400),
  })  : _store = store,
        _sink = sink;

  final SettingsStore _store;
  final EqualizerSink _sink;

  /// Coalesce rapid fader updates before touching mpv `af`.
  final Duration applyDebounce;

  /// Coalesce disk writes while dragging bands.
  final Duration persistDebounce;

  EqualizerSettings _settings = EqualizerSettings.flat;
  Timer? _applyTimer;
  Timer? _persistTimer;

  EqualizerSettings get settings => _settings;

  List<String> get presetNames => EqualizerPresets.builtIn.keys.toList();

  Future<void> load() async {
    final persisted = await _store.read();
    _settings = persisted.equalizerCurve;
    notifyListeners();
    await _sink.apply(_settings);
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

    _applyTimer?.cancel();
    if (applyDebounce <= Duration.zero) {
      unawaited(_sink.apply(next));
    } else {
      _applyTimer = Timer(applyDebounce, () {
        unawaited(_sink.apply(_settings));
      });
    }

    _persistTimer?.cancel();
    if (persistDebounce <= Duration.zero) {
      unawaited(_persist());
    } else {
      _persistTimer = Timer(persistDebounce, () {
        unawaited(_persist());
      });
    }
  }

  /// Flush pending apply/persist (tests / teardown).
  Future<void> flush() async {
    _applyTimer?.cancel();
    _applyTimer = null;
    _persistTimer?.cancel();
    _persistTimer = null;
    await _sink.apply(_settings);
    await _persist();
  }

  Future<void> _persist() async {
    final current = await _store.read();
    await _store.write(current.copyWith(equalizerCurve: _settings));
  }

  /// Drops pending debounces. Call [flush] first to keep an in-flight edit.
  @override
  void dispose() {
    _applyTimer?.cancel();
    _applyTimer = null;
    _persistTimer?.cancel();
    _persistTimer = null;
    super.dispose();
  }
}
