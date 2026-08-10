import 'dart:async';
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';

import '../../../look/look_materials.dart';
import '../../../look/look_palette.dart';
import '../../../theme/look_scope.dart';

/// Window chassis matching mockup `.win`, plus `.rivet` / `.plate` / `.rail`.
class MockupShell extends StatelessWidget {
  const MockupShell({
    super.key,
    required this.child,
    this.width = 825,
    this.borderRadius = 6,
  });

  final Widget child;
  final double width;
  final double borderRadius;

  /// Prefetch the `.win::before` noise tile (call from golden `setUpAll`).
  static Future<void> ensureNoiseReady() async {
    await _NoiseTile.ensure();
  }

  @override
  Widget build(BuildContext context) {
    // Material + explicit no-decoration DefaultTextStyle: without a Material
    // ancestor Flutter paints yellow debug underlines on every Text — that
    // alone destroyed mockup parity in goldens and the live multi-window UI.
    final content = DefaultTextStyle(
      style: const TextStyle(
        decoration: TextDecoration.none,
        color: MockupTokens.ink,
        fontFamily: 'TrampCondensed',
        fontWeight: FontWeight.w700,
      ),
      child: Material(
        type: MaterialType.transparency,
        child: child,
      ),
    );

    return SizedBox(
      width: width,
      child: DecoratedBox(
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(borderRadius),
          boxShadow: const [
            BoxShadow(
              color: Color(0x80000000),
              offset: Offset(0, 2),
              blurRadius: 3,
            ),
            BoxShadow(
              color: Color(0xCC000000),
              offset: Offset(0, 18),
              blurRadius: 34,
              spreadRadius: -14,
            ),
          ],
        ),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(borderRadius),
          child: Stack(
            children: [
              Positioned.fill(
                child: CustomPaint(
                  painter: _ShellPainter(
                    palette: LookScope.of(context).palette,
                    materials: LookScope.of(context).materials,
                  ),
                ),
              ),
              // `.win::before` — inset ~1px noise overlay at ~5% / overlay blend.
              Positioned.fill(
                child: _MockupWinNoise(borderRadius: borderRadius),
              ),
              content,
            ],
          ),
        ),
      ),
    );
  }
}

/// Approximates mockup `--noise` (SVG `feTurbulence` fractalNoise 140×140 tile).
final class _NoiseTile {
  static const int size = 140;

  static ui.Image? image;
  static Future<ui.Image>? _pending;

  static Future<ui.Image> ensure() {
    final existing = image;
    if (existing != null) return Future<ui.Image>.value(existing);
    return _pending ??= _generate();
  }

  static Future<ui.Image> _generate() async {
    const w = size;
    const h = size;
    final pixels = Uint8List(w * h * 4);
    for (var y = 0; y < h; y++) {
      for (var x = 0; x < w; x++) {
        // baseFrequency≈0.9, numOctaves=3 — fine grain grayscale.
        final n = _fractalNoise(x, y);
        final v = (n * 255.0).round().clamp(0, 255);
        final i = (y * w + x) * 4;
        pixels[i] = v;
        pixels[i + 1] = v;
        pixels[i + 2] = v;
        pixels[i + 3] = 255;
      }
    }
    final completer = Completer<ui.Image>();
    ui.decodeImageFromPixels(
      pixels,
      w,
      h,
      ui.PixelFormat.rgba8888,
      completer.complete,
    );
    final decoded = await completer.future;
    image = decoded;
    return decoded;
  }

  /// Deterministic hash in `0..1`.
  static double _hash(int x, int y) {
    var n = (x * 374761393) ^ (y * 668265263) ^ (x * y * 1274126177);
    n = (n ^ (n >> 13)) * 1274126177;
    n = (n ^ (n >> 16)) & 0x7fffffff;
    return n / 0x7fffffff;
  }

  static double _fractalNoise(int x, int y) {
    var sum = 0.0;
    var amp = 1.0;
    var norm = 0.0;
    var xo = x;
    var yo = y;
    for (var octave = 0; octave < 3; octave++) {
      sum += amp * _hash(xo, yo);
      norm += amp;
      amp *= 0.5;
      // Next octave: higher frequency scramble (≈×2 baseFrequency).
      xo = xo * 2 + 17;
      yo = yo * 2 + 31;
    }
    return sum / norm;
  }
}

/// `.win::before` noise: inset 1px, radius shell−1, opacity 0.05, overlay blend.
class _MockupWinNoise extends StatefulWidget {
  const _MockupWinNoise({required this.borderRadius});

  final double borderRadius;

  @override
  State<_MockupWinNoise> createState() => _MockupWinNoiseState();
}

class _MockupWinNoiseState extends State<_MockupWinNoise> {
  ui.Image? _tile;

  @override
  void initState() {
    super.initState();
    final cached = _NoiseTile.image;
    if (cached != null) {
      _tile = cached;
      return;
    }
    _NoiseTile.ensure().then((img) {
      if (mounted) setState(() => _tile = img);
    });
  }

  @override
  Widget build(BuildContext context) {
    final tile = _tile;
    if (tile == null) return const SizedBox.shrink();
    final innerRadius =
        (widget.borderRadius - 1).clamp(0.0, widget.borderRadius);
    return Padding(
      padding: const EdgeInsets.all(1),
      child: IgnorePointer(
        child: ClipRRect(
          borderRadius: BorderRadius.circular(innerRadius),
          child: CustomPaint(
            painter: _NoiseOverlayPainter(tile),
            child: const SizedBox.expand(),
          ),
        ),
      ),
    );
  }
}

class _NoiseOverlayPainter extends CustomPainter {
  const _NoiseOverlayPainter(this.tile);

  final ui.Image tile;

  @override
  void paint(Canvas canvas, Size size) {
    if (size.isEmpty) return;
    final bounds = Offset.zero & size;
    // Opacity 0.05 + mix-blend-mode: overlay (CSS `.win::before`).
    canvas.saveLayer(
      bounds,
      Paint()
        ..blendMode = BlendMode.overlay
        ..color = const Color.fromRGBO(255, 255, 255, 0.05),
    );
    final shader = ImageShader(
      tile,
      TileMode.repeated,
      TileMode.repeated,
      Matrix4.identity().storage,
    );
    canvas.drawRect(bounds, Paint()..shader = shader);
    canvas.restore();
  }

  @override
  bool shouldRepaint(covariant _NoiseOverlayPainter oldDelegate) =>
      oldDelegate.tile != tile;
}

/// Corner rivet matching mockup `.rivet` (7×7).
class MockupRivet extends StatelessWidget {
  const MockupRivet({super.key, this.size = 7});

  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: const _RivetPainter(),
    );
  }
}

/// Brushed metal plate matching mockup `.plate`.
class MockupPlate extends StatelessWidget {
  const MockupPlate({
    super.key,
    this.child,
    this.borderRadius = 4,
  });

  final Widget? child;
  final double borderRadius;

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(borderRadius),
      child: Stack(
        fit: StackFit.passthrough,
        children: [
          const Positioned.fill(
            child: ColoredBox(color: Color(0xFF1E222C)),
          ),
          const Positioned.fill(
            child: CustomPaint(painter: _BrushPainter(opacity: 1)),
          ),
          const Positioned.fill(
            child: DecoratedBox(
              decoration: BoxDecoration(
                border: Border(
                  top: BorderSide(color: Color(0x1AE2ECFF)),
                  bottom: BorderSide(color: Color(0xB3000000)),
                ),
              ),
            ),
          ),
          if (child != null) child!,
        ],
      ),
    );
  }
}

/// Slack filler matching mockup `.rail`.
class MockupRail extends StatelessWidget {
  const MockupRail({
    super.key,
    this.height = 22,
    this.minWidth = 24,
  });

  final double height;
  final double minWidth;

  @override
  Widget build(BuildContext context) {
    return ConstrainedBox(
      constraints: BoxConstraints(minWidth: minWidth, minHeight: height),
      child: SizedBox(
        height: height,
        child: Opacity(
          opacity: 0.9,
          child: ClipRRect(
            borderRadius: BorderRadius.circular(3),
            child: const Stack(
              fit: StackFit.expand,
              children: [
                ColoredBox(color: Color(0xFF1E222C)),
                CustomPaint(painter: _BrushPainter(opacity: 1)),
                DecoratedBox(
                  decoration: BoxDecoration(
                    border: Border(
                      top: BorderSide(color: Color(0x14E2ECFF)),
                      bottom: BorderSide(color: Color(0x99000000)),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _ShellPainter extends CustomPainter {
  const _ShellPainter({required this.palette, required this.materials});

  final LookPalette palette;
  final LookMaterials materials;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final rrect = RRect.fromRectAndRadius(rect, const Radius.circular(6));
    canvas.drawRRect(
      rrect,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            palette.shellHighlight,
            palette.shellBase,
            palette.shellMid,
            palette.shellLow,
            palette.shellDeep,
          ],
          stops: const [0, 0.03, 0.46, 0.92, 1],
        ).createShader(rect),
    );

    // Inset bevels from mockup box-shadow stack (--bevel-light / --bevel-soft).
    // Pack alpha as 8-bit to match prior Color(0x26E2ECFF) / Color(0x0FE2ECFF).
    Color bevelInk(double opacity) {
      final a = (opacity * 255.0).round().clamp(0, 255);
      return Color((a << 24) | 0x00E2ECFF);
    }

    canvas.drawRRect(
      rrect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = bevelInk(materials.bevelLightOpacity),
    );
    canvas.drawLine(
      const Offset(1, 1),
      Offset(1, size.height - 1),
      Paint()..color = bevelInk(materials.bevelSoftOpacity),
    );
    canvas.drawLine(
      Offset(size.width - 1, 1),
      Offset(size.width - 1, size.height - 1),
      Paint()..color = const Color(0x8C000000),
    );
    canvas.drawLine(
      Offset(1, size.height - 1),
      Offset(size.width - 1, size.height - 1),
      Paint()..color = const Color(0xE6000000),
    );
  }

  @override
  bool shouldRepaint(covariant _ShellPainter oldDelegate) =>
      palette != oldDelegate.palette || materials != oldDelegate.materials;
}

class _RivetPainter extends CustomPainter {
  const _RivetPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide / 2;
    canvas.drawCircle(
      Offset(center.dx, center.dy + 0.5),
      radius,
      Paint()..color = const Color(0x1FE2ECFF),
    );
    canvas.drawCircle(
      center,
      radius,
      Paint()
        ..shader = RadialGradient(
          center: const Alignment(-0.3, -0.4),
          colors: const [
            Color(0xFF5C6373),
            Color(0xFF262B33),
            Color(0xFF101218),
          ],
          stops: const [0, 0.6, 1],
        ).createShader(Rect.fromCircle(center: center, radius: radius)),
    );
    canvas.drawCircle(
      center,
      radius,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.8
        ..color = const Color(0xCC000000),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

/// Mockup `--brush` repeating horizontal grain.
class _BrushPainter extends CustomPainter {
  const _BrushPainter({required this.opacity});

  final double opacity;

  @override
  void paint(Canvas canvas, Size size) {
    final a = Paint()..color = Color.fromRGBO(226, 236, 255, 0.045 * opacity);
    final b = Paint()..color = Color.fromRGBO(0, 0, 0, 0.10 * opacity);
    final c = Paint()..color = Color.fromRGBO(226, 236, 255, 0.015 * opacity);
    for (var y = 0.0; y < size.height; y += 3) {
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, 1), a);
      canvas.drawRect(Rect.fromLTWH(0, y + 1, size.width, 1), b);
      canvas.drawRect(Rect.fromLTWH(0, y + 2, size.width, 1), c);
    }
  }

  @override
  bool shouldRepaint(covariant _BrushPainter oldDelegate) =>
      opacity != oldDelegate.opacity;
}
