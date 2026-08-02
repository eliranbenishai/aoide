import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_levels.dart';

void main() {
  test('band count is the display width', () {
    expect(AudioLevels.bandCount, 20);
  });

  test('silent has the right shape and is flagged synthetic', () {
    expect(AudioLevels.silent.bands, hasLength(AudioLevels.bandCount));
    expect(AudioLevels.silent.bands.every((b) => b == 0), isTrue);
    expect(AudioLevels.silent.leftRms, 0);
    expect(AudioLevels.silent.synthetic, isTrue);
  });

  test('synthesised levels stay in range and are flagged', () {
    for (var seed = 0; seed < 50; seed++) {
      final levels = AudioLevels.synthesised(intensity: 0.8, seed: seed);
      expect(levels.bands, hasLength(AudioLevels.bandCount));
      for (final b in levels.bands) {
        expect(b, inInclusiveRange(0.0, 1.0));
      }
      expect(levels.leftRms, inInclusiveRange(0.0, 1.0));
      expect(levels.synthetic, isTrue);
    }
  });

  test('zero intensity synthesises silence', () {
    final levels = AudioLevels.synthesised(intensity: 0, seed: 3);
    expect(levels.bands.every((b) => b == 0), isTrue);
  });

  test('synthesis is deterministic for a given seed', () {
    final a = AudioLevels.synthesised(intensity: 0.5, seed: 11);
    final b = AudioLevels.synthesised(intensity: 0.5, seed: 11);
    expect(a.bands, b.bands);
  });

  test('constructing with the wrong band count is rejected', () {
    expect(
      () => AudioLevels(
        bands: const [0.1, 0.2],
        leftRms: 0,
        rightRms: 0,
        synthetic: true,
      ),
      throwsA(isA<AssertionError>()),
    );
  });
}
