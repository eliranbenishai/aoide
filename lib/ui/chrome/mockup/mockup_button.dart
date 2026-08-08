import 'package:flutter/widgets.dart';

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
    if (value && !widget.isEnabled) return;
    if (_down == value) return;
    setState(() => _down = value);
  }

  @override
  Widget build(BuildContext context) {
    final content = widget.child ??
        Text(
          widget.label!,
          style: TextStyle(
            fontFamily: 'TrampCondensed',
            fontWeight: FontWeight.w700,
            fontSize: 13,
            height: 1,
            letterSpacing: 13 * 0.18,
            color: widget.on
                ? const Color(0xFF04222B)
                : const Color(0xB8C4D2E8),
          ),
        );

    Widget button = CustomPaint(
      painter: _ButtonPainter(on: widget.on, pressed: _down),
      child: Stack(
        alignment: Alignment.center,
        children: [
          Padding(
            padding: widget.padding ?? EdgeInsets.zero,
            child: Center(child: content),
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
      ),
    );

    if (widget.width != null || widget.height != null) {
      button = SizedBox(
        width: widget.width,
        height: widget.height,
        child: button,
      );
    }

    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.semanticLabel ?? widget.label,
      child: MouseRegion(
        cursor: widget.isEnabled
            ? SystemMouseCursors.click
            : SystemMouseCursors.basic,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (_) => _setDown(true),
          onTapUp: (_) => _setDown(false),
          onTapCancel: () => _setDown(false),
          onTap: widget.onPressed,
          child: button,
        ),
      ),
    );
  }
}

class _ButtonPainter extends CustomPainter {
  const _ButtonPainter({required this.on, required this.pressed});

  final bool on;
  final bool pressed;

  @override
  void paint(Canvas canvas, Size size) {
    final rrect = RRect.fromRectAndRadius(
      Offset.zero & size,
      const Radius.circular(4),
    );

    if (on) {
      canvas.drawRRect(
        rrect.inflate(4),
        Paint()
          ..color = const Color(0x663DE7FF)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 7),
      );
      canvas.drawRRect(
        rrect,
        Paint()
          ..shader = const LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              Color(0xFFA9F4FF),
              Color(0xFF3DE7FF),
              Color(0xFF128FA8),
            ],
            stops: [0, 0.45, 1],
          ).createShader(Offset.zero & size),
      );
      canvas.drawRRect(
        rrect,
        Paint()
          ..color = const Color(0xB3F0FDFF)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1,
      );
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(1, size.height - 4, size.width - 2, 3),
          const Radius.circular(2),
        ),
        Paint()..color = const Color(0x8C054658),
      );
    } else {
      canvas.drawRRect(
        rrect,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: pressed
                ? const [
                    Color(0xFF2B313E),
                    Color(0xFF1E222C),
                    Color(0xFF161A22),
                  ]
                : const [
                    Color(0xFF3F4657),
                    Color(0xFF2B313E),
                    Color(0xFF1E222C),
                  ],
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
            Color.fromRGBO(232, 240, 255, on ? 0.28 : 0.12),
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
      on != oldDelegate.on || pressed != oldDelegate.pressed;
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
