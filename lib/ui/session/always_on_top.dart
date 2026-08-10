import '../../domain/tramp_settings.dart';

/// Effective OS always-on-top for one tramp window.
///
/// The settings flag is global; only **visible** windows should be pinned.
bool effectiveAlwaysOnTop({
  required bool alwaysOnTop,
  required bool visible,
}) =>
    alwaysOnTop && visible;

/// Window ids that should currently have OS always-on-top enabled.
List<WindowId> alwaysOnTopTargets({
  required bool alwaysOnTop,
  required bool mainVisible,
  required bool equalizerVisible,
  required bool playlistVisible,
  bool settingsVisible = false,
}) {
  if (!alwaysOnTop) return const [];
  return [
    if (mainVisible) WindowId.main,
    if (equalizerVisible) WindowId.equalizer,
    if (playlistVisible) WindowId.playlist,
    if (settingsVisible) WindowId.settings,
  ];
}
