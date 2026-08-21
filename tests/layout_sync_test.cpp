#include "layout_sync.h"

#include <QTest>

using tramp::DockLayout;
using tramp::LayoutSync;
using tramp::WindowId;

class LayoutSyncTest : public QObject {
  Q_OBJECT

 private slots:
  void nativeAndLogicalAreInversesAcrossTheZoomLadder();
  void logicalToNativeRoundsRatherThanTruncates();
};

void LayoutSyncTest::nativeAndLogicalAreInversesAcrossTheZoomLadder() {
  for (int percent : {75, 100, 125, 150}) {
    LayoutSync layout({}, percent);
    QCOMPARE(layout.zoomPercent(), percent);
    const QPoint native(400, 240);
    QCOMPARE(layout.logicalToNative(layout.nativeToLogical(native)), native);
  }
}

void LayoutSyncTest::logicalToNativeRoundsRatherThanTruncates() {
  LayoutSync layout({}, 75);
  // 825 * 0.75 = 618.75. Truncating loses most of a pixel per panel, and the
  // cluster walks a pixel left every time a drag round-trips through here.
  QCOMPARE(layout.logicalToNative(QPointF(825, 348)), QPoint(619, 261));
}

QTEST_APPLESS_MAIN(LayoutSyncTest)
#include "layout_sync_test.moc"
