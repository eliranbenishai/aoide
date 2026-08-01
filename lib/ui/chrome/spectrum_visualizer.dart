import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

/// Vector LCD spectrum bars — pulse animation while playing, idle when stopped.
class SpectrumVisualizer extends StatefulWidget {
  const SpectrumVisualizer({
    super.key,
    required this.playing,
    this.volume = 1,
  });

  final bool playing;
  final double volume;

  @override
  State<SpectrumVisualizer> createState() => _SpectrumVisualizerState();
}

class _SpectrumVisualizerState extends State<SpectrumVisualizer>
    with SingleTickerProviderStateMixin {
  static const _barCount = 20;

  late final AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1200),
    );
    _syncAnimation();
  }

  @override
  void didUpdateWidget(SpectrumVisualizer oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.playing != widget.playing) {
      _syncAnimation();
    }
  }

  void _syncAnimation() {
    if (widget.playing) {
      _controller.repeat();
    } else {
      _controller.stop();
      _controller.value = 0;
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _controller,
      builder: (context, _) {
        return CustomPaint(
          painter: _SpectrumPainter(
            phase: widget.playing ? _controller.value * 2 * math.pi : 0,
            volume: widget.volume.clamp(0.0, 1.0),
            playing: widget.playing,
          ),
        );
      },
    );
  }
}

class _SpectrumPainter extends CustomPainter {
  _SpectrumPainter({
    required this.phase,
    required this.volume,
    required this.playing,
  });

  final double phase;
  final double volume;
  final bool playing;

  static const _barCount = _SpectrumVisualizerState._barCount;
  static const _minBarFraction = 0.08;
  static const _idleBarFraction = 0.04;

  @override
  void paint(Canvas canvas, Size size) {
    if (size.width <= 0 || size.height <= 0) return;

    final barGap = 1.0;
    final totalGap = barGap * (_barCount - 1);
    final barWidth = ((size.width - totalGap) / _barCount).clamp(1.0, size.width);

    for (var i = 0; i < _barCount; i++) {
      final normalized = playing
          ? (math.sin(phase + i * 0.55) + 1) / 2
          : _idleBarFraction / _minBarFraction;
      final heightFraction =
          (_minBarFraction + normalized * (1 - _minBarFraction)) * volume;
      final barHeight = size.height * heightFraction.clamp(0.0, 1.0);
      final x = i * (barWidth + barGap);
      final y = size.height - barHeight;

      final isPeak = normalized > 0.82;
      final paint = Paint()
        ..color = isPeak ? TrampColors.lcdPeak : TrampColors.lcdPhosphor;

      canvas.drawRect(
        Rect.fromLTWH(x, y, barWidth, barHeight),
        paint,
      );
    }
  }

  @override
  bool shouldRepaint(_SpectrumPainter oldDelegate) {
    return oldDelegate.phase != phase ||
        oldDelegate.volume != volume ||
        oldDelegate.playing != playing;
  }
}
