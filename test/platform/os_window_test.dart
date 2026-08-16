import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/os_window.dart';
import 'package:window_manager/window_manager.dart' show ResizeEdge;

class _RecordingNative implements OsWindowNative {
  Offset position = Offset.zero;
  Size size = Size.zero;
  bool visible = false;
  bool alwaysOnTop = false;
  int moves = 0;
  int resizes = 0;
  int shows = 0;
  int hides = 0;

  @override
  Offset getPosition() => position;

  @override
  Size getSize() => size;

  @override
  void setPosition(double left, double top) {
    moves += 1;
    position = Offset(left, top);
  }

  @override
  void setSize(double width, double height) {
    resizes += 1;
    size = Size(width, height);
  }

  @override
  void show() {
    shows += 1;
    visible = true;
  }

  @override
  void hide() {
    hides += 1;
    visible = false;
  }

  @override
  void setAlwaysOnTop(bool value) => alwaysOnTop = value;

  @override
  void setDecorated(bool decorated) {}

  @override
  void setSkipTaskbar(bool skip) {}

  @override
  void setResizable(bool value) {}

  @override
  void startDrag() {}

  @override
  void startResize(ResizeEdge edge) {}

  @override
  void raise({bool focus = true}) {}

  @override
  void destroy() {}
}

void main() {
  test('position-only frame moves the HWND and does not resize or show', () {
    final native = _RecordingNative();
    final window = OsWindow.test(native);

    window.applyFrame(
      left: 40,
      top: 80,
      width: 619,
      height: 261,
      visible: true,
      alwaysOnTop: false,
      positionOnly: true,
    );

    expect(native.position, const Offset(40, 80));
    expect(native.moves, 1);
    expect(native.resizes, 0);
    expect(native.shows, 0);
    expect(native.hides, 0);
  });

  test('full frame sizes, moves, and maps or hides', () {
    final native = _RecordingNative();
    final window = OsWindow.test(native);

    window.applyFrame(
      left: 10,
      top: 20,
      width: 100,
      height: 50,
      visible: true,
      alwaysOnTop: true,
    );

    expect(native.size, const Size(100, 50));
    expect(native.position, const Offset(10, 20));
    expect(native.visible, isTrue);
    expect(native.alwaysOnTop, isTrue);

    window.applyFrame(
      left: 10,
      top: 20,
      width: 100,
      height: 50,
      visible: false,
      alwaysOnTop: false,
    );
    expect(native.visible, isFalse);
    expect(native.hides, 1);
  });
}
