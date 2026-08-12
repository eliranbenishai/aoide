import 'dart:async';
import 'dart:io';

import 'package:flutter/painting.dart';

/// While a native title-bar drag is active on Linux, poll window position.
///
/// `window_manager` on Linux often emits neither `move` nor `moved` during
/// `gtk_window_begin_move_drag`. Ticks must only signal **motion** when the
/// position actually changes — arming quiet-end on every tick deadlocks
/// (quiet never elapses → poll never stops → snap never finalizes).
class LinuxDragPoll {
  LinuxDragPoll({
    this.interval = const Duration(milliseconds: 32),
    required this.getPosition,
    required this.onMotion,
  });

  final Duration interval;
  final Future<Offset> Function() getPosition;
  final void Function(Offset position) onMotion;

  /// Plugin px threshold to count as motion.
  static const double motionEpsilon = 1.0;

  Timer? _timer;
  Offset? _lastPosition;
  bool _tickInFlight = false;

  static bool get isNeeded => Platform.isLinux;

  bool get isRunning => _timer != null;

  void start() {
    if (!isNeeded) return;
    stop();
    _lastPosition = null;
    _timer = Timer.periodic(interval, (_) => unawaited(_tick()));
  }

  void stop() {
    _timer?.cancel();
    _timer = null;
    _lastPosition = null;
    _tickInFlight = false;
  }

  void dispose() => stop();

  Future<void> _tick() async {
    if (!isRunning || _tickInFlight) return;
    _tickInFlight = true;
    try {
      final position = await getPosition();
      if (!isRunning) return;
      final last = _lastPosition;
      _lastPosition = position;
      if (last == null) return;
      final dx = position.dx - last.dx;
      final dy = position.dy - last.dy;
      if (dx * dx + dy * dy < motionEpsilon * motionEpsilon) return;
      onMotion(position);
    } finally {
      _tickInFlight = false;
    }
  }
}
