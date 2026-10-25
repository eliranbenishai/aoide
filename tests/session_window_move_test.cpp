#include "aoide_fonts.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "panel_registry.h"
#include "persist.h"
#include "session.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <vector>

namespace {

// Fake only monitor discovery. The session still moves the real native panel
// widgets through HostShell, including their synchronous and queued events.
class MonitorSession : public aoide::AoideSession {
 public:
  MonitorSession(QString supportDirectory, int rightHeight)
      : AoideSession(std::move(supportDirectory)), rightHeight_(rightHeight) {}

 private:
  QRect hostRect() const override { return leftScreen().united(rightScreen()); }

  QRect workAreaFor(QRect panel) const override {
    return rightScreen().contains(panel.center()) ? rightWork() : leftWork();
  }

  QRect spanningWorkAreaFor(QRect cluster) const override {
    QRect work;
    if (leftScreen().intersects(cluster)) work = leftWork();
    if (rightScreen().intersects(cluster)) work = work.united(rightWork());
    return work;
  }

  static QRect leftScreen() { return QRect(0, 0, 1920, 1200); }
  QRect rightScreen() const { return QRect(1920, 0, 2560, rightHeight_); }
  static QRect leftWork() { return QRect(0, 25, 1920, 1175); }
  QRect rightWork() const { return QRect(1920, 25, 2560, rightHeight_ - 25); }
  int rightHeight_;
};

}  // namespace

class SessionWindowMoveTest : public QObject {
  Q_OBJECT

 private slots:
  void aMainTitleDragCarriesSnappedNativeWindowsAcrossMonitors_data();
  void aMainTitleDragCarriesSnappedNativeWindowsAcrossMonitors();
};

void SessionWindowMoveTest::aMainTitleDragCarriesSnappedNativeWindowsAcrossMonitors_data() {
  QTest::addColumn<int>("rightHeight");
  QTest::addColumn<int>("startY");
  QTest::newRow("taller-destination") << 1440 << 100;
  QTest::newRow("shorter-destination") << 900 << 300;
}

void SessionWindowMoveTest::aMainTitleDragCarriesSnappedNativeWindowsAcrossMonitors() {
  using aoide::WindowId;
  QFETCH(int, rightHeight);
  QFETCH(int, startY);
  QTemporaryDir support;
  QVERIFY(support.isValid());
  aoide::SupportStore store(support.path());
  aoide::AoideSettings settings;
  settings.zoomPercent = 100;
  settings.main = {true, false, 0, double(startY), {}, {}};
  settings.equalizer = {true, false, 0, double(startY + 348), {}, {}};
  settings.playlist = {true, false, 825, double(startY), 1073.0, 696.0};
  settings.dockEdges = {
      {WindowId::equalizer, WindowId::main, aoide::DockSide::top},
      {WindowId::equalizer, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
  };
  QVERIFY(store.writeSettings(settings));
  aoide::loadAoideFonts();

  HostShell shell(aoide::PanelPresentation::nativeWindows);
  std::vector<std::unique_ptr<HostWindow>> ownedWindows;
  aoide::PanelWindows panels;
  for (const aoide::WindowSpec& spec : aoide::windowSpecs()) {
    ownedWindows.push_back(std::make_unique<HostWindow>(spec, &shell));
    HostWindow* window = ownedWindows.back().get();
    shell.preparePanel(window, spec.id == WindowId::main);
    window->setZoomPercent(settings.zoomPercent);
    panels.set(spec.id, window);
  }
  // Construct after the windows so even an early assertion failure destroys
  // the session before the views it references.
  MonitorSession session(support.path(), rightHeight);
  session.setShell(&shell);
  session.setWindows(panels);
  for (const auto& owned : ownedWindows) {
    HostWindow* window = owned.get();
    connect(window, &HostWindow::nativeMoved, &session, [&, window](QPoint position) {
      session.windowMoved(window->id(), position, false);
    });
    connect(window, &HostWindow::titleDragStarted, &session,
            [&, window]() { session.titleDragBegan(window->id()); });
    connect(window, &HostWindow::titleDragFinished, &session,
            [&, window]() { session.titleDragEnded(window->id()); });
    connect(window, &HostWindow::nativeResized, &session, [&, window](QRect frame) {
      if (window->id() == WindowId::playlist) session.playlistResized(frame);
    });
    connect(window, &HostWindow::extraMapped, &session,
            [&, window]() { session.extraWasMapped(window->id()); });
    window->setSessionView(session.view());
  }
  connect(&shell, &HostShell::primaryMoved, &session, [&](QPoint position) {
    session.windowMoved(WindowId::main, position, false);
  });
  session.reapplyWindowFrames();

  HostWindow* main = panels[WindowId::main];
  HostWindow* eq = panels[WindowId::equalizer];
  HostWindow* playlist = panels[WindowId::playlist];
  QVERIFY(QTest::qWaitForWindowExposed(&shell));
  QVERIFY(QTest::qWaitForWindowExposed(eq));
  QVERIFY(QTest::qWaitForWindowExposed(playlist));
  QVERIFY(eq->isWindow());
  QVERIFY(playlist->isWindow());
  // Construction precedes the fake monitor overrides and can fit a restored
  // playlist to the offscreen plugin's display. Establish the test arrangement
  // by the same finalizing move that snaps a listener's panel after mapping.
  session.windowMoved(WindowId::playlist, QPoint(825, startY), true);
  session.persistNow();
  const aoide::AoideSettings snapped = store.readSettings();
  QCOMPARE(snapped.dockEdges.size(), settings.dockEdges.size());
  for (qsizetype i = 0; i < settings.dockEdges.size(); ++i) {
    QCOMPARE(snapped.dockEdges[i].a, settings.dockEdges[i].a);
    QCOMPARE(snapped.dockEdges[i].b, settings.dockEdges[i].b);
    QCOMPARE(snapped.dockEdges[i].side, settings.dockEdges[i].side);
  }
  QCOMPARE(main->nativeTopLeft(), QPoint(0, startY));
  QCOMPARE(eq->nativeTopLeft() - main->nativeTopLeft(), QPoint(0, 348));
  QCOMPARE(playlist->nativeTopLeft() - main->nativeTopLeft(), QPoint(825, 0));
  QCOMPARE(main->size(), QSize(825, 348));
  QCOMPARE(eq->size(), QSize(825, 348));
  QCOMPARE(playlist->size(), QSize(1073, 696));

  QSignalSpy started(main, &HostWindow::titleDragStarted);
  QSignalSpy finished(main, &HostWindow::titleDragFinished);
  const QPoint pressGlobal = main->mapToGlobal(QPoint(100, 10));
  auto sendMouse = [&](QEvent::Type type, QPoint global, Qt::MouseButton button,
                       Qt::MouseButtons buttons) {
    QMouseEvent event(type, QPointF(main->mapFromGlobal(global)), QPointF(global),
                      button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(main, &event);
  };
  sendMouse(QEvent::MouseButtonPress, pressGlobal, Qt::LeftButton, Qt::LeftButton);
  QCOMPARE(started.count(), 1);

  // The first move already straddles the boundary: the playlist's centre is
  // still on the left display while its right edge is on the right display.
  // Continue until the whole arrangement crosses, then take the same path back.
  for (int x : {100, 400, 900, 1400, 1900, 2200, 1900, 1400, 900, 400, 0}) {
    sendMouse(QEvent::MouseMove, pressGlobal + QPoint(x, 0), Qt::NoButton, Qt::LeftButton);
    const int expectedY = x >= 1920 ? qMin(startY, rightHeight - 696) : startY;
    for (int phase = 0; phase < 3; ++phase) {
      if (phase != 0) QTest::qWait(1);
      if (phase == 2) {
        // Native movement can acknowledge a placement after its synchronous
        // guard has ended. A position already applied by the cluster drag must
        // not become a new, individually clamped playlist drag.
        emit playlist->nativeMoved(playlist->nativeTopLeft());
        QCoreApplication::processEvents(QEventLoop::AllEvents);
      }
      QCOMPARE(main->nativeTopLeft(), QPoint(x, expectedY));
      QCOMPARE(eq->nativeTopLeft() - main->nativeTopLeft(), QPoint(0, 348));
      QCOMPARE(playlist->nativeTopLeft() - main->nativeTopLeft(), QPoint(825, 0));
      QCOMPARE(main->size(), QSize(825, 348));
      QCOMPARE(eq->size(), QSize(825, 348));
      QCOMPARE(playlist->size(), QSize(1073, 696));
    }
  }

  sendMouse(QEvent::MouseButtonRelease, pressGlobal, Qt::LeftButton, Qt::NoButton);
  QCoreApplication::processEvents(QEventLoop::AllEvents);
  QCOMPARE(finished.count(), 1);
  QCOMPARE(main->nativeTopLeft(), QPoint(0, startY));
  QCOMPARE(eq->nativeTopLeft(), QPoint(0, startY + 348));
  QCOMPARE(playlist->nativeTopLeft(), QPoint(825, startY));
  QCOMPARE(playlist->size(), QSize(1073, 696));

  session.persistNow();
  const aoide::AoideSettings saved = store.readSettings();
  QCOMPARE(saved.dockEdges.size(), settings.dockEdges.size());
  for (qsizetype i = 0; i < settings.dockEdges.size(); ++i) {
    QCOMPARE(saved.dockEdges[i].a, settings.dockEdges[i].a);
    QCOMPARE(saved.dockEdges[i].b, settings.dockEdges[i].b);
    QCOMPARE(saved.dockEdges[i].side, settings.dockEdges[i].side);
  }
  QVERIFY(saved.playlist.width.has_value());
  QVERIFY(saved.playlist.height.has_value());
  QCOMPARE(*saved.playlist.width, *settings.playlist.width);
  QCOMPARE(*saved.playlist.height, *settings.playlist.height);
  session.detachWindows();
}

QTEST_MAIN(SessionWindowMoveTest)
#include "session_window_move_test.moc"
