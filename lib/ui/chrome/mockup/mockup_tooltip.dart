import 'package:flutter/material.dart';

import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';
import '../../zoom/zoom_scope.dart';

/// Hover label naming what a control does, in the chrome's own materials.
///
/// Several controls are glyph-only, and a few of the labelled ones are
/// abbreviations (EQ, PL), so the face alone does not always say what the
/// button is for.
///
/// Sizes itself by the zoom factor rather than riding the panel's transform:
/// tooltips live in the overlay, which sits above the single root
/// [ZoomedCanvas] transform, so at 125% an unscaled tip would read noticeably
/// smaller than the chrome around it. Same reason the mockup popup menu scales
/// itself.
class MockupTooltip extends StatelessWidget {
  const MockupTooltip({super.key, required this.message, required this.child});

  /// Null or blank passes [child] straight through, so a control with nothing
  /// worth saying costs nothing.
  final String? message;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    final text = message?.trim();
    if (text == null || text.isEmpty) return child;

    // A tooltip needs somewhere to float, and chrome is routinely pumped bare
    // in goldens and widget tests. Passing through keeps a shared primitive
    // from making an Overlay a condition of rendering a button — the same
    // reason ZoomScope falls back rather than asserting.
    if (Overlay.maybeOf(context) == null) return child;

    final look = LookScope.of(context);
    final palette = look.palette;
    final zoom = ZoomScope.maybeOf(context)?.factor ?? 1.0;

    return Tooltip(
      message: text,
      waitDuration: const Duration(milliseconds: 450),
      preferBelow: false,
      // The button already names itself through Semantics; letting the tooltip
      // add a second node would read the same thing twice.
      excludeFromSemantics: true,
      // Hover only. Buttons own their tap gesture, and a long-press tooltip
      // would sit in the same arena as the controls it labels.
      triggerMode: TooltipTriggerMode.manual,
      padding: EdgeInsets.symmetric(horizontal: 9 * zoom, vertical: 5 * zoom),
      margin: EdgeInsets.all(6 * zoom),
      decoration: BoxDecoration(
        color: palette.shellMid,
        borderRadius: BorderRadius.circular(3 * zoom),
        border: Border.all(
          color: LookPaint.coolSheen(palette).withValues(alpha: 0x33 / 255),
        ),
        boxShadow: const [
          BoxShadow(color: Color(0xB3000000), blurRadius: 8, offset: Offset(0, 2)),
        ],
      ),
      textStyle: TextStyle(
        fontFamily: look.chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: 11 * zoom,
        letterSpacing: 11 * zoom * 0.12,
        height: 1.2,
        decoration: TextDecoration.none,
        color: palette.inkDefault,
      ),
      child: child,
    );
  }
}
