/// Force-mono downmix via mpv `audio-channels`.
///
/// Pins: `mono` when enabled, `auto` when cleared (plan Task 12).
class MonoController {
  MonoController({required PropertySetter setProperty})
      : _setProperty = setProperty;

  final PropertySetter _setProperty;

  Future<void> setForceMono(bool enabled) =>
      _setProperty('audio-channels', enabled ? 'mono' : 'auto');
}

typedef PropertySetter = Future<void> Function(String name, String value);
