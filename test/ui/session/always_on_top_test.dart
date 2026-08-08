import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/ui/session/always_on_top.dart';

void main() {
  group('effectiveAlwaysOnTop', () {
    test('requires both global flag and visibility', () {
      expect(
        effectiveAlwaysOnTop(alwaysOnTop: true, visible: true),
        isTrue,
      );
      expect(
        effectiveAlwaysOnTop(alwaysOnTop: true, visible: false),
        isFalse,
      );
      expect(
        effectiveAlwaysOnTop(alwaysOnTop: false, visible: true),
        isFalse,
      );
    });
  });

  group('alwaysOnTopTargets', () {
    test('empty when global flag off', () {
      expect(
        alwaysOnTopTargets(
          alwaysOnTop: false,
          mainVisible: true,
          equalizerVisible: true,
          playlistVisible: true,
        ),
        isEmpty,
      );
    });

    test('fans out only to visible windows', () {
      expect(
        alwaysOnTopTargets(
          alwaysOnTop: true,
          mainVisible: true,
          equalizerVisible: false,
          playlistVisible: true,
        ),
        [WindowId.main, WindowId.playlist],
      );
    });

    test('includes all three when all visible', () {
      expect(
        alwaysOnTopTargets(
          alwaysOnTop: true,
          mainVisible: true,
          equalizerVisible: true,
          playlistVisible: true,
        ),
        [WindowId.main, WindowId.equalizer, WindowId.playlist],
      );
    });
  });
}
