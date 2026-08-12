import 'dart:ffi';
import 'dart:io';

/// Set to `1` to quit as soon as secondary engines are ready (latency harness).
bool trampAutoQuitRequested() =>
    Platform.environment['TRAMP_AUTO_QUIT'] == '1';

/// Process terminator used after a confirmed quit.
///
/// Tests replace this so they can observe the exit without killing the VM.
void Function(int code) trampExitProcess = exitTrampProcess;

/// Leave the process without running Flutter/GTK engine destructors.
///
/// `dart:io` [exit] still runs `atexit` handlers. On Linux those include
/// per-window FlView / compositor teardown, which is what made close take
/// several seconds. `_exit` skips that and lets the OS reap the process.
void exitTrampProcess(int code) {
  try {
    DynamicLibrary.process()
        .lookupFunction<Void Function(Int32), void Function(int)>('_exit')(
      code,
    );
  } catch (_) {
    exit(code);
  }
}

/// Persist, then terminate. Does not destroy secondary Flutter engines.
Future<void> finishSessionQuit({
  required Future<void> Function() persist,
}) async {
  await persist();
  trampExitProcess(0);
}
