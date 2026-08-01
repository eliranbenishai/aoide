import 'dart:io';

import 'os_media_controls.dart';
import 'os_media_controls_linux.dart';
import 'os_media_controls_macos.dart';
import 'os_media_controls_stub.dart';
import 'os_media_controls_windows.dart';

OsMediaControls createOsMediaControlsImpl() {
  if (Platform.isWindows) {
    return WindowsOsMediaControls();
  }
  if (Platform.isLinux) {
    return LinuxOsMediaControls();
  }
  if (Platform.isMacOS) {
    return MacOsMediaControls();
  }
  return NoOpOsMediaControls();
}
