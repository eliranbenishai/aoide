import 'dart:async';
import 'dart:isolate';
import 'dart:typed_data';

import '../playback/audio_levels.dart';
import 'pcm_decoder.dart';
import 'stft.dart';

export 'pcm_decoder.dart' show PcmBuffer, PcmLoader, MpvPcmDecoder;

/// Precomputed STFT frames for one track.
class Spectrogram {
  Spectrogram({
    required this.frames,
    required this.framesPerSecond,
    required this.sampleRateHz,
  });

  final List<List<double>> frames;
  final int framesPerSecond;
  final int sampleRateHz;

  AudioLevels levelsAt(Duration position) {
    if (frames.isEmpty) return AudioLevels.silent;
    final idx = ((position.inMilliseconds / 1000.0) * framesPerSecond)
        .floor()
        .clamp(0, frames.length - 1);
    final bands = List<double>.from(frames[idx]);
    var rms = 0.0;
    for (final b in bands) {
      rms += b * b;
    }
    rms = (rms / bands.length);
    final amp = rms > 0 ? (rms).clamp(0.0, 1.0) : 0.0;
    return AudioLevels(
      bands: bands,
      leftRms: amp,
      rightRms: amp,
      synthetic: false,
    );
  }
}

/// Isolate-hosted PCM → STFT → 20-band analyser.
///
/// Product path loads PCM via a second mpv `ao=pcm` decode (not a live filter
/// tap). Unit tests inject [pcmLoader] with synthetic fixtures.
class SpectrumAnalyzer {
  SpectrumAnalyzer({
    PcmLoader? pcmLoader,
    StftBandFolder? stft,
  })  : _pcmLoader = pcmLoader ?? MpvPcmDecoder().decode,
        _stft = stft ?? StftBandFolder();

  final PcmLoader _pcmLoader;
  final StftBandFolder _stft;

  static double bandCenterHz(int index, {required int sampleRateHz}) =>
      StftBandFolder.bandCenterHz(index, sampleRateHz: sampleRateHz);

  /// Pure PCM analysis seam (tests + isolate entry).
  Spectrogram analyzeMonoPcm(
    Float64List samples, {
    required int sampleRateHz,
  }) {
    final frames = _stft.analyze(samples, sampleRateHz: sampleRateHz);
    return Spectrogram(
      frames: frames,
      framesPerSecond: _stft.framesPerSecond,
      sampleRateHz: sampleRateHz,
    );
  }

  /// Analyse [path] and emit measured [AudioLevels] while [playing].
  ///
  /// When [position] is provided (production engine), frames are indexed by
  /// playback clock. Without it, wall-clock from the play edge is used.
  Stream<AudioLevels> attach({
    required String path,
    required Stream<bool> playing,
    Stream<Duration>? position,
  }) {
    final out = StreamController<AudioLevels>.broadcast();
    Spectrogram? spectrogram;
    var isPlaying = false;
    var currentPosition = Duration.zero;
    var playAnchor = DateTime.now();
    var anchorPosition = Duration.zero;
    Timer? ticker;
    StreamSubscription<bool>? playingSub;
    StreamSubscription<Duration>? positionSub;
    var cancelled = false;

    void emit() {
      if (out.isClosed || cancelled) return;
      if (!isPlaying || spectrogram == null) {
        out.add(AudioLevels.silent);
        return;
      }
      final pos = position != null
          ? currentPosition
          : anchorPosition + DateTime.now().difference(playAnchor);
      out.add(spectrogram!.levelsAt(pos));
    }

    void syncTicker() {
      ticker?.cancel();
      if (!isPlaying || spectrogram == null) {
        emit();
        return;
      }
      ticker = Timer.periodic(const Duration(milliseconds: 33), (_) => emit());
      emit();
    }

    Future<void> prepare() async {
      try {
        final pcm = await _pcmLoader(path);
        if (cancelled) return;
        spectrogram = await _computeSpectrogram(pcm);
      } catch (_) {
        // Decode failure → honest silence (still synthetic: false).
        spectrogram = Spectrogram(
          frames: [List<double>.filled(AudioLevels.bandCount, 0)],
          framesPerSecond: 30,
          sampleRateHz: 44100,
        );
      }
      if (!cancelled) syncTicker();
    }

    playingSub = playing.listen((playingNow) {
      isPlaying = playingNow;
      if (playingNow) {
        playAnchor = DateTime.now();
        anchorPosition = currentPosition;
      }
      syncTicker();
    });

    if (position != null) {
      positionSub = position.listen((pos) {
        currentPosition = pos;
        if (isPlaying) emit();
      });
    }

    unawaited(prepare());
    out.onCancel = () async {
      cancelled = true;
      ticker?.cancel();
      await playingSub?.cancel();
      await positionSub?.cancel();
    };

    return out.stream;
  }

  Future<Spectrogram> _computeSpectrogram(PcmBuffer pcm) async {
    final samples = Float64List.fromList(pcm.samples);
    final rate = pcm.sampleRateHz;
    try {
      final frames = await Isolate.run(() {
        return StftBandFolder().analyze(samples, sampleRateHz: rate);
      });
      return Spectrogram(
        frames: [
          for (final frame in frames) [for (final v in frame) v.toDouble()],
        ],
        framesPerSecond: _stft.framesPerSecond,
        sampleRateHz: rate,
      );
    } catch (_) {
      // Isolate transfer can fail in some test runners; STFT is still pure Dart.
      return analyzeMonoPcm(pcm.samples, sampleRateHz: rate);
    }
  }
}

