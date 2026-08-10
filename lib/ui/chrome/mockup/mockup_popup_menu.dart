import 'package:flutter/material.dart';

/// Vertical placement of a mockup popup relative to its anchor button.
enum MockupMenuPlacement {
  /// Menu sits just below the button (main options cog, EQ presets).
  below,

  /// Menu sits just above the button (playlist footer menus).
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

  // Button rect in overlay coordinates (handles zoom transforms).
  final button = Rect.fromPoints(
    anchor.localToGlobal(Offset.zero, ancestor: overlay),
    anchor.localToGlobal(anchor.size.bottomRight(Offset.zero), ancestor: overlay),
  );

  final menuHeight = _estimateMenuHeight(items);
  final RelativeRect position;
  switch (placement) {
    case MockupMenuPlacement.below:
      // Same idea as [PopupMenuPosition.under]: menu top at button bottom.
      final top = button.bottom + gap;
      position = RelativeRect.fromLTRB(
        button.left,
        top,
        overlay.size.width - button.right,
        overlay.size.height - top,
      );
    case MockupMenuPlacement.above:
      // Place menu top so its bottom clears the button (estimate height).
      final top = button.top - gap - menuHeight;
      position = RelativeRect.fromLTRB(
        button.left,
        top,
        overlay.size.width - button.right,
        overlay.size.height - top,
      );
  }

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
