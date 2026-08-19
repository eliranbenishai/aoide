#include "host_shell.h"
#include "host_shell_window.h"

#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QTest>

class HostShellWindowTest : public QObject {
  Q_OBJECT

 private slots:
  void shellIsFramelessToplevelNotTool();
  void shellAdvertisesAppLogoOnTheTaskbar();
  void applyLayoutSetsGeometryAndPunchedMask();
  void emptyLayoutHidesTheShell();
  void placePanelsKeepsMainVisibleInTheMask();
  void movingOnePanelDoesNotMoveItsSibling();
  void hostContainsPanelsAtRequestedScreenPositions();
  void translatingEveryPanelLeavesTheHostPut();
  void movingOnePanelDoesNotResizeTheHost();
  void expandingPastTopLeftKeepsSiblingOnScreen();
  void deferredPunchStillAppliesWhenLayoutAlreadyCaughtUp();
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

void HostShellWindowTest::shellAdvertisesAppLogoOnTheTaskbar() {
  HostShell shell;
  const QIcon icon = shell.windowIcon();
  QVERIFY2(!icon.isNull(), "host is the taskbar/pager entry and must carry the app logo");
  const QPixmap px = icon.pixmap(QSize(32, 32));
  QVERIFY(!px.isNull());
  QVERIFY(px.width() >= 16);
  QVERIFY(px.height() >= 16);
  const QImage img = px.toImage().convertToFormat(QImage::Format_ARGB32);
  int opaque = 0;
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (qAlpha(img.pixel(x, y)) > 16) ++opaque;
    }
  }
  QVERIFY2(opaque > 0, "taskbar icon must be painted logo pixels, not an empty pixmap");
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

void HostShellWindowTest::hostContainsPanelsAtRequestedScreenPositions() {
  HostShell shell;
  QWidget main(&shell);
  const QRect mainR(40, 80, 100, 50);
  shell.placePanels({{&main, mainR}});
  shell.show();

  QVERIFY(shell.isVisible());
  QVERIFY(main.isVisible());
  QVERIFY(shell.rect().contains(main.geometry()));
  QCOMPARE(main.mapToGlobal(QPoint(0, 0)), mainR.topLeft());
  QCOMPARE(shell.geometry(), shell.virtualDesktop());
}

void HostShellWindowTest::translatingEveryPanelLeavesTheHostPut() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eqR(0, 0, 100, 50);
  const QRect plR(200, 0, 100, 80);
  shell.placePanels({{&eq, eqR}, {&pl, plR}});
  shell.show();
  const QRect host0 = shell.geometry();
  QCOMPARE(host0, shell.virtualDesktop());

  const QRect eq1(30, 40, 100, 50);
  const QRect pl1(230, 40, 100, 80);
  shell.placePanels({{&eq, eq1}, {&pl, pl1}});
  QCOMPARE(shell.geometry(), host0);
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), eq1.topLeft());
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), pl1.topLeft());
}

void HostShellWindowTest::movingOnePanelDoesNotResizeTheHost() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  shell.placePanels({{&eq, QRect(0, 0, 100, 50)}, {&pl, QRect(200, 0, 100, 80)}});
  shell.show();
  const QSize hostSize = shell.size();

  const QRect plCloser(100, 0, 100, 80);
  shell.placePanels({{&eq, QRect(0, 0, 100, 50)}, {&pl, plCloser}});
  QCOMPARE(shell.size(), hostSize);
  QCOMPARE(shell.geometry(), shell.virtualDesktop());
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), QPoint(0, 0));
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), plCloser.topLeft());
}

void HostShellWindowTest::expandingPastTopLeftKeepsSiblingOnScreen() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eq0(40, 40, 100, 50);
  const QRect pl0(200, 40, 100, 80);
  shell.placePanels({{&eq, eq0}, {&pl, pl0}});
  shell.show();
  const QPoint plScreen = pl.mapToGlobal(QPoint(0, 0));
  const QPoint hostPos = shell.pos();

  const QRect eq1(10, 10, 100, 50);
  shell.placePanels({{&eq, eq1}, {&pl, pl0}});
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), plScreen);
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), eq1.topLeft());
  QCOMPARE(shell.pos(), hostPos);
}

void HostShellWindowTest::deferredPunchStillAppliesWhenLayoutAlreadyCaughtUp() {
  HostShell shell;
  QWidget panel(&shell);
  const QRect start(40, 40, 120, 60);
  const QRect end(200, 80, 120, 60);
  shell.placePanels({{&panel, start}});
  shell.show();
  const QPoint startCenter = panel.geometry().center();
  QVERIFY(shell.mask().contains(startCenter));

  shell.placePanels({{&panel, end}}, false);
  QCOMPARE(panel.mapToGlobal(QPoint(0, 0)), end.topLeft());
  QVERIFY2(!shell.mask().isEmpty(),
           "empty mask is full input on Wayland; punch must stay a panel union");
  QVERIFY2(shell.mask().contains(startCenter),
           "updatePunch=false must keep the previous punch, not follow the move");
  QVERIFY2(!shell.mask().contains(panel.geometry().center()),
           "deferred punch must not jump to the new rect until updatePunch=true");

  shell.placePanels({{&panel, end}}, true);
  QVERIFY(shell.mask().contains(panel.geometry().center()));
  QVERIFY(!shell.mask().contains(startCenter));
  QVERIFY(!shell.mask().contains(QPoint(10, 10)));
}

QTEST_MAIN(HostShellWindowTest)
#include "host_shell_window_test.moc"
