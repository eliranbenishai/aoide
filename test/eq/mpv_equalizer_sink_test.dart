import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/eq/equalizer_af.dart';

void main() {
  test('disabled settings clear the af graph', () {
    expect(buildEqualizerAf(EqualizerSettings.flat), isEmpty);
    expect(
      buildEqualizerAf(
        const EqualizerSettings(
          enabled: false,
          auto: false,
          preamp: 3,
          gains: [0, 0, 0, 0, 0, 12, 0, 0, 0, 0],
        ),
      ),
      isEmpty,
    );
  });

  test('enabled flat curve with zero preamp clears af', () {
    expect(
      buildEqualizerAf(
        EqualizerSettings.flat.copyWith(enabled: true),
      ),
      isEmpty,
    );
  });

  test('preamp alone becomes a volume stage', () {
    expect(
      buildEqualizerAf(
        const EqualizerSettings(
          enabled: true,
          auto: false,
          preamp: 3,
          gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        ),
      ),
      'lavfi=[volume=3dB]',
    );
  });

  test('single boosted band uses octave peaking equalizer', () {
    expect(
      buildEqualizerAf(
        const EqualizerSettings(
          enabled: true,
          auto: false,
          preamp: 0,
          gains: [0, 0, 0, 0, 12, 0, 0, 0, 0, 0],
        ),
      ),
      'lavfi=[equalizer=f=1000:t=o:w=1:g=12]',
    );
  });

  test('preamp and multiple bands join in lavfi chain order', () {
    expect(
      buildEqualizerAf(
        const EqualizerSettings(
          enabled: true,
          auto: false,
          preamp: -2.5,
          gains: [5, 0, 0, 0, 0, 0, 0, 0, 0, -3],
        ),
      ),
      'lavfi=[volume=-2.5dB,equalizer=f=60:t=o:w=1:g=5,'
      'equalizer=f=16000:t=o:w=1:g=-3]',
    );
  });

  test('band centres match EqualizerSettings.bandFrequencies', () {
    final gains = List<double>.filled(10, 0);
    gains[0] = 1;
    gains[9] = 2;
    final af = buildEqualizerAf(
      EqualizerSettings(
        enabled: true,
        auto: false,
        preamp: 0,
        gains: gains,
      ),
    );
    expect(af, contains('f=${EqualizerSettings.bandFrequencies[0]}'));
    expect(af, contains('f=${EqualizerSettings.bandFrequencies[9]}'));
  });
}
