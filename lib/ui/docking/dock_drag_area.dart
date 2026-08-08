import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';

import 'dock_drag_session.dart';

/// Title-bar / grip drag that drives docking via logical coordinates + Shift.
///
/// Replaces `DragToMoveArea` so [DockingCoordinator.move] stays authoritative.
class DockDragArea extends StatefulWidget {
  const DockDragArea({
    super.key,
    required this.zoom,
    required this.logicalTopLeft,
    required this.onMove,
    required this.child,
  });

  /// Global zoom factor (logical → pixel).
  final double zoom;

  /// Current logical top-left of the window being dragged (read at pan-start).
  final ValueGetter<Offset> logicalTopLeft;

  /// Called on pan update/end with logical top-left and Shift undock flag.
  final void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  }) onMove;

  final Widget child;

  @override
  State<DockDragArea> createState() => _DockDragAreaState();
}

class _DockDragAreaState extends State<DockDragArea> {
  DockDragSession? _session;

  bool get _shiftUndock => HardwareKeyboard.instance.isShiftPressed;

  void _emit(Offset globalPosition, {required bool ended}) {
    final session = _session;
    if (session == null) return;
    widget.onMove(
      session.logicalTopLeftFor(globalPosition),
      shiftUndock: _shiftUndock,
      ended: ended,
    );
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.translucent,
      onPanStart: (details) {
        _session = DockDragSession(
          originLogical: widget.logicalTopLeft(),
          originGlobal: details.globalPosition,
          zoom: widget.zoom,
        );
      },
      onPanUpdate: (details) {
        _emit(details.globalPosition, ended: false);
      },
      onPanEnd: (details) {
        _emit(details.globalPosition, ended: true);
        _session = null;
      },
      onPanCancel: () {
        _session = null;
      },
      child: widget.child,
    );
  }
}
