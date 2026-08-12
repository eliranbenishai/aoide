import 'dart:io';

/// Opens [uri] with the OS handler (`xdg-open` / `open` / `cmd start`).
///
/// Only `http` and `https` are accepted so this cannot be used as a general
/// process launcher.
Future<void> openExternalUrl(Uri uri) async {
  if (uri.scheme != 'https' && uri.scheme != 'http') {
    throw ArgumentError.value(uri, 'uri', 'only http(s) URLs');
  }
  final url = uri.toString();
  if (Platform.isLinux) {
    await Process.start('xdg-open', [url], mode: ProcessStartMode.detached);
  } else if (Platform.isMacOS) {
    await Process.start('open', [url], mode: ProcessStartMode.detached);
  } else if (Platform.isWindows) {
    await Process.start(
      'cmd',
      ['/c', 'start', '', url],
      mode: ProcessStartMode.detached,
    );
  }
}
