import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/docking/dock_move_coalescer.dart';

void main() {
  group('DockMoveCoalescer', () {
    test('collapses overlapping schedules into one extra apply pass', () async {
      final coalescer = DockMoveCoalescer();
      var applies = 0;
      final gate = Completer<void>();

      coalescer.schedule(() async {
        applies++;
        await gate.future;
      });

      // While first apply is blocked, many pointer updates arrive.
      coalescer.schedule(() async {
        applies++;
      });
      coalescer.schedule(() async {
        applies++;
      });
      coalescer.schedule(() async {
        applies++;
      });

      expect(coalescer.isBusy, isTrue);
      expect(applies, 1);

      gate.complete();
      await Future<void>.delayed(Duration.zero);
      await Future<void>.delayed(Duration.zero);

      // One in-flight + one coalesced follow-up — not one per schedule.
      expect(applies, 2);
      expect(coalescer.isBusy, isFalse);
    });

    test('flush awaits in-flight work then runs the final apply', () async {
      final coalescer = DockMoveCoalescer();
      final order = <String>[];
      final gate = Completer<void>();

      coalescer.schedule(() async {
        order.add('drag');
        await gate.future;
      });

      final flushed = coalescer.flush(() async {
        order.add('end');
      });

      expect(order, ['drag']);
      gate.complete();
      await flushed;

      expect(order, ['drag', 'end']);
      expect(coalescer.isBusy, isFalse);
    });
  });
}
