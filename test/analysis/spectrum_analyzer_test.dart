import 'dart:async';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/analysis/spectrum_analyzer.dart';
import 'package:tramp/playback/audio_levels.dart';

void main() {
  const sampleRate = 44100;

  test('impulse spreads energy across bands (not synthesised product path)', () {
    final samples = Float64List(4096);
    // Centre of the first Hann window (edges are zeroed by the window).
    samples[512] = 1.0;

    final analyzer = SpectrumAnalyzer();
    final spectrogram = analyzer.analyzeMonoPcm(
      samples,
      sampleRateHz: sampleRate,
    );

    expect(spectrogram.frames, isNotEmpty);
    final frame = spectrogram.levelsAt(Duration.zero);
    expect(frame.synthetic, isFalse);
    expect(frame.bands, hasLength(AudioLevels.bandCount));
    // Impulse has broadband energy — several bands must light up.
    final lit = frame.bands.where((b) => b > 0.05).length;
    expect(lit, greaterThanOrEqualTo(8));
  });

  test('1 kHz sine peaks near the band containing 1 kHz', () {
    final samples = Float64List(sampleRate); // 1 second
    for (var i = 0; i < samples.length; i++) {
      samples[i] = 0.5 * math.sin(2 * math.pi * 1000 * i / sampleRate);
    }

    final analyzer = SpectrumAnalyzer();
    final spectrogram = analyzer.analyzeMonoPcm(
      samples,
      sampleRateHz: sampleRate,
    );

    // Mid-file frame avoids window edge effects.
    final frame = spectrogram.levelsAt(const Duration(milliseconds: 500));
    expect(frame.synthetic, isFalse);

    final peakIndex = _argmax(frame.bands);
    final peakHz = SpectrumAnalyzer.bandCenterHz(
      peakIndex,
      sampleRateHz: sampleRate,
    );
    // Log bands are coarse; allow a neighbouring band.
    expect(peakHz, inInclusiveRange(500, 2000));
    expect(frame.bands[peakIndex], greaterThan(0.3));
  });

  test('silence yields measured silent frames', () {
    final samples = Float64List(2048);
    final spectrogram = SpectrumAnalyzer().analyzeMonoPcm(
      samples,
      sampleRateHz: sampleRate,
    );
    final frame = spectrogram.levelsAt(Duration.zero);
    expect(frame.synthetic, isFalse);
    expect(frame.bands.every((b) => b < 0.01), isTrue);
  });

  test('attach emits measured frames while playing', () async {
    final samples = Float64List(8192);
    for (var i = 0; i < samples.length; i++) {
      samples[i] = 0.4 * math.sin(2 * math.pi * 440 * i / sampleRate);
    }
    final analyzer = SpectrumAnalyzer(
      pcmLoader: (_) async => PcmBuffer(samples: samples, sampleRateHz: sampleRate),
    );

    final playing = StreamController<bool>();
    final position = StreamController<Duration>();
    addTearDown(() async {
      await playing.close();
      await position.close();
    });

    final frames = <AudioLevels>[];
    final sub = analyzer
        .attach(
          path: 'fixture://tone.wav',
          playing: playing.stream,
          position: position.stream,
        )
        .listen(frames.add);
    addTearDown(sub.cancel);

    playing.add(false);
    position.add(Duration.zero);
    // Wait for isolate STFT to finish.
    await Future<void>.delayed(const Duration(milliseconds: 300));
    expect(frames, isNotEmpty);
    expect(frames.last.synthetic, isFalse);
    expect(frames.last.bands.every((b) => b == 0), isTrue);

    playing.add(true);
    position.add(const Duration(milliseconds: 100));
    await Future<void>.delayed(const Duration(milliseconds: 100));
    expect(frames.last.synthetic, isFalse);
    expect(frames.last.bands.any((b) => b > 0.05), isTrue);
  });
}

int _argmax(List<double> values) {
  var best = 0;
  for (var i = 1; i < values.length; i++) {
    if (values[i] > values[best]) best = i;
  }
  return best;
}
