#include "host_shell.h"
#include "host_shell_window.h"

#include <QTest>

class HostShellWindowTest : public QObject {
  Q_OBJECT

 private slots:
  void shellIsFramelessToplevelNotTool();
  void applyLayoutSetsGeometryAndPunchedMask();
  void emptyLayoutHidesTheShell();
  void placePanelsKeepsMainVisibleInTheMask();
  void movingOnePanelDoesNotMoveItsSibling();
  void hostCoversVirtualDesktopNotPanelBbox();
};

void HostShellWindowTest::shellIsFramelessToplevelNotTool() {
  HostShell shell;
  QCOMPARE(shell.windowFlags() & Qt::WindowType_Mask, Qt::WindowFlags(Qt::Window));
  QVERIFY(shell.windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(!shell.windowFlags().testFlag(Qt::Tool));
  QVERIFY(!shell.windowFlags().testFlag(Qt::Dialog));
  QVERIFY(!shell.windowFlags().testFlag(Qt::WindowTransparentForInput));
  QVERIFY(shell.testAttribute(Qt::WA_TranslucentBackground));
  QCOMPARE(shell.windowTitle(), QStringLiteral("Tramp"));
}

void HostShellWindowTest::applyLayoutSetsGeometryAndPunchedMask() {
  HostShell shell;
  tramp::HostShellLayout layout;
  layout.screenRect = QRect(10, 20, 300, 50);
  QRegion mask;
  mask += QRect(0, 0, 100, 50);
  mask += QRect(200, 0, 100, 50);
  layout.localMask = mask;

  shell.applyLayout(layout);

  QCOMPARE(shell.geometry(), tramp::virtualDesktopGeometry());
  QVERIFY(shell.mask().contains(QPoint(10, 10)));
  QVERIFY(shell.mask().contains(QPoint(210, 10)));
  QVERIFY(!shell.mask().contains(QPoint(150, 10)));
}

void HostShellWindowTest::emptyLayoutHidesTheShell() {
  HostShell shell;
  tramp::HostShellLayout visible;
  visible.screenRect = QRect(0, 0, 100, 50);
  visible.localMask = QRegion(QRect(0, 0, 100, 50));
  shell.applyLayout(visible);
  shell.show();
  QVERIFY(shell.isVisible());

  shell.applyLayout({});
  QVERIFY(!shell.isVisible());
  QVERIFY(!shell.mask().isEmpty());
}

void HostShellWindowTest::placePanelsKeepsMainVisibleInTheMask() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect mainR(10, 20, 200, 80);
  const QRect eqR(10, 100, 200, 80);
  const QRect plR(220, 20, 200, 160);
  shell.placePanels({{&main, mainR}, {&eq, eqR}, {&pl, plR}});
  shell.show();

  QVERIFY(main.isVisible());
  QCOMPARE(main.size(), mainR.size());
  QVERIFY(shell.mask().contains(main.geometry().center()));
  QVERIFY(shell.mask().contains(eq.geometry().center()));
  QVERIFY(shell.mask().contains(pl.geometry().center()));
}

void HostShellWindowTest::movingOnePanelDoesNotMoveItsSibling() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eqR(0, 0, 100, 50);
  const QRect plR(200, 0, 100, 80);
  shell.placePanels({{&eq, eqR}, {&pl, plR}});
  shell.show();
  const QPoint pl0 = pl.mapToGlobal(QPoint(0, 0));

  shell.placePanels({{&eq, QRect(40, 20, 100, 50)}, {&pl, plR}});
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), pl0);
}

void HostShellWindowTest::hostCoversVirtualDesktopNotPanelBbox() {
  HostShell shell;
  QWidget main(&shell);
  const QRect mainR(40, 80, 100, 50);
  shell.placePanels({{&main, mainR}});
  shell.show();

  QCOMPARE(shell.geometry(), tramp::virtualDesktopGeometry());
  QCOMPARE(main.mapToGlobal(QPoint(0, 0)), mainR.topLeft());
}

QTEST_MAIN(HostShellWindowTest)
#include "host_shell_window_test.moc"
