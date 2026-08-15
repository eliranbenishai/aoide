import 'dart:io';

/// Append-only startup trace. Lives in the temp dir so a failed boot can
/// still write before support-dir / plugins are up.
void trampStartupLog(String message) {
  try {
    File('${Directory.systemTemp.path}${Platform.pathSeparator}tramp-startup.log')
        .writeAsStringSync(
      '${DateTime.now().toIso8601String()} $message\n',
      mode: FileMode.append,
    );
  } catch (_) {}
}
