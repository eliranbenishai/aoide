import 'dart:async';

/// End detection for OS [windowManager.startDragging] gestures.
///
/// Move events can pause briefly while the mouse is still down. A short
/// "quiet → finalize" timer treats that as release and detaches the dock
/// cohort mid-drag. This tracker:
/// - uses a long last-resort quiet delay (WM_EXITSIZEMOVE is sometimes lost)
/// - remembers a soft quiet-end so the next move can **resume** the drag
/// - prefers confirmed ends (pointer-up / onWindowMoved) via [endedConfirmed]
class NativeDragTracker {
  NativeDragTracker({
    this.quietFinalizeDelay = const Duration(milliseconds: 1500),
    required this.onQuietFinalize,
  });

  /// Last-resort finalize when neither pan-up nor onWindowMoved arrives.
  final Duration quietFinalizeDelay;

  /// Invoked after a quiet timeout (soft end — drag may still resume).
  final void Function() onQuietFinalize;

  bool _active = false;
  bool _softEnded = false;
  Timer? _quiet;

  bool get isActive => _active;

  /// True after a quiet finalize until the next move resumes or a hard end.
  bool get softEnded => _softEnded;

  void started() {
    _active = true;
    _softEnded = false;
    _cancelQuiet();
  }

  /// Returns whether this move belongs to an active (or resumed) drag.
  bool onMoveEvent() {
    if (!_active && _softEnded) {
      _active = true;
      _softEnded = false;
    }
    if (!_active) return false;
    _armQuiet();
    return true;
  }

  /// Confirmed end (pointer up / onWindowMoved).
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
