import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/tramp_window.dart';

void main() {
  test('default playlist window size scales main width and tall well height', () {
    expect(defaultPlaylistWindowSize(100), const Size(824, 660));
    expect(defaultPlaylistWindowSize(200), const Size(1648, 1320));
  });
}
