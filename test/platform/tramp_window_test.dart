import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/tramp_window.dart';

void main() {
  group('trampWindowSizeMatches', () {
    test('accepts exact match', () {
      expect(
        trampWindowSizeMatches(const Size(619, 261), const Size(619, 261)),
        isTrue,
      );
    });

    test('accepts within default 2px tolerance', () {
      expect(
        trampWindowSizeMatches(const Size(620, 262), const Size(619, 261)),
        isTrue,
      );
    });

    test('rejects oversized GTK default vs zoomed canvas', () {
      expect(
        trampWindowSizeMatches(const Size(1280, 720), const Size(619, 261)),
        isFalse,
      );
    });
  });
}
