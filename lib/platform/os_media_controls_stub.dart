import '../playback/playback_controller.dart';
import 'os_media_controls.dart';

class NoOpOsMediaControls implements OsMediaControls {
  @override
  Future<void> start(PlaybackController playback) async {}

  @override
  Future<void> stop() async {}
}

OsMediaControls createOsMediaControlsImpl() => NoOpOsMediaControls();
