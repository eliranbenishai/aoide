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
}
