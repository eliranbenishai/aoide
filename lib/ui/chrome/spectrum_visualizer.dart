import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../playback/audio_levels.dart';
import '../../theme/tramp_colors.dart';

/// Spectrum bars driven by the engine's analyser stream.
///
/// The widget owns no animation of its own — it renders what the engine reports,
/// smoothed so that a coarse source cadence still looks alive.
class SpectrumVisualizer extends StatefulWidget {
  const SpectrumVisualizer({super.key, required this.levels});

  final Stream<AudioLevels> levels;

  @override
  State<SpectrumVisualizer> createState() => _SpectrumVisualizerState();
}

class _SpectrumVisualizerState extends State<SpectrumVisualizer> {
  static const _decay = 0.86;
  static const _peakDecay = 0.97;

  late List<double> _bars;
  late List<double> _peaks;
  StreamSubscription<AudioLevels>? _subscription;

  @override
  void initState() {
    super.initState();
    _bars = List<double>.filled(AudioLevels.bandCount, 0);
    _peaks = List<double>.filled(AudioLevels.bandCount, 0);
    _listen();
  }

  @override
  void didUpdateWidget(SpectrumVisualizer oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.levels != widget.levels) {
      _subscription?.cancel();
      _listen();
    }
  }

  void _listen() {
    _subscription = widget.levels.listen(_onFrame);
  }

  void _onFrame(AudioLevels frame) {
    if (!mounted) return;
    setState(() {
      for (var i = 0; i < AudioLevels.bandCount; i++) {
        final incoming = frame.bands[i].clamp(0.0, 1.0);
        // Fast attack, slow decay: rise instantly, fall smoothly.
        _bars[i] = incoming > _bars[i] ? incoming : _bars[i] * _decay;
        _peaks[i] = math.max(_bars[i], _peaks[i] * _peakDecay);
      }
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      painter: SpectrumPainter(
        bars: List<double>.unmodifiable(_bars),
        peaks: List<double>.unmodifiable(_peaks),
      ),
    );
  }
}

class SpectrumPainter extends CustomPainter {
  const SpectrumPainter({required this.bars, required this.peaks});

  final List<double> bars;
  final List<double> peaks;

  @override
  void paint(Canvas canvas, Size size) {
    if (bars.isEmpty) return;

    final slot = size.width / bars.length;
    final barWidth = math.max(1.0, slot - 1);
    final lit = Paint()..color = TrampColors.phosphor;
    final dim = Paint()..color = TrampColors.phosphorDim;

    for (var i = 0; i < bars.length; i++) {
      final left = i * slot;
      final height = size.height * bars[i];
      if (height > 0) {
        canvas.drawRect(
          Rect.fromLTWH(left, size.height - height, barWidth, height),
          lit,
        );
      }

      final peakY = size.height - size.height * peaks[i];
      canvas.drawRect(Rect.fromLTWH(left, peakY, barWidth, 1), dim);
    }
  }

  @override
  bool shouldRepaint(SpectrumPainter old) =>
      !_same(old.bars, bars) || !_same(old.peaks, peaks);

  static bool _same(List<double> a, List<double> b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
