#include "host_shell_window.h"
#include "host_window.h"
#include "window_spec.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>
#include <cstdio>

class HostWindowMoveTest : public QObject {
  Q_OBJECT

 private slots:
  void parentedPanelMoveDoesNotEmitNativeMoved();
  void siblingDragDoesNotPayFullClusterPaint();
};

void HostWindowMoveTest::parentedPanelMoveDoesNotEmitNativeMoved() {
  HostShell shell;
  HostWindow panel(tramp::windowSpecs()[0], &shell);
  shell.show();
  panel.show();
  QSignalSpy spy(&panel, &HostWindow::nativeMoved);
  panel.move(40, 20);
  QCOMPARE(spy.count(), 0);
}

void HostWindowMoveTest::siblingDragDoesNotPayFullClusterPaint() {
  const auto specs = tramp::windowSpecs();
  HostShell shell;
  HostWindow main(specs[0], &shell);
  HostWindow pl(specs[2], &shell);
  const QRect mainR(40, 40, specs[0].size.width(), specs[0].size.height());
  const QRect plR(40, 200, specs[2].size.width(), specs[2].size.height());
  shell.placePanels({{&main, mainR}, {&pl, plR}});
  shell.show();
  QApplication::processEvents();

  const QSize logical = specs[2].logicalSize;
  for (int i = 0; i < 20; ++i) pl.setPlaylistLogicalSize(logical);

  QElapsedTimer timer;
  timer.start();
  for (int i = 0; i < 20; ++i) {
    shell.placePanels({{&main, mainR.translated(i, 0)}, {&pl, plR}}, false);
  }
  const qint64 ns = timer.nsecsElapsed();
  std::fprintf(stderr, "drag-path CPU placePanels: %lld ns\n", static_cast<long long>(ns));

  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), plR.topLeft());
  QVERIFY2(ns < 10'000'000,
           "moving a sibling must not pay a full cluster paint (10ms for 20 moves)");
}

QTEST_MAIN(HostWindowMoveTest)
#include "host_window_move_test.moc"
