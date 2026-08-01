import '../playback/playback_controller.dart';
import 'os_media_controls.dart';

/// Linux OS media keys via MPRIS (session D-Bus).
///
/// TODO(v1): Register an MPRIS `org.mpris.MediaPlayer2` player on the session
/// bus exposing Play, Pause, PlayPause, Next, Previous, Stop, and metadata.
class LinuxOsMediaControls implements OsMediaControls {
  @override
  Future<void> start(PlaybackController playback) async {}

  @override
  Future<void> stop() async {}
}
