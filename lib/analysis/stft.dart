import 'dart:math' as math;
import 'dart:typed_data';

import '../playback/audio_levels.dart';

/// STFT → log-folded 20-band spectrum frames.
class StftBandFolder {
  StftBandFolder({
    this.fftSize = 1024,
    this.framesPerSecond = 30,
  });

  final int fftSize;
  final int framesPerSecond;

  /// Analyse mono PCM into per-frame band energies (0..1).
  List<List<double>> analyze(
    Float64List samples, {
    required int sampleRateHz,
  }) {
    if (samples.isEmpty || sampleRateHz <= 0) return const [];

    final hop = math.max(1, sampleRateHz ~/ framesPerSecond);
    final window = _hann(fftSize);
    final bandEdges = _logBandEdges(sampleRateHz, fftSize);
    final frames = <List<double>>[];

    for (var start = 0; start + fftSize <= samples.length; start += hop) {
      final re = Float64List(fftSize);
      final im = Float64List(fftSize);
      for (var i = 0; i < fftSize; i++) {
        re[i] = samples[start + i] * window[i];
      }
      _fftInPlace(re, im);

      final mags = Float64List(fftSize ~/ 2);
      for (var k = 0; k < mags.length; k++) {
        mags[k] = math.sqrt(re[k] * re[k] + im[k] * im[k]);
      }

      final bands = List<double>.filled(AudioLevels.bandCount, 0);
      for (var b = 0; b < AudioLevels.bandCount; b++) {
        final lo = bandEdges[b];
        final hi = bandEdges[b + 1];
        var sum = 0.0;
        var count = 0;
        for (var k = lo; k < hi && k < mags.length; k++) {
          sum += mags[k];
          count++;
        }
        bands[b] = count == 0 ? 0 : sum / count;
      }

      // Normalise per frame so a loud tone fills the display.
      var peak = 0.0;
      for (final v in bands) {
        if (v > peak) peak = v;
      }
      if (peak > 1e-12) {
        for (var i = 0; i < bands.length; i++) {
          bands[i] = (bands[i] / peak).clamp(0.0, 1.0);
        }
      }
      frames.add(bands);
    }

    if (frames.isEmpty) {
      frames.add(List<double>.filled(AudioLevels.bandCount, 0));
    }
    return frames;
  }

  /// Centre frequency (Hz) of display band [index].
  static double bandCenterHz(int index, {required int sampleRateHz}) {
    final edgesHz = _logBandEdgeHz(sampleRateHz);
    return math.sqrt(edgesHz[index] * edgesHz[index + 1]);
  }

  static List<int> _logBandEdges(int sampleRateHz, int fftSize) {
    final edgesHz = _logBandEdgeHz(sampleRateHz);
    final binHz = sampleRateHz / fftSize;
    return [
      for (final hz in edgesHz)
        (hz / binHz).floor().clamp(1, (fftSize ~/ 2) - 1),
    ];
  }

  static List<double> _logBandEdgeHz(int sampleRateHz) {
    const fMin = 40.0;
    final fMax = sampleRateHz / 2.0;
    final edges = <double>[];
    for (var i = 0; i <= AudioLevels.bandCount; i++) {
      final t = i / AudioLevels.bandCount;
      edges.add(fMin * math.pow(fMax / fMin, t));
    }
    return edges;
  }

  static Float64List _hann(int n) {
    final w = Float64List(n);
    for (var i = 0; i < n; i++) {
      w[i] = 0.5 * (1 - math.cos(2 * math.pi * i / (n - 1)));
    }
    return w;
  }

  /// In-place radix-2 Cooley–Tukey FFT.
  static void _fftInPlace(Float64List re, Float64List im) {
    final n = re.length;
    if (n == 0 || (n & (n - 1)) != 0) {
      throw ArgumentError('fftSize must be power of two');
    }

    // Bit-reverse permutation.
    var j = 0;
    for (var i = 1; i < n; i++) {
      var bit = n >> 1;
      for (; j & bit != 0; bit >>= 1) {
        j ^= bit;
      }
      j ^= bit;
      if (i < j) {
        final tr = re[i];
        re[i] = re[j];
        re[j] = tr;
        final ti = im[i];
        im[i] = im[j];
        im[j] = ti;
      }
    }

    for (var len = 2; len <= n; len <<= 1) {
      final ang = -2 * math.pi / len;
      final wlenRe = math.cos(ang);
      final wlenIm = math.sin(ang);
      for (var i = 0; i < n; i += len) {
        var wRe = 1.0;
        var wIm = 0.0;
        for (var k = 0; k < len ~/ 2; k++) {
          final uRe = re[i + k];
          final uIm = im[i + k];
          final vRe = re[i + k + len ~/ 2] * wRe - im[i + k + len ~/ 2] * wIm;
          final vIm = re[i + k + len ~/ 2] * wIm + im[i + k + len ~/ 2] * wRe;
          re[i + k] = uRe + vRe;
          im[i + k] = uIm + vIm;
          re[i + k + len ~/ 2] = uRe - vRe;
          im[i + k + len ~/ 2] = uIm - vIm;
          final nextWRe = wRe * wlenRe - wIm * wlenIm;
          wIm = wRe * wlenIm + wIm * wlenRe;
          wRe = nextWRe;
        }
      }
    }
  }
}
