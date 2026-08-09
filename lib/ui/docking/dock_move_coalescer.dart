import 'dart:async';

/// Latest-wins scheduler for dock frame applies during title-bar drag.
///
/// Pointer events arrive faster than OS `setPosition` round-trips. Scheduling
/// every update causes overlapping applies and the window trails the cursor.
/// This coalescer keeps at most one apply in flight and, when more moves arrive
/// meanwhile, runs exactly one extra pass with whatever state is current.
class DockMoveCoalescer {
  Future<void>? _loop;
  bool _pending = false;

  /// Whether an apply loop is currently running.
  bool get isBusy => _loop != null;

  /// Request an apply. Concurrent calls collapse into one extra pass.
  void schedule(Future<void> Function() apply) {
    _pending = true;
    _loop ??= _run(apply);
  }

  /// Wait for any in-flight loop, then run [apply] once (drag-end full frame).
  Future<void> flush(Future<void> Function() apply) async {
    _pending = false;
    final inFlight = _loop;
    if (inFlight != null) {
      await inFlight;
    }
    await apply();
  }

  Future<void> _run(Future<void> Function() apply) async {
    try {
      do {
        _pending = false;
        await apply();
      } while (_pending);
    } finally {
      _loop = null;
    }
  }
}
