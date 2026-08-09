import 'package:flutter/widgets.dart';

import '../../../theme/mockup_tokens.dart';
import '../logo.dart';
import 'mockup_icons.dart';

/// Title strip matching mockup `.tbar` (42px).
class MockupTitleBar extends StatelessWidget {
  const MockupTitleBar({
    super.key,
    required this.windowName,
    this.wordmarkSize = 24,
    this.showVersion = true,
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
  final bool showVersion;
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

  static const _windowNameStyle = TextStyle(
    fontFamily: 'TrampCondensed',
    fontWeight: FontWeight.w700,
    fontSize: 13,
    height: 1,
    letterSpacing: 13 * 0.26,
    decoration: TextDecoration.none,
    color: Color(0x8CC8D6EB),
    shadows: [
      Shadow(
        offset: Offset(0, 1),
        color: Color(0xB3000000),
      ),
    ],
  );

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: height,
      child: ClipRRect(
        borderRadius: const BorderRadius.vertical(top: Radius.circular(5)),
        child: Stack(
          children: [
            const Positioned.fill(child: CustomPaint(painter: _TitleBarPainter())),
            Padding(
              // Optical center sits slightly below geometric mid (top bevel reads
              // heavier). Without the 30px brand mark (EQ / PL), condensed caps
              // sit high in the em-box — use a stronger downward nudge.
              padding: EdgeInsets.fromLTRB(10, showBrand ? 3 : 5, 9, showBrand ? 1 : 0),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.center,
                children: [
                  Expanded(
                    child: _wrapDrag(
                      Row(
                        children: [
                          if (showBrand) ...[
                            const _TitleLogo(),
                            const SizedBox(width: 12),
                            _Wordmark(
                              size: wordmarkSize,
                              showVersion: showVersion,
                            ),
                            const SizedBox(width: 12),
                          ],
                          const Expanded(child: _Grip()),
                          const SizedBox(width: 12),
                          Text(
                            windowName.toUpperCase(),
                            style: _windowNameStyle,
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
  const _TitleLogo();

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
          // `0 0 12px rgba(255, 61, 154, 0.28)` — outside the face.
          Container(
            width: 30,
            height: 30,
            decoration: const BoxDecoration(
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(color: Color(0x47FF3D9A), blurRadius: 12),
                BoxShadow(
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
        ..shader = const LinearGradient(
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

class _Wordmark extends StatelessWidget {
  const _Wordmark({required this.size, required this.showVersion});

  final double size;
  final bool showVersion;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          'TRAMP',
          style: TextStyle(
            fontFamily: 'TrampCondensed',
            fontWeight: FontWeight.w700,
            fontSize: size,
            height: 1,
            letterSpacing: size * 0.2,
            decoration: TextDecoration.none,
            color: const Color(0xFFEAF2FF),
            shadows: const [
              Shadow(offset: Offset(0, -1), color: Color(0x4DE2ECFF)),
              Shadow(offset: Offset(0, 1), color: Color(0xD9000000)),
              // CSS `0 0 14px rgba(61,231,255,0.3)` — Skia blur reads hotter.
              Shadow(color: Color(0x4D3DE7FF), blurRadius: 5),
            ],
          ),
        ),
        if (showVersion)
          Padding(
            padding: const EdgeInsets.only(left: 4, top: 2),
            child: Text(
              '1.0',
              style: TextStyle(
                fontFamily: 'TrampCondensed',
                fontWeight: FontWeight.w700,
                fontSize: 11,
                height: 1,
                letterSpacing: 11 * 0.06,
                decoration: TextDecoration.none,
                color: MockupTokens.phos.withValues(alpha: 0.85),
                shadows: const [
                  Shadow(color: Color(0x803DE7FF), blurRadius: 8),
                ],
              ),
            ),
          ),
      ],
    );
  }
}

class _Grip extends StatelessWidget {
  const _Grip();

  @override
  Widget build(BuildContext context) {
    return const SizedBox(
      height: 8,
      width: double.infinity,
      child: CustomPaint(painter: _GripPainter()),
    );
  }
}

class _GripPainter extends CustomPainter {
  const _GripPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rail = Rect.fromLTWH(0, 2, size.width, 2);
    // Soft bloom behind the 2px rail (CSS `box-shadow: 0 0 7px`).
    canvas.drawRect(
      rail,
      Paint()
        ..shader = const LinearGradient(
          colors: [
            Color(0x00000000),
            Color(0x4D3DE7FF),
            Color(0x4D3DE7FF),
            Color(0x00000000),
          ],
          stops: [0, 0.12, 0.88, 1],
        ).createShader(rail)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3.5),
    );
    canvas.drawRect(
      rail,
      Paint()
        ..shader = const LinearGradient(
          colors: [
            Color(0x00000000),
            MockupTokens.phosDim,
            MockupTokens.accentDim,
            MockupTokens.phosDim,
            Color(0x00000000),
          ],
          stops: [0, 0.12, 0.5, 0.88, 1],
        ).createShader(rail),
    );
    final under = Rect.fromLTWH(0, 6, size.width, 1);
    canvas.drawRect(
      under,
      Paint()
        ..shader = const LinearGradient(
          colors: [
            Color(0x00000000),
            Color(0x59FF3D9A),
            Color(0x00000000),
          ],
        ).createShader(under),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
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
    final collapse = onCollapse != null;
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        _WinBtn(
          onPressed: collapse ? onCollapse : onMinimize,
          semanticLabel: collapse ? 'Collapse' : 'Minimize',
          child: MockupIcons.minimize(),
        ),
        if (showZoom) ...[
          const SizedBox(width: 5),
          _WinBtn(
            onPressed: onZoomOut,
            semanticLabel: 'Zoom out',
            child: MockupIcons.zoomOut(),
          ),
          const SizedBox(width: 5),
          _WinBtn(
            onPressed: onZoomIn,
            semanticLabel: 'Zoom in',
            child: MockupIcons.zoomIn(),
          ),
        ],
        const SizedBox(width: 5),
        _WinBtn(
          onPressed: onClose,
          semanticLabel: 'Close',
          close: true,
          child: MockupIcons.close(),
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

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      label: widget.semanticLabel,
      child: GestureDetector(
        onTapDown: (_) => setState(() => _down = true),
        onTapUp: (_) => setState(() => _down = false),
        onTapCancel: () => setState(() => _down = false),
        onTap: widget.onPressed,
        child: CustomPaint(
          painter: _WinBtnPainter(close: widget.close, pressed: _down),
          child: SizedBox(
            width: 26,
            height: 22,
            child: Center(child: widget.child),
          ),
        ),
      ),
    );
  }
}

class _TitleBarPainter extends CustomPainter {
  const _TitleBarPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    canvas.drawRect(
      rect,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0xFF3C4356),
            Color(0xFF2C3241),
            Color(0xFF1D222C),
            Color(0xFF12151C),
          ],
          stops: [0, 0.26, 0.62, 1],
        ).createShader(rect),
    );
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height * 0.5),
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            const Color(0x1FE8F0FF),
            const Color(0x00E8F0FF),
          ],
        ).createShader(Rect.fromLTWH(0, 0, size.width, size.height * 0.5)),
    );
    canvas.drawLine(
      const Offset(0, 0.5),
      Offset(size.width, 0.5),
      Paint()..color = const Color(0x38E2ECFF),
    );
    canvas.drawLine(
      Offset(0, size.height - 0.5),
      Offset(size.width, size.height - 0.5),
      Paint()..color = const Color(0xBF000000),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

class _WinBtnPainter extends CustomPainter {
  const _WinBtnPainter({required this.close, required this.pressed});

  final bool close;
  final bool pressed;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final rrect = RRect.fromRectAndRadius(rect, const Radius.circular(3));
    final colors = close
        ? (pressed
            ? const [Color(0xFF79204A), Color(0xFF4A1129), Color(0xFF2E0A1A)]
            : const [Color(0xFF9C2A60), Color(0xFF79204A), Color(0xFF4A1129)])
        : (pressed
            ? const [Color(0xFF2F3543), Color(0xFF20242E), Color(0xFF161920)]
            : const [Color(0xFF454D60), Color(0xFF2F3543), Color(0xFF20242E)]);

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
      Paint()..color = pressed ? const Color(0x26E8F0FF) : const Color(0x4DE8F0FF),
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
      close != oldDelegate.close || pressed != oldDelegate.pressed;
}
