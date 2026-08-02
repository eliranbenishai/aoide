import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';

void main() {
  test('bands are the Winamp 2.x set', () {
    expect(EqualizerSettings.bandFrequencies,
        [60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000]);
  });

  test('flat is ten zero gains, disabled', () {
    expect(EqualizerSettings.flat.gains, hasLength(10));
    expect(EqualizerSettings.flat.gains.every((g) => g == 0), isTrue);
    expect(EqualizerSettings.flat.preamp, 0);
    expect(EqualizerSettings.flat.enabled, isFalse);
  });

  test('gains clamp to plus or minus twelve dB', () {
    final hot = EqualizerSettings.flat.withGain(0, 99);
    expect(hot.gains[0], EqualizerSettings.gainLimit);
    final cold = EqualizerSettings.flat.withGain(0, -99);
    expect(cold.gains[0], -EqualizerSettings.gainLimit);
  });

  test('withGain leaves other bands untouched', () {
    final s = EqualizerSettings.flat.withGain(3, 5);
    expect(s.gains[3], 5);
    expect(s.gains[4], 0);
  });

  test('an out-of-range band index is ignored rather than throwing', () {
    expect(EqualizerSettings.flat.withGain(99, 5).gains,
        EqualizerSettings.flat.gains);
  });

  test('json round-trips every field', () {
    const original = EqualizerSettings(
      enabled: true,
      auto: true,
      preamp: 3.5,
      gains: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
      presetName: 'Rock',
    );
    expect(EqualizerSettings.fromJson(original.toJson()), original);
  });

  test('malformed json falls back to flat', () {
    expect(EqualizerSettings.fromJson(const {}), EqualizerSettings.flat);
    expect(
      EqualizerSettings.fromJson(const {
        'gains': [1, 2]
      }),
      EqualizerSettings.flat,
      reason: 'wrong band count is corrupt, not partially usable',
    );
  });

  test('every built-in preset has ten in-range gains', () {
    expect(EqualizerPresets.builtIn, isNotEmpty);
    for (final entry in EqualizerPresets.builtIn.entries) {
      expect(entry.value, hasLength(10), reason: entry.key);
      for (final g in entry.value) {
        expect(g.abs(), lessThanOrEqualTo(EqualizerSettings.gainLimit),
            reason: entry.key);
      }
    }
  });
}
