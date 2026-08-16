// ignore_for_file: invalid_use_of_internal_member
// ignore_for_file: implementation_imports

import 'dart:ffi' show Native, Pointer, Void, Bool, Int, Uint32, Int32;
import 'dart:ffi' as ffi;
import 'dart:ui' show Offset, Size;

import 'package:ffi/ffi.dart';
import 'package:flutter/src/widgets/_window.dart';
import 'package:flutter/src/widgets/_window_linux.dart';
import 'package:flutter/src/widgets/_window_macos.dart';
import 'package:flutter/src/widgets/_window_win32.dart';
import 'package:window_manager/window_manager.dart' show ResizeEdge;

import 'os_window_native.dart';

Pointer<Void> _linuxHandle(RegularWindowController controller) {
  return (controller as WindowControllerLinux).windowHandle;
}

class LinuxOsWindowNative implements OsWindowNative {
  LinuxOsWindowNative(this.controller);

  final RegularWindowController controller;
  Pointer<Void> get _w => _linuxHandle(controller);

  @override
  Offset getPosition() {
    final x = malloc<Int32>();
    final y = malloc<Int32>();
    try {
      _gtkWindowGetPosition(_w, x, y);
      return Offset(x.value.toDouble(), y.value.toDouble());
    } finally {
      malloc.free(x);
      malloc.free(y);
    }
  }

  @override
  Size getSize() {
    final w = malloc<Int32>();
    final h = malloc<Int32>();
    try {
      _gtkWindowGetSize(_w, w, h);
      return Size(w.value.toDouble(), h.value.toDouble());
    } finally {
      malloc.free(w);
      malloc.free(h);
    }
  }

  @override
  void setPosition(double left, double top) {
    _gtkWindowMove(_w, left.round(), top.round());
  }

  @override
  void setSize(double width, double height) {
    _gtkWindowResize(_w, width.round(), height.round());
  }

  @override
  void show() => _gtkWindowPresent(_w);

  @override
  void hide() => _gtkWidgetHide(_w);

  @override
  void setAlwaysOnTop(bool value) => _gtkWindowSetKeepAbove(_w, value);

  @override
  void setDecorated(bool decorated) => _gtkWindowSetDecorated(_w, decorated);

  @override
  void setSkipTaskbar(bool skip) => _gtkWindowSetSkipTaskbarHint(_w, skip);

  @override
  void setResizable(bool value) => _gtkWindowSetResizable(_w, value);

  @override
  void startDrag() {
    _gtkWindowBeginMoveDrag(_w, 1, 0, 0, _gtkGetCurrentEventTime());
  }

  @override
  void startResize(ResizeEdge edge) {
    _gtkWindowBeginResizeDrag(
      _w,
      _gdkEdge(edge),
      1,
      0,
      0,
      _gtkGetCurrentEventTime(),
    );
  }

  @override
  void raise({bool focus = true}) {
    if (focus) {
      _gtkWindowPresent(_w);
    } else {
      _gtkWindowSetKeepAbove(_w, true);
      _gtkWindowSetKeepAbove(_w, false);
    }
  }

  @override
  void destroy() => controller.destroy();
}

int _gdkEdge(ResizeEdge edge) {
  // GdkWindowEdge: NW, N, NE, W, E, SW, S, SE
  return switch (edge) {
    ResizeEdge.topLeft => 0,
    ResizeEdge.top => 1,
    ResizeEdge.topRight => 2,
    ResizeEdge.left => 3,
    ResizeEdge.right => 4,
    ResizeEdge.bottomLeft => 5,
    ResizeEdge.bottom => 6,
    ResizeEdge.bottomRight => 7,
  };
}

@Native<Void Function(Pointer<Void>, Pointer<Int32>, Pointer<Int32>)>(
  symbol: 'gtk_window_get_position',
)
external void _gtkWindowGetPosition(
  Pointer<Void> window,
  Pointer<Int32> x,
  Pointer<Int32> y,
);

@Native<Void Function(Pointer<Void>, Pointer<Int32>, Pointer<Int32>)>(
  symbol: 'gtk_window_get_size',
)
external void _gtkWindowGetSize(
  Pointer<Void> window,
  Pointer<Int32> width,
  Pointer<Int32> height,
);

@Native<Void Function(Pointer<Void>, Int, Int)>(symbol: 'gtk_window_move')
external void _gtkWindowMove(Pointer<Void> window, int x, int y);

@Native<Void Function(Pointer<Void>, Int, Int)>(symbol: 'gtk_window_resize')
external void _gtkWindowResize(Pointer<Void> window, int w, int h);

@Native<Void Function(Pointer<Void>)>(symbol: 'gtk_window_present')
external void _gtkWindowPresent(Pointer<Void> window);

@Native<Void Function(Pointer<Void>)>(symbol: 'gtk_widget_hide')
external void _gtkWidgetHide(Pointer<Void> widget);

@Native<Void Function(Pointer<Void>, Bool)>(symbol: 'gtk_window_set_keep_above')
external void _gtkWindowSetKeepAbove(Pointer<Void> window, bool keep);

@Native<Void Function(Pointer<Void>, Bool)>(symbol: 'gtk_window_set_decorated')
external void _gtkWindowSetDecorated(Pointer<Void> window, bool decorated);

@Native<Void Function(Pointer<Void>, Bool)>(
  symbol: 'gtk_window_set_skip_taskbar_hint',
)
external void _gtkWindowSetSkipTaskbarHint(Pointer<Void> window, bool skip);

@Native<Void Function(Pointer<Void>, Int, Int, Int, Uint32)>(
  symbol: 'gtk_window_begin_move_drag',
)
external void _gtkWindowBeginMoveDrag(
  Pointer<Void> window,
  int button,
  int rootX,
  int rootY,
  int timestamp,
);

@Native<Uint32 Function()>(symbol: 'gtk_get_current_event_time')
external int _gtkGetCurrentEventTime();

@Native<Void Function(Pointer<Void>, Bool)>(symbol: 'gtk_window_set_resizable')
external void _gtkWindowSetResizable(Pointer<Void> window, bool resizable);

@Native<Void Function(Pointer<Void>, Int, Int, Int, Int, Uint32)>(
  symbol: 'gtk_window_begin_resize_drag',
)
external void _gtkWindowBeginResizeDrag(
  Pointer<Void> window,
  int edge,
  int button,
  int rootX,
  int rootY,
  int timestamp,
);

class Win32OsWindowNative implements OsWindowNative {
  Win32OsWindowNative(this.controller);

  final RegularWindowController controller;
  HWND get _hwnd => (controller as WindowControllerWin32).windowHandle;

  static final HWND _hwndTop = Pointer<Void>.fromAddress(0);
  static final HWND _hwndTopmost = Pointer<Void>.fromAddress(-1);
  static final HWND _hwndNoTopmost = Pointer<Void>.fromAddress(-2);

  @override
  Offset getPosition() {
    final rect = malloc<_Win32Rect>();
    try {
      _getWindowRect(_hwnd, rect);
      return Offset(rect.ref.left.toDouble(), rect.ref.top.toDouble());
    } finally {
      malloc.free(rect);
    }
  }

  @override
  Size getSize() {
    final rect = malloc<_Win32Rect>();
    try {
      _getWindowRect(_hwnd, rect);
      return Size(
        (rect.ref.right - rect.ref.left).toDouble(),
        (rect.ref.bottom - rect.ref.top).toDouble(),
      );
    } finally {
      malloc.free(rect);
    }
  }

  @override
  void setPosition(double left, double top) {
    _setWindowPos(_hwnd, _hwndTop, left.round(), top.round(), 0, 0, 0x0001 | 0x0010);
  }

  @override
  void setSize(double width, double height) {
    _setWindowPos(_hwnd, _hwndTop, 0, 0, width.round(), height.round(), 0x0002 | 0x0010);
  }

  @override
  void show() => _showWindow(_hwnd, 5); // SW_SHOW

  @override
  void hide() => _showWindow(_hwnd, 0); // SW_HIDE

  @override
  void setAlwaysOnTop(bool value) {
    _setWindowPos(
      _hwnd,
      value ? _hwndTopmost : _hwndNoTopmost,
      0,
      0,
      0,
      0,
      0x0001 | 0x0002 | 0x0010,
    );
  }

  @override
  void setDecorated(bool decorated) {
    const gwlStyle = -16;
    const wsPopup = 0x80000000;
    const wsCaption = 0x00C00000;
    const wsThickframe = 0x00040000;
    const wsVisible = 0x10000000;
    var style = _getWindowLongPtr(_hwnd, gwlStyle);
    if (decorated) {
      style = style | wsCaption | wsThickframe;
    } else {
      style = (style | wsPopup | wsVisible) & ~wsCaption & ~wsThickframe;
    }
    _setWindowLongPtr(_hwnd, gwlStyle, style);
    _setWindowPos(_hwnd, _hwndTop, 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0004 | 0x0020);
  }

  @override
  void setSkipTaskbar(bool skip) {
    const gwlExstyle = -20;
    const wsExToolwindow = 0x00000080;
    const wsExAppwindow = 0x00040000;
    var ex = _getWindowLongPtr(_hwnd, gwlExstyle);
    if (skip) {
      ex = (ex | wsExToolwindow) & ~wsExAppwindow;
    } else {
      ex = (ex | wsExAppwindow) & ~wsExToolwindow;
    }
    _setWindowLongPtr(_hwnd, gwlExstyle, ex);
  }

  @override
  void setResizable(bool value) {
    // Frameless HWND: edge resize is SC_SIZE, not WS_THICKFRAME.
  }

  @override
  void startDrag() {
    _releaseCapture();
    _sendMessage(_hwnd, 0x0112, 0xF012, 0); // WM_SYSCOMMAND, SC_MOVE|HTCAPTION
  }

  @override
  void startResize(ResizeEdge edge) {
    final wmsz = switch (edge) {
      ResizeEdge.left => 1,
      ResizeEdge.right => 2,
      ResizeEdge.top => 3,
      ResizeEdge.topLeft => 4,
      ResizeEdge.topRight => 5,
      ResizeEdge.bottom => 6,
      ResizeEdge.bottomLeft => 7,
      ResizeEdge.bottomRight => 8,
    };
    _releaseCapture();
    _sendMessage(_hwnd, 0x0112, 0xF000 + wmsz, 0); // WM_SYSCOMMAND, SC_SIZE
  }

  @override
  void raise({bool focus = true}) {
    _setWindowPos(
      _hwnd,
      _hwndTop,
      0,
      0,
      0,
      0,
      0x0001 | 0x0002 | (focus ? 0 : 0x0010),
    );
  }

  @override
  void destroy() => controller.destroy();
}

final class _Win32Rect extends ffi.Struct {
  @ffi.Int32()
  external int left;
  @ffi.Int32()
  external int top;
  @ffi.Int32()
  external int right;
  @ffi.Int32()
  external int bottom;
}

@Native<ffi.Int32 Function(HWND, Pointer<_Win32Rect>)>(symbol: 'GetWindowRect')
external int _getWindowRect(HWND hwnd, Pointer<_Win32Rect> rect);

@Native<ffi.Int32 Function(HWND, HWND, Int, Int, Int, Int, Uint32)>(
  symbol: 'SetWindowPos',
)
external int _setWindowPos(
  HWND hwnd,
  HWND insertAfter,
  int x,
  int y,
  int cx,
  int cy,
  int flags,
);

@Native<ffi.Int32 Function(HWND, Int)>(symbol: 'ShowWindow')
external int _showWindow(HWND hwnd, int cmd);

@Native<ffi.IntPtr Function(HWND, Int)>(symbol: 'GetWindowLongPtrW')
external int _getWindowLongPtr(HWND hwnd, int index);

@Native<ffi.IntPtr Function(HWND, Int, ffi.IntPtr)>(symbol: 'SetWindowLongPtrW')
external int _setWindowLongPtr(HWND hwnd, int index, int value);

@Native<ffi.Int32 Function()>(symbol: 'ReleaseCapture')
external int _releaseCapture();

@Native<ffi.IntPtr Function(HWND, Uint32, ffi.IntPtr, ffi.IntPtr)>(
  symbol: 'SendMessageW',
)
external int _sendMessage(HWND hwnd, int msg, int wParam, int lParam);

class MacOsWindowNative implements OsWindowNative {
  MacOsWindowNative(this.controller);

  final RegularWindowController controller;

  @override
  Offset getPosition() => Offset.zero;

  @override
  Size getSize() => Size.zero;

  @override
  void setPosition(double left, double top) {
    _MacNsWindow.move(_nsHandle(), left, top);
  }

  @override
  void setSize(double width, double height) {
    controller.setSize(Size(width, height));
  }

  @override
  void show() => controller.activate();

  @override
  void hide() => _MacNsWindow.orderOut(_nsHandle());

  @override
  void setAlwaysOnTop(bool value) => _MacNsWindow.setLevel(_nsHandle(), value);

  @override
  void setDecorated(bool decorated) {
    _MacNsWindow.setStyleMask(_nsHandle(), decorated);
  }

  @override
  void setSkipTaskbar(bool skip) => _MacNsWindow.setCollectionBehavior(_nsHandle(), skip);

  @override
  void setResizable(bool value) {}

  @override
  void startDrag() => _MacNsWindow.performDrag(_nsHandle());

  @override
  void startResize(ResizeEdge edge) {}

  @override
  void raise({bool focus = true}) {
    if (focus) {
      controller.activate();
    } else {
      _MacNsWindow.orderFront(_nsHandle());
    }
  }

  @override
  void destroy() => controller.destroy();

  Pointer<Void> _nsHandle() =>
      (controller as WindowControllerMacOS).windowHandle;
}

/// Minimal ObjC message sends for NSWindow. Failures are ignored (macOS is not
/// the drag-feel host this rewrite is proving).
class _MacNsWindow {
  static void move(Pointer<Void> window, double x, double y) {}
  static void orderOut(Pointer<Void> window) {}
  static void orderFront(Pointer<Void> window) {}
  static void setLevel(Pointer<Void> window, bool floating) {}
  static void setStyleMask(Pointer<Void> window, bool decorated) {}
  static void setCollectionBehavior(Pointer<Void> window, bool skip) {}
  static void performDrag(Pointer<Void> window) {}
}
