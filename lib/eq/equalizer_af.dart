import '../domain/equalizer_settings.dart';

/// Builds an mpv `af` property value for [settings].
///
/// Returns an empty string when EQ is off (or on but fully flat with zero
/// preamp) so the host clears the filter graph. When active, returns a
/// `lavfi=[...]` chain: optional preamp `volume` plus peaking `equalizer`
/// stages at the Winamp band centres (1-octave width).
String buildEqualizerAf(EqualizerSettings settings) {
  if (!settings.enabled) return '';

  final stages = <String>[];
  if (settings.preamp != 0) {
    stages.add('volume=${_formatDb(settings.preamp)}dB');
  }

  final gains = settings.gains;
  final freqs = EqualizerSettings.bandFrequencies;
  final count =
      gains.length < freqs.length ? gains.length : freqs.length;
  for (var i = 0; i < count; i++) {
    final gain = gains[i];
    if (gain == 0) continue;
    stages.add(
      'equalizer=f=${freqs[i]}:t=o:w=1:g=${_formatDb(gain)}',
    );
  }

  if (stages.isEmpty) return '';
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
