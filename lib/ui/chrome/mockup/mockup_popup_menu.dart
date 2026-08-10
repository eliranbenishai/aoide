import 'package:flutter/material.dart';

import '../../zoom/zoom_scope.dart';

/// Vertical placement of a mockup popup relative to its anchor button.
enum MockupMenuPlacement {
  /// Prefer just below the button; if it won't fit, open to the right instead.
  below,

  /// Prefer just above the button; if it won't fit, open to the right instead.
  above,
}

/// Shows a popup menu anchored to [anchor], scaled by the ambient [ZoomScope].
///
/// Chrome lives under [ZoomedCanvas]; Material's [showMenu] does not. This
/// builds a transparent-barrier dialog and applies the same zoom factor so the
/// menu matches chrome density and usually fits under the trigger.
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
  final zoom = ZoomScope.maybeOf(context)?.factor ?? 1.0;
  const edgePad = 8.0;
  final gap = 4.0 * zoom;

  // Button rect in overlay coordinates (includes ZoomedCanvas transform).
  final button = Rect.fromPoints(
    anchor.localToGlobal(Offset.zero, ancestor: overlay),
    anchor.localToGlobal(anchor.size.bottomRight(Offset.zero), ancestor: overlay),
  );

  // Menu is laid out at logical size then scaled — use visual height for fit.
  final visualMenuHeight = _estimateMenuHeight(items) * zoom;
  final overlayH = overlay.size.height;

  late final double top;
  late final double left;
  switch (placement) {
    case MockupMenuPlacement.below:
      final belowTop = button.bottom + gap;
      // Prefer under the trigger even if the menu kisses the window edge —
      // covering the lit button is worse than a flush bottom.
      if (belowTop + visualMenuHeight <= overlayH + 0.5) {
        top = belowTop;
        left = button.left;
      } else {
        top = button.top.clamp(edgePad, overlayH - edgePad);
        left = button.right + gap;
      }
    case MockupMenuPlacement.above:
      final aboveTop = button.top - gap - visualMenuHeight;
      if (aboveTop >= -0.5) {
        top = aboveTop.clamp(0.0, overlayH);
        left = button.left;
      } else {
        top = (button.top - visualMenuHeight).clamp(edgePad, overlayH - edgePad);
        left = button.right + gap;
      }
  }

  final menuColor = color ?? Theme.of(context).colorScheme.surface;

  return showGeneralDialog<T>(
    context: context,
    barrierDismissible: true,
    barrierLabel: MaterialLocalizations.of(context).modalBarrierDismissLabel,
    barrierColor: Colors.transparent,
    transitionDuration: Duration.zero,
    pageBuilder: (context, animation, secondaryAnimation) {
      return Stack(
        children: [
          Positioned(
            left: left,
            top: top,
            child: Transform.scale(
              scale: zoom,
              alignment: Alignment.topLeft,
              child: Material(
                color: menuColor,
                elevation: 8,
                shadowColor: Colors.black,
                child: IntrinsicWidth(
                  child: Padding(
                    padding: const EdgeInsets.symmetric(vertical: 8),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        for (final entry in items)
                          if (entry is PopupMenuItem<T>)
                            _MockupMenuRow<T>(item: entry),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
        ],
      );
    },
  );
}

double _estimateMenuHeight(List<PopupMenuEntry<dynamic>> items) {
  var height = 0.0;
  for (final item in items) {
    height += item.height;
  }
  // Match prior Material popup vertical padding (symmetric 8).
  return height + 16;
}

class _MockupMenuRow<T> extends StatelessWidget {
  const _MockupMenuRow({required this.item});

  final PopupMenuItem<T> item;

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: item.enabled
          ? () {
              item.onTap?.call();
              Navigator.of(context).pop<T>(item.value);
            }
          : null,
      child: SizedBox(
        height: item.height,
        child: Padding(
          padding: item.padding ??
              const EdgeInsets.symmetric(horizontal: 16),
          child: Align(
            alignment: AlignmentDirectional.centerStart,
            child: item.child,
          ),
        ),
      ),
    );
  }
}
