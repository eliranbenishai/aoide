import 'dart:ui';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/harness_flags.dart';

void main() {
  test('trampWindowFill opaque argument is graphite, default is transparent', () {
    expect(trampWindowFill(opaque: true), const Color(0xFF1A1A1A));
    expect(trampWindowFill(opaque: false), const Color(0x00000000));
  });
}
