import 'dart:async';
import 'dart:io';

/// While a native title-bar drag is active on Linux, poll for position sync.
///
/// `window_manager` on Linux often does not emit `move` / `moved` during
/// `gtk_window_begin_move_drag`, so [NativeDragTracker] never re-arms and
/// dock snap / sibling carry never run. Periodic ticks drive the same path
/// as configure-event move notifications.
class LinuxDragPoll {
  LinuxDragPoll({
    this.interval = const Duration(milliseconds: 32),
    required this.onTick,
  });

  final Duration interval;
  final void Function() onTick;

  Timer? _timer;

  static bool get isNeeded => Platform.isLinux;

  void start() {
    if (!isNeeded) return;
    stop();
    _timer = Timer.periodic(interval, (_) => onTick());
  }

  void stop() {
    _timer?.cancel();
    _timer = null;
  }

  void dispose() => stop();
}
