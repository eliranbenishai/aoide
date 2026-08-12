import 'package:fake_async/fake_async.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/docking/native_drag_tracker.dart';

void main() {
  test('brief quiet during drag does not finalize', () {
    fakeAsync((async) {
      var quietEnds = 0;
      final tracker = NativeDragTracker(
        quietFinalizeDelay: const Duration(milliseconds: 750),
        onQuietFinalize: () => quietEnds++,
      );

      tracker.started();
      expect(tracker.onMoveEvent(), isTrue);
      async.elapse(const Duration(milliseconds: 400));
      expect(tracker.onMoveEvent(), isTrue);
      async.elapse(const Duration(milliseconds: 400));
      expect(quietEnds, 0);
      expect(tracker.isActive, isTrue);

      tracker.endedConfirmed();
      async.elapse(const Duration(milliseconds: 2000));
      expect(quietEnds, 0);
      expect(tracker.isActive, isFalse);
      tracker.dispose();
    });
  });

  test('quiet finalize then further move resumes the drag', () {
    fakeAsync((async) {
      var quietEnds = 0;
      final tracker = NativeDragTracker(
        quietFinalizeDelay: const Duration(milliseconds: 180),
        onQuietFinalize: () => quietEnds++,
      );

      tracker.started();
      expect(tracker.onMoveEvent(), isTrue);
      async.elapse(const Duration(milliseconds: 180));
      expect(quietEnds, 1);
      expect(tracker.isActive, isFalse);
      expect(tracker.softEnded, isTrue);

      // OS is still dragging — next move must reattach, not be ignored.
      expect(tracker.onMoveEvent(), isTrue);
      expect(tracker.isActive, isTrue);
      expect(tracker.softEnded, isFalse);

      tracker.endedConfirmed();
      tracker.dispose();
    });
  });

  test('started arms quiet finalize even without move events', () {
    fakeAsync((async) {
      var quietEnds = 0;
      final tracker = NativeDragTracker(
        quietFinalizeDelay: const Duration(milliseconds: 180),
        onQuietFinalize: () => quietEnds++,
      );

      tracker.started();
      async.elapse(const Duration(milliseconds: 180));
      expect(quietEnds, 1);
      expect(tracker.softEnded, isTrue);
      tracker.dispose();
    });
  });

  test('moves without started are ignored', () {
    final tracker = NativeDragTracker(onQuietFinalize: () {});
    expect(tracker.onMoveEvent(), isFalse);
    tracker.dispose();
  });
}
