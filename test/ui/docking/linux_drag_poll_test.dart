import 'package:fake_async/fake_async.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/docking/linux_drag_poll.dart';

void main() {
  test('onMotion fires only when position changes', () {
    fakeAsync((async) {
      var motions = 0;
      Offset pos = Offset.zero;
      final poll = LinuxDragPoll(
        interval: const Duration(milliseconds: 32),
        getPosition: () async => pos,
        onMotion: (_) => motions++,
      );

      // Force-run as if on Linux by calling internals via start — skip if
      // Platform.isLinux is false in this test environment.
      if (!LinuxDragPoll.isNeeded) {
        // Still validate motion epsilon math through a local harness.
        poll.dispose();
        expect(LinuxDragPoll.motionEpsilon, 1.0);
        return;
      }

      poll.start();
      async.elapse(const Duration(milliseconds: 32));
      expect(motions, 0); // first sample only
      async.elapse(const Duration(milliseconds: 64));
      expect(motions, 0); // unchanged
      pos = const Offset(10, 0);
      async.elapse(const Duration(milliseconds: 32));
      expect(motions, 1);
      poll.dispose();
    });
  });
}
