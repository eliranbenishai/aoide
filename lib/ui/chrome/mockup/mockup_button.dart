import 'package:flutter/widgets.dart';

import 'mockup_hover.dart';

/// Raised control matching mockup `.btn` / `.btn--on` / `.btn--label`.
class MockupButton extends StatefulWidget {
  const MockupButton({
    super.key,
    this.onPressed,
    this.child,
    this.label,
    this.on = false,
    this.menu = false,
    this.width,
    this.height,
    this.padding,
    this.semanticLabel,
  }) : assert(child != null || label != null);

  final VoidCallback? onPressed;
  final Widget? child;
  final String? label;
  final bool on;
  final bool menu;
  final double? width;
  final double? height;
  final EdgeInsetsGeometry? padding;
  final String? semanticLabel;

  bool get isEnabled => onPressed != null;

  @override
  State<MockupButton> createState() => _MockupButtonState();
}

class _MockupButtonState extends State<MockupButton> {
  bool _down = false;

  void _setDown(bool value) {
    // Clearing pressed must always work (e.g. disable mid-gesture).
    if (value && !widget.isEnabled) return;
    if (_down == value) return;
    setState(() => _down = value);
  }

  @override
  void didUpdateWidget(covariant MockupButton oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!widget.isEnabled && _down) {
      _down = false;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.semanticLabel ?? widget.label,
      child: MockupHover(
        enabled: widget.isEnabled,
        builder: (context, hover) {
          // `.btn--on` ink is deep teal (`#04222b`) for labels and SVG fills.
          const onInk = Color(0xFF04222B);
          Widget content = widget.child ??
              Text(
                widget.label!.toUpperCase(),
                style: TextStyle(
                  fontFamily: 'TrampCondensed',
                  fontWeight: FontWeight.w700,
                  fontSize: 13,
                  height: 1,
                  letterSpacing: 13 * 0.18,
                  decoration: TextDecoration.none,
                  color: widget.on ? onInk : const Color(0xB8C4D2E8),
                ),
              );
          if (widget.on && widget.child != null) {
            content = ColorFiltered(
              colorFilter: const ColorFilter.mode(onInk, BlendMode.srcIn),
              child: content,
            );
          }

          final face = Stack(
            alignment: Alignment.center,
            children: [
              Padding(
                padding: widget.padding ?? EdgeInsets.zero,
                child: Center(
                  child: MockupGlyphGlow(amount: hover, child: content),
                ),
              ),
              if (widget.menu)
                const Positioned(
                  right: 5,
                  bottom: 5,
                  child: CustomPaint(
                    size: Size(6, 6),
                    painter: _MenuCaretPainter(),
                  ),
                ),
            ],
          );

          // Stack with Clip.none so `.btn--on` bloom can paint outside the
          // layout box (CSS box-shadow is not clipped to the border box).
          Widget button = Stack(
            clipBehavior: Clip.none,
            alignment: Alignment.center,
            children: [
              if (widget.on)
                const Positioned(
                  left: -14,
                  right: -14,
                  top: -14,
                  bottom: -14,
                  child: CustomPaint(painter: _OnBloomPainter()),
                ),
              CustomPaint(
                painter: _ButtonPainter(
                  on: widget.on,
                  pressed: _down,
                  hover: hover,
                ),
                child: face,
              ),
            ],
          );

          if (widget.width != null || widget.height != null) {
            button = SizedBox(
              width: widget.width,
              height: widget.height,
              child: button,
            );
          }

          if (!widget.isEnabled) {
            button = Opacity(
              opacity: MockupHoverTokens.disabledOpacity,
              child: button,
            );
          }

          return GestureDetector(
            behavior: HitTestBehavior.opaque,
            onTapDown: (_) => _setDown(true),
            onTapUp: (_) => _setDown(false),
            onTapCancel: () => _setDown(false),
            onTap: widget.onPressed,
            child: button,
          );
        },
      ),
    );
  }
}

/// Soft outer bloom for lit buttons — sized larger than the face via [Positioned].
class _OnBloomPainter extends CustomPainter {
  const _OnBloomPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final inset = 14.0;
    final face = Rect.fromLTWH(
      inset,
      inset,
      size.width - inset * 2,
      size.height - inset * 2,
    );
    if (face.width <= 0 || face.height <= 0) return;
    final rrect = RRect.fromRectAndRadius(face, const Radius.circular(4));
    canvas.drawRRect(
      rrect.inflate(3),
      Paint()
        ..color = const Color(0x4D3DE7FF)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

class _ButtonPainter extends CustomPainter {
  const _ButtonPainter({
    required this.on,
    required this.pressed,
    required this.hover,
  });

  final bool on;
  final bool pressed;
  final double hover;

  @override
  void paint(Canvas canvas, Size size) {
    final rrect = RRect.fromRectAndRadius(
      Offset.zero & size,
      const Radius.circular(4),
    );

    if (on) {
      final colors = [
        mockupHoverLift(const Color(0xFFA9F4FF), hover, 0.1),
        mockupHoverLift(const Color(0xFF3DE7FF), hover, 0.08),
        mockupHoverLift(const Color(0xFF128FA8), hover, 0.08),
      ];
      canvas.drawRRect(
        rrect,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: colors,
            stops: const [0, 0.45, 1],
          ).createShader(Offset.zero & size),
      );
      canvas.drawLine(
        const Offset(2, 1),
        Offset(size.width - 2, 1),
        Paint()..color = const Color(0xB3F0FDFF),
      );
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(1, size.height - 4, size.width - 2, 3),
          const Radius.circular(2),
        ),
        Paint()..color = const Color(0x8C054658),
      );
    } else {
      final base = pressed
          ? const [
              Color(0xFF2B313E),
              Color(0xFF1E222C),
              Color(0xFF161A22),
            ]
          : const [
              Color(0xFF3F4657),
              Color(0xFF2B313E),
              Color(0xFF1E222C),
            ];
      // Pressed wins over hover for the dark inset; otherwise lift the face.
      final colors = pressed
          ? base
          : base.map((c) => mockupHoverLift(c, hover)).toList(growable: false);
      canvas.drawRRect(
        rrect,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: colors,
            stops: const [0, 0.48, 1],
          ).createShader(Offset.zero & size),
      );
      canvas.drawRRect(
        rrect,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              const Color(0x33E8F0FF),
              const Color(0x00000000),
              const Color(0x80000000),
            ],
          ).createShader(Offset.zero & size),
      );
    }

    // Top gloss (::before).
    final gloss = RRect.fromRectAndCorners(
      Rect.fromLTRB(1, 1, size.width - 1, size.height * 0.55),
      topLeft: const Radius.circular(3),
      topRight: const Radius.circular(3),
      bottomLeft: const Radius.circular(6),
      bottomRight: const Radius.circular(6),
    );
    canvas.drawRRect(
      gloss,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color.fromRGBO(232, 240, 255, on ? 0.28 : 0.12 + 0.08 * hover),
            const Color(0x00E8F0FF),
          ],
        ).createShader(gloss.outerRect),
    );

    canvas.drawRRect(
      rrect.shift(const Offset(0, 1)),
      Paint()
        ..color = const Color(0x99000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 1.2)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.5,
    );
  }

  @override
  bool shouldRepaint(covariant _ButtonPainter oldDelegate) =>
      on != oldDelegate.on ||
      pressed != oldDelegate.pressed ||
      hover != oldDelegate.hover;
}

class _MenuCaretPainter extends CustomPainter {
  const _MenuCaretPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, size.height)
      ..lineTo(size.width, size.height)
      ..lineTo(size.width, 0)
      ..close();
    canvas.drawPath(
      path,
      Paint()..color = const Color(0x73D6E2F5),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
