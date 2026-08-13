import 'session_messages.dart';

/// The secondary roles a snapshot broadcast may reach.
///
/// A window exists as a `WindowController` from the moment it is created, which
/// is well before its engine has registered a method channel — pushing into
/// that gap throws `CHANNEL_UNREGISTERED`. Playback notifies on every position
/// tick and each notification broadcasts, so a session that resumes into a
/// playing track fires dozens of pushes while the secondaries are still coming
/// up, and every one of them lands on a channel nobody is listening to.
///
/// The handshake is the readiness signal, and it answers with a full set of
/// snapshots for that role, so a broadcast skipped here is not a broadcast
/// lost. Failures that survive this gate are worth printing.
List<WindowRole> secondaryBroadcastRoles({
  required Set<WindowRole> created,
  required Set<WindowRole> ready,
}) {
  return [
    for (final role in WindowRole.values)
      if (role != WindowRole.main &&
          created.contains(role) &&
          ready.contains(role))
        role,
  ];
}
