import 'package:media_kit/media_kit.dart';

import '../domain/equalizer_settings.dart';
import 'equalizer_af.dart';
import 'equalizer_controller.dart';

export 'equalizer_af.dart' show buildEqualizerAf;

/// Applies [EqualizerSettings] to a media_kit [Player] via mpv `af`.
///
/// Requires a **full** libmpv build with libavfilter equalizer support.
/// Audibility is gated by `tool/eq_measure.dart`, not by set-property return
/// codes (slim builds historically reported success while no-oping filters).
///
/// Coalesces overlapping applies: fader drags fire faster than `setProperty`
/// round-trips; stacking them glitches audio. Latest-wins while one apply
/// is in flight.
class MpvEqualizerSink implements EqualizerSink {
  MpvEqualizerSink(this._player);

  final Player _player;
  EqualizerSettings? _pending;
  bool _applying = false;
  String? _lastAf;

  @override
  Future<void> apply(EqualizerSettings settings) async {
    _pending = settings;
    if (_applying) return;
    _applying = true;
    try {
      final platform = _player.platform;
      if (platform is! NativePlayer) {
        _pending = null;
        return;
      }
      while (_pending != null) {
        final next = _pending!;
        _pending = null;
        final af = buildEqualizerAf(next);
        if (af == _lastAf) continue;
        _lastAf = af;
        await platform.setProperty('af', af);
      }
    } finally {
      _applying = false;
    }
    // Cover the race where apply() arrived after the loop drained _pending
    // but before _applying cleared.
    if (_pending != null) {
      await apply(_pending!);
    }
  }
}
