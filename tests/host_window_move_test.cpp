#include "host_shell_window.h"
#include "host_window.h"
#include "window_spec.h"

#include <QSignalSpy>
#include <QTest>

class HostWindowMoveTest : public QObject {
  Q_OBJECT

 private slots:
  void parentedPanelMoveDoesNotEmitNativeMoved();
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

QTEST_MAIN(HostWindowMoveTest)
#include "host_window_move_test.moc"
