import 'package:flutter/widgets.dart';

import '../../../look/look_palette.dart';
import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';
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
    // Clearing pressed state must always work (e.g. disable mid-gesture).
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
          final look = LookScope.of(context);
          final palette = look.palette;
          final onInk = LookPaint.buttonOnInk(palette);
          final lift = LookPaint.hoverLiftTarget(palette);
          Widget content = widget.child ??
              Text(
                widget.label!.toUpperCase(),
                style: TextStyle(
                  fontFamily: look.chromeFamily,
                  fontWeight: FontWeight.w700,
                  fontSize: 13,
                  height: 1,
                  letterSpacing: 13 * 0.18,
                  decoration: TextDecoration.none,
                  color: widget.on ? onInk : LookPaint.buttonLabelIdle(palette),
                ),
              );
          if (widget.on && widget.child != null) {
            content = ColorFiltered(
              colorFilter: ColorFilter.mode(onInk, BlendMode.srcIn),
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
                Positioned(
                  right: 5,
                  bottom: 5,
                  child: CustomPaint(
                    size: const Size(6, 6),
                    painter: _MenuCaretPainter(color: LookPaint.glyphInk(palette, 0x73)),
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
                Positioned(
                  left: -14,
                  right: -14,
                  top: -14,
                  bottom: -14,
                  child: CustomPaint(
                    painter: _OnBloomPainter(
                      color: LookPaint.phosphorBloom(palette),
                    ),
                  ),
                ),
              CustomPaint(
                painter: _ButtonPainter(
                  on: widget.on,
                  pressed: _down,
                  hover: hover,
                  palette: palette,
                  liftTarget: lift,
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
  const _OnBloomPainter({required this.color});

  final Color color;

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
        ..color = color
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
    );
  }

  @override
  bool shouldRepaint(covariant _OnBloomPainter oldDelegate) =>
      color != oldDelegate.color;
}

class _ButtonPainter extends CustomPainter {
  const _ButtonPainter({
    required this.on,
    required this.pressed,
    required this.hover,
    required this.palette,
    required this.liftTarget,
  });

  final bool on;
  final bool pressed;
  final double hover;
  final LookPalette palette;
  final Color liftTarget;

  @override
  void paint(Canvas canvas, Size size) {
    final rrect = RRect.fromRectAndRadius(
      Offset.zero & size,
      const Radius.circular(4),
    );

    if (on) {
      final base = LookPaint.buttonOn(palette);
      final colors = [
        mockupHoverLift(base[0], hover, 0.1, liftTarget),
        mockupHoverLift(base[1], hover, 0.08, liftTarget),
        mockupHoverLift(base[2], hover, 0.08, liftTarget),
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
        Paint()
          ..color = LookPaint.buttonOnLip(palette).withValues(alpha: 0xB3 / 255),
      );
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(1, size.height - 4, size.width - 2, 3),
          const Radius.circular(2),
        ),
        Paint()
          ..color = LookPaint.buttonOnFoot(palette).withValues(alpha: 0x8C / 255),
      );
    } else {
      final base =
          pressed ? LookPaint.buttonPressed(palette) : LookPaint.buttonIdle(palette);
      // Pressed wins over hover for the dark inset; otherwise lift the face.
      final colors = pressed
          ? base
          : base
              .map((c) => mockupHoverLift(c, hover, MockupHoverTokens.faceLift, liftTarget))
              .toList(growable: false);
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
      final sheen = LookPaint.hoverLiftTarget(palette);
      canvas.drawRRect(
        rrect,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              sheen.withValues(alpha: 0x33 / 255),
              sheen.withValues(alpha: 0),
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
    final glossTop = LookPaint.hoverLiftTarget(palette);
    canvas.drawRRect(
      gloss,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            glossTop.withValues(alpha: on ? 0.28 : 0.12 + 0.08 * hover),
            glossTop.withValues(alpha: 0),
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
      hover != oldDelegate.hover ||
      palette != oldDelegate.palette ||
      liftTarget != oldDelegate.liftTarget;
}

class _MenuCaretPainter extends CustomPainter {
  const _MenuCaretPainter({required this.color});

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, size.height)
      ..lineTo(size.width, size.height)
      ..lineTo(size.width, 0)
      ..close();
    canvas.drawPath(path, Paint()..color = color);
  }

  @override
  bool shouldRepaint(covariant _MenuCaretPainter oldDelegate) =>
      color != oldDelegate.color;
}
