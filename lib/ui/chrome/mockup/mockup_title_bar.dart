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
    this.onMinimize,
    this.onCollapse,
    this.onZoomOut,
    this.onZoomIn,
    this.onClose,
  });

  final String windowName;
  final double wordmarkSize;
  final bool showVersion;
  final bool showZoom;
  final VoidCallback? onMinimize;

  /// When set (EQ / playlist), shows Collapse (shade) instead of Minimize.
  final VoidCallback? onCollapse;
  final VoidCallback? onZoomOut;
  final VoidCallback? onZoomIn;
  final VoidCallback? onClose;

  static const height = 42.0;

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
              padding: const EdgeInsets.fromLTRB(10, 0, 9, 0),
              child: Row(
                children: [
                  const _TitleLogo(),
                  const SizedBox(width: 12),
                  _Wordmark(size: wordmarkSize, showVersion: showVersion),
                  const SizedBox(width: 12),
                  const Expanded(child: _Grip()),
                  const SizedBox(width: 12),
                  Text(
                    windowName.toUpperCase(),
                    style: const TextStyle(
                      fontFamily: 'TrampCondensed',
                      fontWeight: FontWeight.w700,
                      fontSize: 13,
                      height: 1,
                      letterSpacing: 13 * 0.26,
                      color: Color(0x8CC8D6EB),
                      shadows: [
                        Shadow(
                          offset: Offset(0, 1),
                          color: Color(0xB3000000),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 12),
                  const Expanded(child: _Grip()),
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
}

class _TitleLogo extends StatelessWidget {
  const _TitleLogo();

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 30,
      height: 30,
      decoration: const BoxDecoration(
        shape: BoxShape.circle,
        color: Color(0xFFE9ECF4),
        boxShadow: [
          BoxShadow(color: Color(0xA6000000), spreadRadius: 1),
          BoxShadow(
            color: Color(0x47FF3D9A),
            blurRadius: 12,
          ),
          BoxShadow(
            color: Color(0x8C000000),
            offset: Offset(0, 2),
            blurRadius: 4,
          ),
        ],
      ),
      child: Stack(
        fit: StackFit.expand,
        children: [
          const Padding(
            padding: EdgeInsets.all(1),
            child: ClipOval(child: TrampLogo(size: 28)),
          ),
          DecoratedBox(
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              gradient: LinearGradient(
                begin: const Alignment(-0.6, -0.8),
                end: const Alignment(0.4, 0.6),
                colors: [
                  const Color(0x73FFFFFF),
                  const Color(0x00FFFFFF),
                ],
                stops: const [0, 0.46],
              ),
            ),
          ),
        ],
      ),
    );
  }
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
            color: const Color(0xFFEAF2FF),
            shadows: const [
              Shadow(offset: Offset(0, -1), color: Color(0x4DE2ECFF)),
              Shadow(offset: Offset(0, 1), color: Color(0xD9000000)),
              Shadow(color: Color(0x4D3DE7FF), blurRadius: 14),
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
    return SizedBox(
      height: 8,
      child: Stack(
        alignment: Alignment.centerLeft,
        children: [
          Container(
            height: 2,
            decoration: BoxDecoration(
              gradient: const LinearGradient(
                colors: [
                  Color(0x00000000),
                  MockupTokens.phosDim,
                  MockupTokens.accentDim,
                  MockupTokens.phosDim,
                  Color(0x00000000),
                ],
                stops: [0, 0.12, 0.5, 0.88, 1],
              ),
              boxShadow: const [
                BoxShadow(color: Color(0x4D3DE7FF), blurRadius: 7),
              ],
            ),
          ),
          Positioned(
            left: 0,
            right: 0,
            top: 6,
            child: Container(
              height: 1,
              decoration: const BoxDecoration(
                gradient: LinearGradient(
                  colors: [
                    Color(0x00000000),
                    Color(0x59FF3D9A),
                    Color(0x00000000),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
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
    final rrect = RRect.fromRectAndRadius(
      Offset.zero & size,
      const Radius.circular(3),
    );
    final colors = close
        ? (pressed
            ? const [Color(0xFF79204A), Color(0xFF4A1129), Color(0xFF2E0A1A)]
            : const [Color(0xFF9C2A60), Color(0xFF79204A), Color(0xFF4A1129)])
        : (pressed
            ? const [Color(0xFF2F3543), Color(0xFF20242E), Color(0xFF161920)]
            : const [Color(0xFF454D60), Color(0xFF2F3543), Color(0xFF20242E)]);
    canvas.drawRRect(
      rrect,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: colors,
          stops: const [0, 0.55, 1],
        ).createShader(Offset.zero & size),
    );
    canvas.drawRRect(
      rrect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x4DE8F0FF),
    );
  }

  @override
  bool shouldRepaint(covariant _WinBtnPainter oldDelegate) =>
      close != oldDelegate.close || pressed != oldDelegate.pressed;
}
