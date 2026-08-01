import '../playback/playback_controller.dart';
import 'os_media_controls_stub.dart'
    if (dart.library.io) 'os_media_controls_io.dart';

abstract class OsMediaControls {
  Future<void> start(PlaybackController playback);
  Future<void> stop();
}

OsMediaControls createOsMediaControls() => createOsMediaControlsImpl();
