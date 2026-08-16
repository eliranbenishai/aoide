import 'dart:io';
import 'dart:ui';

/// Process-env switches for local drag / window-host experiments.
///
/// Not product settings. A missing or non-`1` value is off.
/// Linux C++ (not Dart): `TRAMP_ENABLE_IMPELLER=1` restores Impeller.
class HarnessFlags {
  HarnessFlags._();

  /// Do not spawn EQ / playlist / settings / about windows.
  static bool get soloMain => _on('TRAMP_SOLO_MAIN');

  /// Skip [LinuxDragPoll] during native title-bar drag.
  static bool get disableLinuxDragPoll => _on('TRAMP_DISABLE_LINUX_DRAG_POLL');

  /// Opaque window fill (no per-pixel alpha). Isolates compositor snapshot drag.
  static bool get opaqueWindows => _on('TRAMP_OPAQUE_WINDOWS');

  /// Time windowManager getPosition / setPosition then exit.
  static bool get positionBench => _on('TRAMP_POSITION_BENCH');

  static bool _on(String key) => Platform.environment[key] == '1';
}

/// GTK / Flutter window fill. Transparent so mockup corners punch through,
/// unless [HarnessFlags.opaqueWindows] is set.
Color trampWindowFill({bool opaque = false}) {
  if (opaque || HarnessFlags.opaqueWindows) {
    return const Color(0xFF1A1A1A);
  }
  return const Color(0x00000000);
}
