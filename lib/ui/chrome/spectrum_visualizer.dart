import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../look/look_materials.dart';
import '../../playback/audio_levels.dart';
import '../../theme/look_scope.dart';

/// Spectrum bars driven by the engine's analyser stream.
///
/// Cyan→accent gradient + peak caps match `player-mockup-2.html` / mockup main.
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
    final look = LookScope.of(context);
    return CustomPaint(
      painter: SpectrumPainter(
        bars: List<double>.unmodifiable(_bars),
        peaks: List<double>.unmodifiable(_peaks),
        materials: look.materials,
        phosphor: look.palette.phosphorDefault,
        phosphorHot: look.palette.phosphorHot,
      ),
    );
  }
}

class SpectrumPainter extends CustomPainter {
  const SpectrumPainter({
    required this.bars,
    required this.peaks,
    required this.materials,
    required this.phosphor,
    required this.phosphorHot,
  });

  final List<double> bars;
  final List<double> peaks;
  final LookMaterials materials;
  final Color phosphor;
  final Color phosphorHot;

  static const _barWidth = 9.0;
  static const _gap = 3.0;

  @override
  void paint(Canvas canvas, Size size) {
    if (bars.isEmpty) return;
    final stops = materials.spectrumStops;
    if (stops.isEmpty) return;

    for (var i = 0; i < bars.length; i++) {
      final left = i * (_barWidth + _gap);
      if (left + _barWidth > size.width) break;
      final h = (size.height * bars[i].clamp(0.0, 1.0)).clamp(0.0, size.height);
      final peakH =
          (size.height * peaks[i].clamp(0.0, 1.0)).clamp(0.0, size.height);
      final barRect = Rect.fromLTWH(left, size.height - h, _barWidth, h);
      if (h > 0) {
        canvas.drawRect(
          barRect,
          Paint()
            ..shader = LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: stops,
              stops: stops.length == 4
                  ? const [0, 0.26, 0.62, 1]
                  : null,
            ).createShader(barRect),
        );
        canvas.drawRect(
          barRect,
          Paint()
            ..color = phosphor.withValues(alpha: 0x66 / 255)
            ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3.5),
        );
      }
      if (peakH > 2) {
        final cap = Rect.fromLTWH(left, size.height - peakH, _barWidth, 2);
        canvas.drawRect(
          cap,
          Paint()
            ..color = Color.lerp(const Color(0xFFFFFFFF), phosphorHot, 0.2)!,
        );
        canvas.drawRect(
          cap,
          Paint()
            ..color = phosphor.withValues(alpha: 0xE6 / 255)
            ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
        );
      }
    }
  }

  @override
  bool shouldRepaint(SpectrumPainter old) =>
      !_same(old.bars, bars) ||
      !_same(old.peaks, peaks) ||
      old.materials != materials ||
      old.phosphor != phosphor ||
      old.phosphorHot != phosphorHot;

  static bool _same(List<double> a, List<double> b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
