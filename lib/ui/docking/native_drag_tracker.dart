import 'dart:async';

/// End detection for OS `startDragging` gestures.
///
/// Prefer [endedConfirmed] from `onWindowMoved`. A quiet timeout is only a
/// last resort when WM_EXITSIZEMOVE is dropped — and must be long enough that
/// brief pauses do not detach the dock cohort. If move events resume after a
/// quiet finalize, [onMoveEvent] reports a resume so the host can reattach.
class NativeDragTracker {
  NativeDragTracker({
    this.quietFinalizeDelay = const Duration(milliseconds: 750),
    required this.onQuietFinalize,
  });

  final Duration quietFinalizeDelay;
  final void Function() onQuietFinalize;

  bool _active = false;
  bool _softEnded = false;
  Timer? _quiet;

  bool get isActive => _active;
  bool get softEnded => _softEnded;

  void started() {
    _active = true;
    _softEnded = false;
    _cancelQuiet();
  }

  /// Returns true when this move is part of an active drag (or a resume).
  bool onMoveEvent() {
    if (!_active && _softEnded) {
      _active = true;
      _softEnded = false;
    }
    if (!_active) return false;
    _armQuiet();
    return true;
  }

  void endedConfirmed() {
    _cancelQuiet();
    _active = false;
    _softEnded = false;
  }

  void dispose() => _cancelQuiet();

  void _armQuiet() {
    _quiet?.cancel();
    _quiet = Timer(quietFinalizeDelay, () {
      if (!_active) return;
      _active = false;
      _softEnded = true;
      onQuietFinalize();
    });
  }

  void _cancelQuiet() {
    _quiet?.cancel();
    _quiet = null;
  }
}
