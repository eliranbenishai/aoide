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
class MpvEqualizerSink implements EqualizerSink {
  MpvEqualizerSink(this._player);

  final Player _player;

  @override
  Future<void> apply(EqualizerSettings settings) async {
    final platform = _player.platform;
    if (platform is! NativePlayer) return;

    final af = buildEqualizerAf(settings);
    await platform.setProperty('af', af);
  }
}
