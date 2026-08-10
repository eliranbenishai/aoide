import '../../domain/tramp_settings.dart';

/// Visible tramp windows that participate in a main minimize/restore cycle.
///
/// Design §2 / plan pins: main title-bar minimize applies to the visible
/// docked group — implemented as every currently visible tramp window
/// (secondaries lack an independent minimize API). Settings is included when
/// [minimizeHidesSecondaries] is true (caller filters).
List<WindowId> minimizeGroupTargets({
  required bool mainVisible,
  required bool equalizerVisible,
  required bool playlistVisible,
  bool settingsVisible = false,
}) {
  return [
    if (mainVisible) WindowId.main,
    if (equalizerVisible) WindowId.equalizer,
    if (playlistVisible) WindowId.playlist,
    if (settingsVisible) WindowId.settings,
  ];
}

/// Bookkeeping for hiding/showing visible EQ/PL/settings with main minimize.
///
/// Layout visibility flags are left unchanged — secondaries are only
/// OS-hidden for the duration of the minimize cycle.
final class MinimizeGroupCycle {
  Set<WindowId> _secondariesHidden = {};
  bool _active = false;

  bool get isActive => _active;

  /// Snapshot secondaries to hide with main. Idempotent while already active.
  Set<WindowId> begin({
    required bool equalizerVisible,
    required bool playlistVisible,
    bool settingsVisible = false,
  }) {
    if (_active) return Set.unmodifiable(_secondariesHidden);
    _active = true;
    _secondariesHidden = {
      if (equalizerVisible) WindowId.equalizer,
      if (playlistVisible) WindowId.playlist,
      if (settingsVisible) WindowId.settings,
    };
    return Set.unmodifiable(_secondariesHidden);
  }

  /// Secondaries to show again (still marked visible in layout).
  Set<WindowId> end({
    required bool equalizerVisible,
    required bool playlistVisible,
    bool settingsVisible = false,
  }) {
    if (!_active) return const {};
    _active = false;
    final restore = {
      if (_secondariesHidden.contains(WindowId.equalizer) && equalizerVisible)
        WindowId.equalizer,
      if (_secondariesHidden.contains(WindowId.playlist) && playlistVisible)
        WindowId.playlist,
      if (_secondariesHidden.contains(WindowId.settings) && settingsVisible)
        WindowId.settings,
    };
    _secondariesHidden = {};
    return restore;
  }

  /// While active, frame apply must not re-show group-hidden secondaries.
  bool shouldSuppressShow(WindowId id) =>
      _active && _secondariesHidden.contains(id);
}
