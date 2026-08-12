import 'dart:async';

/// End detection for OS `startDragging` gestures.
///
/// Prefer [endedConfirmed] from `onWindowMoved` (Windows). On Linux the plugin
/// never emits `moved`, so [onQuietFinalize] is the real gesture end — hosts
/// should still snap / persist there.
///
/// Quiet is armed only from [onMoveEvent] (real motion), never from
/// [started] alone — arming at press false-ends mid-drag on Linux. Keep the
/// delay long enough that brief pauses do not detach the dock cohort. If move
/// events resume after a quiet finalize, [onMoveEvent] reports a resume so
/// the host can reattach.
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
    // Do not arm quiet here — wait for motion via [onMoveEvent].
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
