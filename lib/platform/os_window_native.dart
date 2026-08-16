import 'dart:ui' show Offset, Size;

import 'package:window_manager/window_manager.dart' show ResizeEdge;

/// Platform HWND / GtkWindow / NSWindow operations for one [OsWindow].
abstract class OsWindowNative {
  Offset getPosition();
  Size getSize();
  void setPosition(double left, double top);
  void setSize(double width, double height);
  void show();
  void hide();
  void setAlwaysOnTop(bool value);
  void setDecorated(bool decorated);
  void setSkipTaskbar(bool skip);
  void setResizable(bool value);
  void startDrag();
  void startResize(ResizeEdge edge);
  void raise({bool focus = true});
  void destroy();
}
