import '../domain/equalizer_settings.dart';

/// Builds an mpv `af` property value for [settings].
///
/// Returns an empty string when EQ is off so the host clears the filter graph.
/// When enabled, always emits a **stable** lavfi chain: preamp `volume` plus
/// all ten peaking `equalizer` stages (including 0 dB bands). A fixed topology
/// avoids tearing the graph down/up on every fader tick (audio dropouts).
String buildEqualizerAf(EqualizerSettings settings) {
  if (!settings.enabled) return '';

  final stages = <String>[
    'volume=${_formatDb(settings.preamp)}dB',
  ];

  final gains = settings.gains;
  final freqs = EqualizerSettings.bandFrequencies;
  final count =
      gains.length < freqs.length ? gains.length : freqs.length;
  for (var i = 0; i < count; i++) {
    stages.add(
      'equalizer=f=${freqs[i]}:t=o:w=1:g=${_formatDb(gains[i])}',
    );
  }

  return 'lavfi=[${stages.join(',')}]';
}

String _formatDb(double db) {
  if (db == db.roundToDouble()) return db.toInt().toString();
  final text = db.toStringAsFixed(2);
  if (text.contains('.')) {
    return text.replaceFirst(RegExp(r'\.?0+$'), '');
  }
  return text;
}
