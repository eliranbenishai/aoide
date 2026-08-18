#include "host_shell.h"

#include <QTest>

class HostShellTest : public QObject {
  Q_OBJECT

private slots:
  void onePanelIsTheScreenRectAndLocalOriginMask();
  void twoPanelsWithAGapExcludeTheGapFromTheMask();
  void overlappingPanelsUnionInTheMask();
  void emptyInputYieldsNullScreenRectAndEmptyMask();
  void panelNativeSizeUsesLogicalWhenWidgetHasNoSize();
};

void HostShellTest::onePanelIsTheScreenRectAndLocalOriginMask() {
  const auto layout = tramp::hostShellLayout({QRect(10, 20, 100, 50)});
  QCOMPARE(layout.screenRect, QRect(10, 20, 100, 50));
  QCOMPARE(layout.localMask, QRegion(QRect(0, 0, 100, 50)));
}

void HostShellTest::twoPanelsWithAGapExcludeTheGapFromTheMask() {
  const auto layout =
      tramp::hostShellLayout({QRect(0, 0, 100, 50), QRect(200, 0, 100, 50)});
  QCOMPARE(layout.screenRect, QRect(0, 0, 300, 50));
  QVERIFY(layout.localMask.contains(QPoint(10, 10)));
  QVERIFY(layout.localMask.contains(QPoint(210, 10)));
  QVERIFY(!layout.localMask.contains(QPoint(150, 10)));
}

void HostShellTest::overlappingPanelsUnionInTheMask() {
  const auto layout =
      tramp::hostShellLayout({QRect(0, 0, 100, 50), QRect(50, 0, 100, 50)});
  QCOMPARE(layout.screenRect, QRect(0, 0, 150, 50));
  QRegion expected;
  expected += QRect(0, 0, 100, 50);
  expected += QRect(50, 0, 100, 50);
  QCOMPARE(layout.localMask, expected);
}

void HostShellTest::emptyInputYieldsNullScreenRectAndEmptyMask() {
  const auto layout = tramp::hostShellLayout({});
  QVERIFY(layout.screenRect.isNull());
  QVERIFY(layout.localMask.isEmpty());
}

void HostShellTest::panelNativeSizeUsesLogicalWhenWidgetHasNoSize() {
  QCOMPARE(tramp::panelNativeSize(QSize(619, 261), QSize(0, 0)), QSize(619, 261));
}

QTEST_APPLESS_MAIN(HostShellTest)
#include "host_shell_test.moc"
