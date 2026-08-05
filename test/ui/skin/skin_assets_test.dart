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

  test('equalizer_shade_face is 1624x70', () {
    final bytes =
        File('assets/skin/graphite/equalizer_shade_face.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, 1624);
    expect(size.$2, 70);
  });

  // Equalizer controls (Task 7).
  const eqControls = {
    'eq_on_idle': (86, 36),
    'eq_on_active': (86, 36),
    'eq_auto_idle': (90, 36),
    'eq_auto_active': (90, 36),
    'eq_presets_idle': (226, 34),
    'eq_presets_pressed': (226, 34),
    'eq_collapse_idle': (78, 44),
    'eq_collapse_pressed': (78, 44),
    'eq_close_idle': (76, 44),
    'eq_close_pressed': (76, 44),
    'eq_thumb': (68, 46),
  };
  eqControls.forEach((name, dims) {
    test('$name is ${dims.$1}x${dims.$2}', () {
      final bytes =
          File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
      final size = readPngSize(bytes);
      expect(size.$1, dims.$1);
      expect(size.$2, dims.$2);
    });
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
  // bezel shared by zoom-/zoom+, in idle + pressed. All 90x50.
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

  // Mute bezel sprites (OPEN-adjacent well + baked speaker). 130x50.
  for (final state in const ['idle', 'muted', 'pressed']) {
    final name = 'mute_$state';
    test('$name is 130x50', () {
      final bytes =
          File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
      final size = readPngSize(bytes);
      expect(size.$1, 130);
      expect(size.$2, 50);
    });
  }

  // Playlist toolbar label buttons (placeholders). 2× logical sizes.
  const playlistToolbar = {
    'pl_load_idle': (108, 44),
    'pl_load_pressed': (108, 44),
    'pl_save_idle': (108, 44),
    'pl_save_pressed': (108, 44),
    'pl_add_idle': (96, 44),
    'pl_add_pressed': (96, 44),
  };
  playlistToolbar.forEach((name, dims) {
    test('$name is ${dims.$1}x${dims.$2}', () {
      final bytes =
          File('assets/skin/graphite/controls/$name.png').readAsBytesSync();
      final size = readPngSize(bytes);
      expect(size.$1, dims.$1);
      expect(size.$2, dims.$2);
    });
  });

  // Playlist 9-slice regions (Task 8), authored at 2x (logical = px / 2). The
  // 24 px corners match the PlaylistSlices border of 12 logical; edges tile
  // along their long axis and the well tiles both.
  const playlistSlices = {
    'nw': (24, 24),
    'n': (64, 24),
    'ne': (24, 24),
    'w': (24, 64),
    'e': (24, 64),
    'sw': (24, 24),
    's': (64, 24),
    'se': (24, 24),
    'well': (96, 96),
  };
  playlistSlices.forEach((name, dims) {
    test('playlist $name is ${dims.$1}x${dims.$2}', () {
      final bytes =
          File('assets/skin/graphite/playlist/$name.png').readAsBytesSync();
      final size = readPngSize(bytes);
      expect(size.$1, dims.$1);
      expect(size.$2, dims.$2);
    });
  });

  test('slider_thumb is a non-empty crop', () {
    final bytes =
        File('assets/skin/graphite/controls/slider_thumb.png').readAsBytesSync();
    final size = readPngSize(bytes);
    expect(size.$1, greaterThan(0));
    expect(size.$2, greaterThan(0));
  });
}
