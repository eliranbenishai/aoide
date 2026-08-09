import 'dart:ui';

import 'package:flutter/widgets.dart';

import '../../../theme/look_scope.dart';

/// Shared desktop hover timing / intensity for mockup chrome controls.
abstract final class MockupHoverTokens {
  static const duration = Duration(milliseconds: 150);

  /// Blend toward light steel for face / thumb fills.
  static const faceLift = 0.14;

  /// Soft phosphor bloom behind glyphs (icons / labels).
  static const glyphGlowOpacity = 0.55;
  static const glyphBlurSigma = 2.2;

  /// Face / glyph dim when `onPressed == null` (disabled).
  static const disabledOpacity = 0.45;
}

/// Lighten [color] toward steel-white by [hover] × faceLift.
Color mockupHoverLift(Color color, double hover, [double amount = MockupHoverTokens.faceLift]) {
  if (hover <= 0) return color;
  return Color.lerp(color, const Color(0xFFE8F0FF), amount * hover)!;
}

/// Phosphor bloom behind [child] (icon / label). Amount is `0..1`.
class MockupGlyphGlow extends StatelessWidget {
  const MockupGlyphGlow({
    super.key,
    required this.amount,
    required this.child,
  });

  final double amount;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    if (amount <= 0.001) return child;
    return Stack(
      alignment: Alignment.center,
      clipBehavior: Clip.none,
      children: [
        IgnorePointer(
          child: Opacity(
            opacity: (MockupHoverTokens.glyphGlowOpacity * amount).clamp(0.0, 1.0),
            child: ImageFiltered(
              imageFilter: ImageFilter.blur(
                sigmaX: MockupHoverTokens.glyphBlurSigma,
                sigmaY: MockupHoverTokens.glyphBlurSigma,
              ),
              child: ColorFiltered(
                colorFilter: ColorFilter.mode(
                  LookScope.of(context).palette.phosphorDefault,
                  BlendMode.srcATop,
                ),
                child: child,
              ),
            ),
          ),
        ),
        child,
      ],
    );
  }
}

/// Animates a `0..1` hover amount (~150ms ease-out) via [builder].
class MockupHover extends StatefulWidget {
  const MockupHover({
    super.key,
    required this.builder,
    this.enabled = true,
    this.cursor,
  });

  final bool enabled;
  final MouseCursor? cursor;
  final Widget Function(BuildContext context, double hover) builder;

  @override
  State<MockupHover> createState() => _MockupHoverState();
}

class _MockupHoverState extends State<MockupHover>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: MockupHoverTokens.duration,
    );
  }

  @override
  void didUpdateWidget(covariant MockupHover oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!widget.enabled && _controller.value > 0) {
      _controller.reverse();
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      cursor: widget.cursor ??
          (widget.enabled
              ? SystemMouseCursors.click
              : SystemMouseCursors.basic),
      onEnter: (_) {
        if (widget.enabled) _controller.forward();
      },
      onExit: (_) => _controller.reverse(),
      child: AnimatedBuilder(
        animation: _controller,
        builder: (context, _) {
          final t = Curves.easeOut.transform(_controller.value);
          return widget.builder(context, t);
        },
      ),
    );
  }
}
