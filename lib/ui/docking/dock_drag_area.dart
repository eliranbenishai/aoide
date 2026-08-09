import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:window_manager/window_manager.dart';

import 'dock_drag_session.dart';

/// Title-bar / grip drag that drives docking.
///
/// Production ([nativeDragging] = true): OS owns the dragged HWND via
/// [windowManager.startDragging] so the window tracks the cursor with no
/// Flutter `setPosition` fight. Callers sync docked siblings from
/// `onWindowMove` / `onWindowMoved` using [onNativeDragStarted].
///
/// Important: [startDragging] steals the pointer, so Flutter often delivers
/// `onPanCancel` immediately — that must **not** be treated as drag end.
///
/// Tests ([nativeDragging] = false): pan updates drive [onMove] through
/// [DockDragSession] without touching the OS.
class DockDragArea extends StatefulWidget {
  const DockDragArea({
    super.key,
    required this.zoom,
    required this.logicalTopLeft,
    required this.onMove,
    required this.child,
    this.nativeDragging = true,
    this.onNativeDragStarted,
    this.startDragging,
  });

  /// Global zoom factor (logical → pixel).
  final double zoom;

  /// Current logical top-left of the window being dragged (read at pan-start).
  final ValueGetter<Offset> logicalTopLeft;

  /// Called on pan update/end with logical top-left and Shift undock flag.
  ///
  /// Only used when [nativeDragging] is false (widget tests).
  final void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  }) onMove;

  /// Invoked when a native OS drag begins so the host/client can enter
  /// sibling-sync mode.
  final VoidCallback? onNativeDragStarted;

  /// Override for tests; defaults to [windowManager.startDragging].
  final Future<void> Function()? startDragging;

  /// When true, use OS drag; when false, gesture-driven [onMove] (tests).
  final bool nativeDragging;

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

  Future<void> _beginNativeDrag() async {
    widget.onNativeDragStarted?.call();
    final start = widget.startDragging ?? windowManager.startDragging;
    await start();
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.translucent,
      onPanStart: (details) {
        if (widget.nativeDragging) {
          unawaited(_beginNativeDrag());
          return;
        }
        _session = DockDragSession(
          originLogical: widget.logicalTopLeft(),
          originGlobal: details.globalPosition,
          zoom: widget.zoom,
        );
      },
      onPanUpdate: (details) {
        if (widget.nativeDragging) return;
        _emit(details.globalPosition, ended: false);
      },
      onPanEnd: (details) {
        if (widget.nativeDragging) return;
        _emit(details.globalPosition, ended: true);
        _session = null;
      },
      onPanCancel: () {
        // Native startDragging cancels the Flutter pan — ignore.
        _session = null;
      },
      child: widget.child,
    );
  }
}
