#include "host_shell.h"
#include "host_shell_window.h"

#include <QTest>

class HostShellWindowTest : public QObject {
  Q_OBJECT

 private slots:
  void shellIsFramelessToplevelNotTool();
  void applyLayoutSetsGeometryAndPunchedMask();
  void emptyLayoutHidesTheShell();
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

  QCOMPARE(shell.geometry(), QRect(10, 20, 300, 50));
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

QTEST_MAIN(HostShellWindowTest)
#include "host_shell_window_test.moc"
