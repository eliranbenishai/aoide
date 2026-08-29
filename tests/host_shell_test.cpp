#include "host_shell.h"

#include <QTest>

class HostShellTest : public QObject {
  Q_OBJECT

private slots:
  void layoutUsesHostRectNotPanelUnion();
  void twoPanelsWithAGapExcludeTheGapFromTheMask();
  void overlappingPanelsUnionInTheMask();
  void emptyInputYieldsNullScreenRectAndEmptyMask();
  void clampKeepsPanelInsideHost();
  void clampShrinksPanelLargerThanHost();
  void clusterDeltaFitsUnionInsideHost();
  void clusterDeltaNullWhenUnionExceedsHost();
  void panelNativeSizeUsesLogicalWhenWidgetHasNoSize();
  void panelLocalUsesActualHostOriginNotRequestedBBox();
  void emptyWorkAreaReservesNoTopLift();
  void aPanelAlreadyBelowTheStripDoesNotMove();
  void aPanelUnderTheStripGetsExactlyTheShortfall();
  void aPanelWhoseTopEqualsTheWorkAreaTopDoesNotMove();
  void aWorkAreaWithANegativeTopLiftsByTheShortfall();
};

void HostShellTest::layoutUsesHostRectNotPanelUnion() {
  const QRect host(0, 0, 1920, 1080);
  const auto layout = aoide::hostShellLayout(host, {QRect(10, 20, 100, 50)});
  QCOMPARE(layout.screenRect, host);
  QCOMPARE(layout.localMask, QRegion(QRect(10, 20, 100, 50)));
}

void HostShellTest::twoPanelsWithAGapExcludeTheGapFromTheMask() {
  const auto layout =
      aoide::hostShellLayout(QRect(0, 0, 800, 600), {QRect(0, 0, 100, 50), QRect(200, 0, 100, 50)});
  QCOMPARE(layout.screenRect, QRect(0, 0, 800, 600));
  QVERIFY(layout.localMask.contains(QPoint(10, 10)));
  QVERIFY(layout.localMask.contains(QPoint(210, 10)));
  QVERIFY(!layout.localMask.contains(QPoint(150, 10)));
}

void HostShellTest::overlappingPanelsUnionInTheMask() {
  const auto layout =
      aoide::hostShellLayout(QRect(0, 0, 400, 200), {QRect(0, 0, 100, 50), QRect(50, 0, 100, 50)});
  QCOMPARE(layout.screenRect, QRect(0, 0, 400, 200));
  QRegion expected;
  expected += QRect(0, 0, 100, 50);
  expected += QRect(50, 0, 100, 50);
  QCOMPARE(layout.localMask, expected);
}

void HostShellTest::emptyInputYieldsNullScreenRectAndEmptyMask() {
  const auto layout = aoide::hostShellLayout(QRect(0, 0, 800, 600), {});
  QVERIFY(layout.screenRect.isNull());
  QVERIFY(layout.localMask.isEmpty());
}

void HostShellTest::clampKeepsPanelInsideHost() {
  const QRect host(0, 0, 200, 100);
  QCOMPARE(aoide::clampRectToHost(QRect(-20, -10, 50, 40), host), QRect(0, 0, 50, 40));
  QCOMPARE(aoide::clampRectToHost(QRect(180, 80, 50, 40), host), QRect(150, 60, 50, 40));
}

void HostShellTest::clampShrinksPanelLargerThanHost() {
  const QRect host(10, 20, 100, 80);
  QCOMPARE(aoide::clampRectToHost(QRect(0, 0, 400, 300), host), QRect(10, 20, 100, 80));
}

void HostShellTest::clusterDeltaFitsUnionInsideHost() {
  const QRect host(0, 0, 200, 100);
  const auto delta = aoide::clusterDeltaToFit({QRect(180, 10, 40, 20), QRect(190, 40, 30, 20)}, host);
  QVERIFY(delta.has_value());
  QCOMPARE(*delta, QPoint(-20, 0));
}

void HostShellTest::clusterDeltaNullWhenUnionExceedsHost() {
  const QRect host(0, 0, 100, 50);
  QVERIFY(!aoide::clusterDeltaToFit({QRect(0, 0, 80, 40), QRect(50, 20, 80, 40)}, host).has_value());
}

void HostShellTest::panelNativeSizeUsesLogicalWhenWidgetHasNoSize() {
  QCOMPARE(aoide::panelNativeSize(QSize(619, 261), QSize(0, 0)), QSize(619, 261));
}

void HostShellTest::panelLocalUsesActualHostOriginNotRequestedBBox() {
  const QPoint actualHost(100, 40);
  const QPoint siblingScreen(200, 40);
  const QPoint requestedOrigin(40, 40);
  QCOMPARE(aoide::panelLocalTopLeft(siblingScreen, actualHost), QPoint(100, 0));
  QVERIFY(aoide::panelLocalTopLeft(siblingScreen, requestedOrigin) != QPoint(100, 0));
}

void HostShellTest::emptyWorkAreaReservesNoTopLift() {
  QCOMPARE(aoide::reservedTopLift(QRect(0, 0, 825, 348), QRect()), 0);
}

void HostShellTest::aPanelAlreadyBelowTheStripDoesNotMove() {
  QCOMPARE(aoide::reservedTopLift(QRect(40, 80, 825, 348), QRect(0, 25, 1920, 1055)), 0);
}

void HostShellTest::aPanelUnderTheStripGetsExactlyTheShortfall() {
  QCOMPARE(aoide::reservedTopLift(QRect(0, 0, 825, 348), QRect(0, 25, 1920, 1055)), 25);
  QCOMPARE(aoide::reservedTopLift(QRect(100, -10, 480, 360), QRect(0, 25, 1920, 1055)), 35);
}

void HostShellTest::aPanelWhoseTopEqualsTheWorkAreaTopDoesNotMove() {
  QCOMPARE(aoide::reservedTopLift(QRect(0, 25, 825, 348), QRect(0, 25, 1920, 1055)), 0);
}

void HostShellTest::aWorkAreaWithANegativeTopLiftsByTheShortfall() {
  const QRect upperWork(0, -1055, 1920, 1055);
  QCOMPARE(aoide::reservedTopLift(QRect(40, -1080, 825, 348), upperWork), 25);
  QCOMPARE(aoide::reservedTopLift(QRect(40, -1000, 825, 348), upperWork), 0);
}

QTEST_APPLESS_MAIN(HostShellTest)
#include "host_shell_test.moc"
