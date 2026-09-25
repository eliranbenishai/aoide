#include "wait_cursor.h"

#include <QApplication>
#include <QTest>
#include <QWidget>

#include <cstdio>

extern "C" void aoideBeginColorSpaceCheck();
extern "C" int aoideEndColorSpaceCheck();

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  std::printf("Qt %s (%s)\n", qVersion(), qPrintable(QGuiApplication::platformName()));
  QWidget window;
  window.resize(40, 40);
  window.setCursor(Qt::PointingHandCursor);
  window.show();
  if (!QTest::qWaitForWindowExposed(&window)) return 1;

  // First use of the real Cocoa wait cursor must build its native image.
  // Offscreen tests cannot reach QImage::toCGImage via the platform cursor.
  aoideBeginColorSpaceCheck();
  { aoide::WaitCursorScope wait; }
  const int checked = aoideEndColorSpaceCheck();
  if (checked == 0) {
    std::fputs("FAIL: no native cursor image conversion was checked\n", stderr);
    return 1;
  }
  if (QGuiApplication::overrideCursor() || window.cursor().shape() != Qt::PointingHandCursor) {
    std::fputs("FAIL: wait cursor was not restored\n", stderr);
    return 1;
  }
  std::printf("PASS: %d native cursor image conversion(s) kept their color space alive\n", checked);
  return 0;
}
