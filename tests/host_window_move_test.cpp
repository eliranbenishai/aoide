#include "chrome_bodies.h"
#include "chrome_hits.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "session_view.h"
#include "tramp_metrics.h"
#include "wait_cursor.h"
#include "window_spec.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>
#include <cstdio>
#include <cstdlib>

class PaintCountHost : public HostWindow {
 public:
  using HostWindow::HostWindow;
  int paints = 0;

 protected:
  void paintEvent(QPaintEvent* event) override {
    ++paints;
    HostWindow::paintEvent(event);
  }
};

class HostWindowMoveTest : public QObject {
  Q_OBJECT

 private slots:
  void parentedPanelMoveDoesNotEmitNativeMoved();
  void siblingDragDoesNotPayFullClusterPaint();
  void waitCursorRebuildsChassisBeforeRefreshReturns();
  void refreshButtonLightsWhilePlaylistRefreshing();
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

void HostWindowMoveTest::waitCursorRebuildsChassisBeforeRefreshReturns() {
  HostShell shell;
  PaintCountHost panel(tramp::windowSpecs()[0], &shell);
  shell.show();
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  const tramp::SessionView view = tramp::SessionView::golden();
  panel.setSessionView(view);
  QApplication::processEvents();
  {
    tramp::WaitCursorScope wait;
    const int beforeRefresh = panel.paints;
    panel.setSessionView(view);
    QVERIFY2(panel.paints > beforeRefresh,
             "skin refresh must rebuild chrome before the wait cursor drops");
  }
}

namespace {

QImage paintPlaylistPanel(const tramp::SessionView& view) {
  const QSize logical = tramp::kPlaylistDefault;
  QImage img(logical, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::black);
  QPainter p(&img);
  tramp::paintWindowBody(p, tramp::WindowId::playlist, logical, nullptr, view);
  return img;
}

tramp::ChromeHit refreshHit(const tramp::SessionView& view) {
  const QSize logical = tramp::kPlaylistDefault;
  for (int y = logical.height() - 90; y < logical.height(); ++y) {
    for (int x = logical.width() - 50; x < logical.width(); ++x) {
      const tramp::ChromeHit hit =
          tramp::hitTest(tramp::WindowId::playlist, logical, QPoint(x, y), view);
      if (hit.kind == tramp::ChromeHit::Kind::plRefresh) return hit;
    }
  }
  return {};
}

int rgbDistance(QRgb a, const QColor& b) {
  const QColor c = QColor::fromRgba(a);
  return std::abs(c.red() - b.red()) + std::abs(c.green() - b.green()) +
         std::abs(c.blue() - b.blue());
}

}  // namespace

void HostWindowMoveTest::refreshButtonLightsWhilePlaylistRefreshing() {
  tramp::SessionView idle;
  idle.playlistRefreshEnabled = true;
  tramp::SessionView busy = idle;
  busy.playlistRefreshing = true;

  const tramp::ChromeHit hit = refreshHit(busy);
  QCOMPARE(hit.kind, tramp::ChromeHit::Kind::plRefresh);
  QVERIFY(hit.rect.isValid());

  const QPoint sample(hit.rect.center().x(), hit.rect.top() + 4);
  const QImage idleImg = paintPlaylistPanel(idle);
  const QImage busyImg = paintPlaylistPanel(busy);
  const tramp::ChromeTokens tokens = tramp::ChromeTokens::builtin();
  const int idleToOn = rgbDistance(idleImg.pixel(sample), tokens.btnOn0);
  const int busyToOn = rgbDistance(busyImg.pixel(sample), tokens.btnOn0);
  QVERIFY2(busyToOn < idleToOn,
           "Refresh must use the on face while playlistRefreshing is set");
}

QTEST_MAIN(HostWindowMoveTest)
#include "host_window_move_test.moc"
