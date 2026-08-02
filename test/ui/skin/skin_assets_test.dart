import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

/// Reads a PNG's pixel dimensions straight from the IHDR chunk.
///
/// A PNG begins with the 8-byte signature, then the IHDR chunk:
/// 4-byte length, 4-byte type ("IHDR"), then the 13-byte data whose first
/// eight bytes are the big-endian width and height. So width occupies bytes
/// 16..19 and height bytes 20..23 of the file. Reading these avoids pulling in
/// an image-decoding dependency just to assert dimensions.
(int, int) readPngSize(List<int> bytes) {
  const signature = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
  if (bytes.length < 24) {
    throw const FormatException('Not a PNG: file too short');
  }
  for (var i = 0; i < signature.length; i++) {
    if (bytes[i] != signature[i]) {
      throw const FormatException('Not a PNG: bad signature');
    }
  }
  int be32(int offset) =>
      (bytes[offset] << 24) |
      (bytes[offset + 1] << 16) |
      (bytes[offset + 2] << 8) |
      bytes[offset + 3];
  return (be32(16), be32(20));
}

void main() {
  test('main_face is 1624x484', () {
    final bytes =
        File('assets/skin/graphite/main_face.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, 1624);
    expect(size.$2, 484);
  });

  test('equalizer_face is 1624x412', () {
    final bytes =
        File('assets/skin/graphite/equalizer_face.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, 1624);
    expect(size.$2, 412);
  });

  // Transport buttons are logical 69x40, authored at 2x -> 138x80.
  const transportNames = [
    'transport_prev', 'transport_play', 'transport_pause',
    'transport_stop', 'transport_next',
  ];
  for (final base in transportNames) {
    for (final state in const ['idle', 'pressed']) {
      final name = '${base}_$state';
      test('$name is 138x80', () {
        final bytes =
            File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
        final size = readPngSize(bytes);
        expect(size.$1, 138);
        expect(size.$2, 80);
      });
    }
  }

  // Toggle sprites: shuffle/repeat 152x58, EQ/PL 114x40, in idle + active.
  const toggles = {
    'shuffle': (152, 58),
    'repeat': (152, 58),
    'eq': (114, 40),
    'pl': (114, 40),
  };
  toggles.forEach((base, dims) {
    for (final state in const ['idle', 'active']) {
      final name = '${base}_$state';
      test('$name is ${dims.$1}x${dims.$2}', () {
        final bytes =
            File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
        final size = readPngSize(bytes);
        expect(size.$1, dims.$1);
        expect(size.$2, dims.$2);
      });
    }
  });

  // Title-bar window bezels: minimize / close (baked glyphs) and the blank
  // bezel shared by zoom-/zoom+ and mute, in idle + pressed. All 90x50.
  const windowBezels = ['win_minimize', 'win_close', 'win_blank'];
  for (final base in windowBezels) {
    for (final state in const ['idle', 'pressed']) {
      final name = '${base}_$state';
      test('$name is 90x50', () {
        final bytes =
            File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
        final size = readPngSize(bytes);
        expect(size.$1, 90);
        expect(size.$2, 50);
      });
    }
  }

  test('slider_thumb is a non-empty crop', () {
    final bytes =
        File('assets/skin/graphite/controls/slider_thumb.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, greaterThan(0));
    expect(size.$2, greaterThan(0));
  });
}
