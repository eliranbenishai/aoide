import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/ui/session/minimize_group.dart';

void main() {
  group('minimizeGroupTargets', () {
    test('includes every currently visible tramp window', () {
      expect(
        minimizeGroupTargets(
          mainVisible: true,
          equalizerVisible: true,
          playlistVisible: false,
        ),
        [WindowId.main, WindowId.equalizer],
      );
    });

    test('empty when nothing is visible', () {
      expect(
        minimizeGroupTargets(
          mainVisible: false,
          equalizerVisible: false,
          playlistVisible: false,
        ),
        isEmpty,
      );
    });
  });

  group('MinimizeGroupCycle', () {
    test('begin snapshots visible secondaries to hide with main', () {
      final cycle = MinimizeGroupCycle();
      expect(
        cycle.begin(equalizerVisible: true, playlistVisible: true),
        {WindowId.equalizer, WindowId.playlist},
      );
      expect(cycle.isActive, isTrue);
      expect(cycle.shouldSuppressShow(WindowId.equalizer), isTrue);
      expect(cycle.shouldSuppressShow(WindowId.main), isFalse);
    });

    test('begin is idempotent while active', () {
      final cycle = MinimizeGroupCycle();
      cycle.begin(equalizerVisible: true, playlistVisible: false);
      expect(
        cycle.begin(equalizerVisible: false, playlistVisible: true),
        {WindowId.equalizer},
      );
    });

    test('end restores only secondaries still marked visible', () {
      final cycle = MinimizeGroupCycle();
      cycle.begin(equalizerVisible: true, playlistVisible: true);
      expect(
        cycle.end(equalizerVisible: true, playlistVisible: false),
        {WindowId.equalizer},
      );
      expect(cycle.isActive, isFalse);
      expect(cycle.shouldSuppressShow(WindowId.equalizer), isFalse);
    });

    test('end is a no-op when cycle was never begun', () {
      final cycle = MinimizeGroupCycle();
      expect(
        cycle.end(equalizerVisible: true, playlistVisible: true),
        isEmpty,
      );
    });
  });
}
