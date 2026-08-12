import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/session/session_quit.dart';

void main() {
  tearDown(() {
    trampExitProcess = exitTrampProcess;
  });

  test('finishSessionQuit persists then exits the process', () async {
    var persisted = false;
    var exitCode = -1;
    trampExitProcess = (code) => exitCode = code;

    await finishSessionQuit(
      persist: () async {
        persisted = true;
      },
    );

    expect(persisted, isTrue);
    expect(exitCode, 0);
  });
}
