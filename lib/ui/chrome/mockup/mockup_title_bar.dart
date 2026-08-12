import 'package:flutter/widgets.dart';

import '../logo.dart';
import 'mockup_hover.dart';
import 'mockup_icons.dart';
import '../../../look/look_materials.dart';
import '../../../look/look_palette.dart';
import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';

/// Title strip matching mockup `.tbar` (42px).
class MockupTitleBar extends StatelessWidget {
  const MockupTitleBar({
    super.key,
    required this.windowName,
    this.wordmarkSize = 24,
    this.showZoom = true,
    this.showBrand = true,
    this.onMinimize,
    this.onCollapse,
    this.onZoomOut,
    this.onZoomIn,
    this.onClose,
    this.wrapDragRegion,
  });

  final String windowName;
  final double wordmarkSize;
  final bool showZoom;

  /// When false (EQ / playlist), omit logo + TRAMP wordmark — role title only.
  final bool showBrand;
  final VoidCallback? onMinimize;

  /// When set (EQ / playlist), shows Collapse (shade) instead of Minimize.
  final VoidCallback? onCollapse;
  final VoidCallback? onZoomOut;
  final VoidCallback? onZoomIn;
  final VoidCallback? onClose;

  /// Wraps the draggable strip (brand / grips / title) only — never the
  /// window buttons, so dock drag cannot steal minimize / zoom / close taps.
  final Widget Function(Widget dragRegion)? wrapDragRegion;

  static const height = 42.0;

  static TextStyle _windowNameStyle(BuildContext context) {
    final look = LookScope.of(context);
    return TextStyle(
      fontFamily: look.chromeFamily,
      fontWeight: FontWeight.w700,
      fontSize: 13,
      height: 1,
      letterSpacing: 13 * 0.26,
      decoration: TextDecoration.none,
      color: LookPaint.windowName(look.palette),
      shadows: const [
        Shadow(
          offset: Offset(0, 1),
          color: Color(0xB3000000),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    final palette = LookScope.of(context).palette;
    return SizedBox(
      height: height,
      child: ClipRRect(
        borderRadius: const BorderRadius.vertical(top: Radius.circular(5)),
        child: Stack(
          children: [
            Positioned.fill(
              child: CustomPaint(
                painter: _TitleBarPainter(palette: palette),
              ),
            ),
            // Fill the strip; center like `.tbar { align-items: center }`.
            // A loose Stack child sizes the Row to logo/buttons and pins top.
            Positioned.fill(
              child: Padding(
                padding: const EdgeInsets.fromLTRB(10, 0, 9, 0),
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    Expanded(
                      child: _wrapDrag(
                        Row(
                          children: [
                            if (showBrand) ...[
                              _TitleLogo(accent: palette.accentDefault),
                              const SizedBox(width: 12),
                              _Wordmark(size: wordmarkSize),
                              const SizedBox(width: 12),
                            ],
                            const Expanded(child: _Grip()),
                            const SizedBox(width: 12),
                            Text(
                              windowName.toUpperCase(),
                              style: _windowNameStyle(context),
                            ),
                            const SizedBox(width: 12),
                            const Expanded(child: _Grip()),
                          ],
                        ),
                      ),
                    ),
                    const SizedBox(width: 12),
                    _WindowButtons(
                      showZoom: showZoom,
                      onMinimize: onMinimize,
                      onCollapse: onCollapse,
                      onZoomOut: onZoomOut,
                      onZoomIn: onZoomIn,
                      onClose: onClose,
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _wrapDrag(Widget region) {
    final wrap = wrapDragRegion;
    if (wrap == null) return region;
    return wrap(region);
  }
}

class _TitleLogo extends StatelessWidget {
  const _TitleLogo({required this.accent});

  final Color accent;

  @override
  Widget build(BuildContext context) {
    // Mockup `.tbar-logo`: 30×30 circle, logo at `center / 112%`, ring + pink bloom.
    return SizedBox(
      width: 30,
      height: 30,
      child: Stack(
        clipBehavior: Clip.none,
        alignment: Alignment.center,
        children: [
          // `0 0 12px rgba(accent, 0.28)` — outside the face.
          Container(
            width: 30,
            height: 30,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: accent.withValues(alpha: 0x47 / 255),
                  blurRadius: 12,
                ),
                const BoxShadow(
                  color: Color(0x8C000000),
                  offset: Offset(0, 2),
                  blurRadius: 4,
                ),
              ],
            ),
          ),
          ClipOval(
            child: ColoredBox(
              color: const Color(0xFFE9ECF4),
              child: Transform.scale(
                scale: 1.12,
                child: const TrampLogo(size: 30),
              ),
            ),
          ),
          // `0 0 0 1px rgba(0, 0, 0, 0.65)`
          IgnorePointer(
            child: Container(
              width: 30,
              height: 30,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: const Color(0xA6000000), width: 1),
              ),
            ),
          ),
          // Inset highlights: top sheen + bottom shade + CSS ::after gloss.
          const IgnorePointer(
            child: CustomPaint(
              size: Size.square(30),
              painter: _TitleLogoInsetPainter(),
            ),
          ),
        ],
      ),
    );
  }
}

class _TitleLogoInsetPainter extends CustomPainter {
  const _TitleLogoInsetPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final rrect = RRect.fromRectAndRadius(rect, Radius.circular(size.width / 2));
    canvas.save();
    canvas.clipRRect(rrect);
    // Soft top inset highlight — clipped inside the disc (no outer white ring).
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height * 0.55),
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [Color(0x80FFFFFF), Color(0x00FFFFFF)],
        ).createShader(rect),
    );
    // Bottom shade: inset 0 -3px 6px rgba(20, 34, 66, 0.35)
    canvas.drawRect(
      Rect.fromLTWH(0, size.height * 0.45, size.width, size.height * 0.55),
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [Color(0x00142242), Color(0x59142242)],
        ).createShader(Rect.fromLTWH(0, size.height * 0.45, size.width, size.height * 0.55)),
    );
    // ::after gloss wedge
    canvas.drawRect(
      rect,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment(-0.6, -0.8),
          end: Alignment(0.4, 0.6),
          colors: [Color(0x73FFFFFF), Color(0x00FFFFFF)],
          stops: [0, 0.46],
        ).createShader(rect),
    );
    canvas.restore();
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

/// The TRAMP wordmark. No version rides along with it — the About window's
/// readout is where the version belongs.
class _Wordmark extends StatelessWidget {
  const _Wordmark({required this.size});

  final double size;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return Text(
      'TRAMP',
      style: TextStyle(
        fontFamily: look.chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: size,
        height: 1,
        letterSpacing: size * 0.2,
        decoration: TextDecoration.none,
        color: LookPaint.wordmark(palette),
        shadows: [
          Shadow(offset: const Offset(0, -1), color: LookPaint.hoverLiftTarget(palette).withValues(alpha: 0x4D / 255)),
          const Shadow(offset: Offset(0, 1), color: Color(0xD9000000)),
          // CSS `0 0 14px` phosphor — Skia blur reads hotter.
          Shadow(color: palette.phosphorDefault.withValues(alpha: 0x4D / 255), blurRadius: 5),
        ],
      ),
    );
  }
}

class _Grip extends StatelessWidget {
  const _Grip();

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    return SizedBox(
      height: 8,
      width: double.infinity,
      child: CustomPaint(
        painter: _GripPainter(
          materials: look.materials,
          palette: look.palette,
        ),
      ),
    );
  }
}

class _GripPainter extends CustomPainter {
  const _GripPainter({
    required this.materials,
    required this.palette,
  });

  final LookMaterials materials;
  final LookPalette palette;

  @override
  void paint(Canvas canvas, Size size) {
    final rail = Rect.fromLTWH(0, 2, size.width, 2);
    final bloom = LookPaint.phosphorBloom(palette);
    // Soft bloom behind the 2px rail (CSS `box-shadow: 0 0 7px`).
    canvas.drawRect(
      rail,
      Paint()
        ..shader = LinearGradient(
          colors: [
            const Color(0x00000000),
            bloom,
            bloom,
            const Color(0x00000000),
          ],
          stops: const [0, 0.12, 0.88, 1],
        ).createShader(rail)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3.5),
    );
    canvas.drawRect(
      rail,
      Paint()
        ..shader = LinearGradient(
          colors: [
            const Color(0x00000000),
            if (materials.railStops.length >= 3) ...[
              materials.railStops[0],
              materials.railStops[1],
              materials.railStops[2],
            ] else ...[
              palette.phosphorDim,
              palette.accentDim,
              palette.phosphorDim,
            ],
            const Color(0x00000000),
          ],
          stops: const [0, 0.12, 0.5, 0.88, 1],
        ).createShader(rail),
    );
    final under = Rect.fromLTWH(0, 6, size.width, 1);
    final underGlow = LookPaint.accentBloom(palette);
    canvas.drawRect(
      under,
      Paint()
        ..shader = LinearGradient(
          colors: [
            const Color(0x00000000),
            underGlow,
            const Color(0x00000000),
          ],
        ).createShader(under),
    );
  }

  @override
  bool shouldRepaint(covariant _GripPainter oldDelegate) =>
      materials != oldDelegate.materials || palette != oldDelegate.palette;
}

class _WindowButtons extends StatelessWidget {
  const _WindowButtons({
    required this.showZoom,
    this.onMinimize,
    this.onCollapse,
    this.onZoomOut,
    this.onZoomIn,
    this.onClose,
  });

  final bool showZoom;
  final VoidCallback? onMinimize;
  final VoidCallback? onCollapse;
  final VoidCallback? onZoomOut;
  final VoidCallback? onZoomIn;
  final VoidCallback? onClose;

  @override
  Widget build(BuildContext context) {
    final palette = LookScope.of(context).palette;
    final glyph = LookPaint.glyphInk(palette);
    final closeGlyph = LookPaint.closeGlyphInk(palette);
    final collapse = onCollapse != null;
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        _WinBtn(
          onPressed: collapse ? onCollapse : onMinimize,
          semanticLabel: collapse ? 'Collapse' : 'Minimize',
          child: MockupIcons.minimize(color: glyph),
        ),
        if (showZoom) ...[
          const SizedBox(width: 5),
          _WinBtn(
            onPressed: onZoomOut,
            semanticLabel: 'Zoom out',
            child: MockupIcons.zoomOut(color: glyph),
          ),
          const SizedBox(width: 5),
          _WinBtn(
            onPressed: onZoomIn,
            semanticLabel: 'Zoom in',
            child: MockupIcons.zoomIn(color: glyph),
          ),
        ],
        const SizedBox(width: 5),
        _WinBtn(
          onPressed: onClose,
          semanticLabel: 'Close',
          close: true,
          child: MockupIcons.close(color: closeGlyph),
        ),
      ],
    );
  }
}

class _WinBtn extends StatefulWidget {
  const _WinBtn({
    required this.child,
    this.onPressed,
    this.semanticLabel,
    this.close = false,
  });

  final Widget child;
  final VoidCallback? onPressed;
  final String? semanticLabel;
  final bool close;

  @override
  State<_WinBtn> createState() => _WinBtnState();
}

class _WinBtnState extends State<_WinBtn> {
  bool _down = false;

  bool get _enabled => widget.onPressed != null;

  void _setDown(bool value) {
    if (value && !_enabled) return;
    if (_down == value) return;
    setState(() => _down = value);
  }

  @override
  void didUpdateWidget(covariant _WinBtn oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!_enabled && _down) {
      _down = false;
    }
  }

  @override
  Widget build(BuildContext context) {
    final palette = LookScope.of(context).palette;
    final lift = LookPaint.hoverLiftTarget(palette);
    return Semantics(
      button: true,
      enabled: _enabled,
      label: widget.semanticLabel,
      child: MockupHover(
        enabled: _enabled,
        builder: (context, hover) {
          Widget face = CustomPaint(
            painter: _WinBtnPainter(
              close: widget.close,
              pressed: _down,
              hover: hover,
              palette: palette,
              liftTarget: lift,
            ),
            child: SizedBox(
              width: 26,
              height: 22,
              child: Center(
                child: MockupGlyphGlow(amount: hover, child: widget.child),
              ),
            ),
          );
          if (!_enabled) {
            face = Opacity(
              opacity: MockupHoverTokens.disabledOpacity,
              child: face,
            );
          }
          return GestureDetector(
            onTapDown: (_) => _setDown(true),
            onTapUp: (_) => _setDown(false),
            onTapCancel: () => _setDown(false),
            onTap: widget.onPressed,
            child: face,
          );
        },
      ),
    );
  }
}

class _TitleBarPainter extends CustomPainter {
  const _TitleBarPainter({required this.palette});

  final LookPalette palette;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final stops = LookPaint.titleBarStops(palette);
    final lift = LookPaint.hoverLiftTarget(palette);
    final cool = LookPaint.coolSheen(palette);
    canvas.drawRect(
      rect,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: stops,
          stops: const [0, 0.26, 0.62, 1],
        ).createShader(rect),
    );
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height * 0.5),
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            lift.withValues(alpha: 0x1F / 255),
            lift.withValues(alpha: 0),
          ],
        ).createShader(Rect.fromLTWH(0, 0, size.width, size.height * 0.5)),
    );
    canvas.drawLine(
      const Offset(0, 0.5),
      Offset(size.width, 0.5),
      Paint()..color = cool.withValues(alpha: 0x38 / 255),
    );
    canvas.drawLine(
      Offset(0, size.height - 0.5),
      Offset(size.width, size.height - 0.5),
      Paint()..color = const Color(0xBF000000),
    );
  }

  @override
  bool shouldRepaint(covariant _TitleBarPainter oldDelegate) =>
      palette != oldDelegate.palette;
}

class _WinBtnPainter extends CustomPainter {
  const _WinBtnPainter({
    required this.close,
    required this.pressed,
    required this.hover,
    required this.palette,
    required this.liftTarget,
  });

  final bool close;
  final bool pressed;
  final double hover;
  final LookPalette palette;
  final Color liftTarget;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final rrect = RRect.fromRectAndRadius(rect, const Radius.circular(3));
    final base = close
        ? (pressed
            ? LookPaint.winBtnClosePressed(palette)
            : LookPaint.winBtnCloseIdle(palette))
        : (pressed
            ? LookPaint.winBtnPressed(palette)
            : LookPaint.winBtnIdle(palette));
    final colors = pressed
        ? base
        : base
            .map((c) => mockupHoverLift(c, hover, MockupHoverTokens.faceLift, liftTarget))
            .toList(growable: false);

    // Outer drop: mockup `0 1px 2px rgba(0, 0, 0, 0.55)`.
    canvas.drawRRect(
      rrect.shift(const Offset(0, 1)),
      Paint()
        ..color = const Color(0x8C000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 1.2),
    );

    canvas.drawRRect(
      rrect,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: colors,
          stops: const [0, 0.55, 1],
        ).createShader(rect),
    );

    canvas.save();
    canvas.clipRRect(rrect);
    // Inset top lip: `inset 0 1px 0 rgba(232, 240, 255, 0.3)`.
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, 1),
      Paint()
        ..color = pressed
            ? liftTarget.withValues(alpha: 0x26 / 255)
            : Color.lerp(
                liftTarget.withValues(alpha: 0x4D / 255),
                liftTarget.withValues(alpha: 0x80 / 255),
                hover,
              )!,
    );
    // Inset bottom lip: `inset 0 -1px 0 rgba(0, 0, 0, 0.6)`.
    canvas.drawRect(
      Rect.fromLTWH(0, size.height - 1, size.width, 1),
      Paint()..color = const Color(0x99000000),
    );
    canvas.restore();
  }

  @override
  bool shouldRepaint(covariant _WinBtnPainter oldDelegate) =>
      close != oldDelegate.close ||
      pressed != oldDelegate.pressed ||
      hover != oldDelegate.hover ||
      palette != oldDelegate.palette ||
      liftTarget != oldDelegate.liftTarget;
}
