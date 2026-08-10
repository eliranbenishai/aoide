import 'package:flutter/material.dart';

/// Vertical placement of a mockup popup relative to its anchor button.
enum MockupMenuPlacement {
  /// Prefer just below the button; if it won't fit, open to the right instead.
  ///
  /// Needed on the short main/EQ canvases (especially when zoomed): Material's
  /// [showMenu] otherwise slides an oversized menu upward and covers the trigger.
  below,

  /// Prefer just above the button; if it won't fit, open to the right instead.
  above,
}

/// Shows a [showMenu] popup anchored to [anchor] without covering it.
Future<T?> showMockupMenu<T>({
  required BuildContext context,
  required RenderBox anchor,
  required List<PopupMenuEntry<T>> items,
  required MockupMenuPlacement placement,
  Color? color,
}) {
  assert(items.isNotEmpty);
  final overlay =
      Overlay.of(context).context.findRenderObject()! as RenderBox;
  const gap = 4.0;
  const edgePad = 8.0;

  // Button rect in overlay coordinates (handles zoom transforms).
  final button = Rect.fromPoints(
    anchor.localToGlobal(Offset.zero, ancestor: overlay),
    anchor.localToGlobal(anchor.size.bottomRight(Offset.zero), ancestor: overlay),
  );

  final menuHeight = _estimateMenuHeight(items);
  final maxBottom = overlay.size.height - edgePad;

  late final double top;
  late final double left;
  switch (placement) {
    case MockupMenuPlacement.below:
      final belowTop = button.bottom + gap;
      if (belowTop + menuHeight <= maxBottom) {
        top = belowTop;
        left = button.left;
      } else {
        // Not enough room under the trigger (common at <100% zoom on main).
        // Sit beside it so the lit button stays visible.
        top = button.top.clamp(edgePad, maxBottom);
        left = button.right + gap;
      }
    case MockupMenuPlacement.above:
      final aboveTop = button.top - gap - menuHeight;
      if (aboveTop >= edgePad) {
        top = aboveTop;
        left = button.left;
      } else {
        top = (button.top - menuHeight).clamp(edgePad, maxBottom);
        left = button.right + gap;
      }
  }

  final position = RelativeRect.fromLTRB(
    left,
    top,
    overlay.size.width - left - button.width,
    overlay.size.height - top,
  );

  return showMenu<T>(
    context: context,
    position: position,
    color: color,
    items: items,
  );
}

double _estimateMenuHeight(List<PopupMenuEntry<dynamic>> items) {
  var height = 0.0;
  for (final item in items) {
    height += item.height;
  }
  // Material popup vertical padding (symmetric 8).
  return height + 16;
}
