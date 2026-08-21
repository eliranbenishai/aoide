#include "chrome_bodies.h"
#include "chrome_hits.h"
#include "chrome_layout.h"
#include "mockup_draw.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "session_view.h"
#include "tramp_metrics.h"
#include "wait_cursor.h"
#include "window_spec.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>
#include <cmath>
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
  void movingAPanelDoesNotRerasteriseIt();
  void hitRegionsCoverWhatIsPainted();
  void waitCursorRebuildsChassisBeforeRefreshReturns();
  void refreshButtonLightsWhilePlaylistRefreshing();
  void waitCursorFlushShowsRefreshOn();
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

namespace {

/// `HostWindow::logicalFrom`: a click lands on whatever logical pixel the widget
/// pixel divides down to. Going through this is the closest an automated test
/// gets to clicking the chrome at a given zoom.
QPoint logicalAtZoom(QSize logical, int zoomPercent, QPointF widgetPos) {
  const QSize widget = tramp::zoomed(logical, zoomPercent);
  const qreal sx = qreal(widget.width()) / qMax(1, logical.width());
  const qreal sy = qreal(widget.height()) / qMax(1, logical.height());
  return QPoint(int(widgetPos.x() / sx), int(widgetPos.y() / sy));
}

/// First and last widget pixel whose centre falls inside a painted span.
QPair<int, int> paintedPixels(qreal from, qreal to, qreal scale) {
  return {int(std::ceil(from * scale - 0.5)), int(std::ceil(to * scale - 0.5)) - 1};
}

/// Pointer positions on the extremes and the middle of what the chrome paints
/// for one control. The extremes are the whole question: a hit region that
/// stops short of the paint fails there and nowhere else.
QVector<QPointF> paintedSamples(QSize logical, const QRectF& painted, int zoomPercent) {
  const QSize widget = tramp::zoomed(logical, zoomPercent);
  const qreal sx = qreal(widget.width()) / qMax(1, logical.width());
  const qreal sy = qreal(widget.height()) / qMax(1, logical.height());
  const QPair<int, int> xs = paintedPixels(painted.left(), painted.right(), sx);
  const QPair<int, int> ys = paintedPixels(painted.top(), painted.bottom(), sy);
  QVector<QPointF> out;
  for (int x : {xs.first, (xs.first + xs.second) / 2, xs.second}) {
    for (int y : {ys.first, (ys.first + ys.second) / 2, ys.second}) {
      out.push_back(QPointF(x + 0.5, y + 0.5));
    }
  }
  return out;
}

/// Every extreme and midpoint pixel of what the chrome paints for one control
/// must land on that control, at both shipping zooms.
void assertPaintIsGrabbable(tramp::WindowId id, QSize logical, const tramp::SessionView& view,
                            const QRectF& painted, tramp::ChromeHit::Kind kind, int index,
                            const QString& what) {
  for (int zoom : {75, 150}) {
    for (const QPointF& at : paintedSamples(logical, painted, zoom)) {
      const tramp::ChromeHit hit =
          tramp::hitTest(id, logical, logicalAtZoom(logical, zoom, at), view);
      QVERIFY2(hit.kind == kind && hit.index == index,
               qPrintable(QStringLiteral("%1 paints into (%2, %3) at %4%, which is not a hit")
                              .arg(what)
                              .arg(at.x())
                              .arg(at.y())
                              .arg(zoom)));
    }
  }
}

}  // namespace

// Hit geometry and paint geometry used to be derived separately, so they could
// and did drift apart: slider thumbs painted taller than their groove, and a
// fixed-width hit box against a text-measured About pill. Both sides now share
// `chrome_layout.h`, and these walk the painted extent of each control at the
// zoom levels the chrome ships at to keep it that way.
void HostWindowMoveTest::hitRegionsCoverWhatIsPainted() {
  const auto specs = tramp::windowSpecs();
  const tramp::SessionView view;

  const QSize main = specs[0].logicalSize;
  auto grabCoversPaint = [&](tramp::WindowId id, QSize logical, const QRectF& painted,
                             tramp::ChromeHit::Kind kind, const QString& what, int index = -1) {
    assertPaintIsGrabbable(id, logical, view, painted, kind, index, what);
  };

  const tramp::MainDisplayRow display = tramp::layoutMainDisplay(tramp::panelBody(main));
  grabCoversPaint(tramp::WindowId::main, main, display.options, tramp::ChromeHit::Kind::options,
                  "the options cog");
  grabCoversPaint(tramp::WindowId::main, main, display.well, tramp::ChromeHit::Kind::timeToggle,
                  "the display well");

  const tramp::MainVolumeRow vol = tramp::layoutMainVolumeRow(tramp::panelBody(main));
  grabCoversPaint(tramp::WindowId::main, main, vol.mute, tramp::ChromeHit::Kind::mute, "Mute");
  grabCoversPaint(tramp::WindowId::main, main,
                  QRectF(vol.track.left(), vol.track.center().y() - tramp::kVolumeThumbH / 2,
                         vol.track.width(), tramp::kVolumeThumbH),
                  tramp::ChromeHit::Kind::volume, "the volume well and its thumb");
  grabCoversPaint(tramp::WindowId::main, main, vol.mono, tramp::ChromeHit::Kind::mono, "MONO");
  grabCoversPaint(tramp::WindowId::main, main, vol.eq, tramp::ChromeHit::Kind::eqToggle, "EQ");
  grabCoversPaint(tramp::WindowId::main, main, vol.pl, tramp::ChromeHit::Kind::plToggle, "PL");

  const QFontMetricsF stamp{tramp::monoFont(14)};
  const tramp::MainSeekRow seekRow = tramp::layoutMainSeekRow(
      tramp::panelBody(main), stamp.horizontalAdvance(tramp::formatClock(view.positionMs)),
      stamp.horizontalAdvance(tramp::formatClock(view.durationMs)));
  grabCoversPaint(tramp::WindowId::main, main,
                  QRectF(seekRow.track.left(), seekRow.track.center().y() - tramp::kSeekThumbH / 2,
                         seekRow.track.width(), tramp::kSeekThumbH),
                  tramp::ChromeHit::Kind::seek, "the seek well and its thumb");

  // The seek row stamps elapsed time on the left whatever the display well
  // above it is showing, so flipping the well to REMAIN must not move the seek
  // well's left edge. It used to: the hit measured the remaining-time string
  // while the row painted the elapsed one, so the two disagreed by the width of
  // a digit exactly when the digit counts differed.
  {
    tramp::SessionView elapsedShown;
    elapsedShown.durationMs = 1'200'000;  // 20:00
    elapsedShown.positionMs = 599'000;    // 9:59 elapsed, 10:01 remaining
    tramp::SessionView remainShown = elapsedShown;
    remainShown.showElapsed = false;
    QVERIFY2(!qFuzzyCompare(
                 stamp.horizontalAdvance(tramp::formatClock(elapsedShown.positionMs)),
                 stamp.horizontalAdvance(tramp::formatClock(elapsedShown.durationMs -
                                                            elapsedShown.positionMs))),
             "this case only bites while the elapsed and remaining stamps differ in width");

    const int rowY = int(seekRow.row.center().y());
    auto seekHit = [&](const tramp::SessionView& v) {
      for (int x = 0; x < main.width(); ++x) {
        const tramp::ChromeHit hit =
            tramp::hitTest(tramp::WindowId::main, main, QPoint(x, rowY), v);
        if (hit.kind == tramp::ChromeHit::Kind::seek) {
          return hit;
        }
      }
      return tramp::ChromeHit{};
    };
    const tramp::ChromeHit whileElapsed = seekHit(elapsedShown);
    QCOMPARE(whileElapsed.kind, tramp::ChromeHit::Kind::seek);
    QCOMPARE(seekHit(remainShown).rect, whileElapsed.rect);

    const tramp::MainSeekRow remainRow = tramp::layoutMainSeekRow(
        tramp::panelBody(main),
        stamp.horizontalAdvance(tramp::formatClock(elapsedShown.positionMs)),
        stamp.horizontalAdvance(tramp::formatClock(elapsedShown.durationMs)));
    assertPaintIsGrabbable(
        tramp::WindowId::main, main, remainShown,
        QRectF(remainRow.track.left(), remainRow.track.center().y() - tramp::kSeekThumbH / 2,
               remainRow.track.width(), tramp::kSeekThumbH),
        tramp::ChromeHit::Kind::seek, -1,
        QStringLiteral("the seek well while the display well shows REMAIN"));
  }

  const tramp::MainTransportRow transport = tramp::layoutMainTransportRow(
      tramp::panelBody(main), tramp::toggleBtnWidth(QStringLiteral("SHUFFLE")),
      tramp::toggleBtnWidth(QStringLiteral("REPEAT")));
  grabCoversPaint(tramp::WindowId::main, main, transport.prev, tramp::ChromeHit::Kind::prev,
                  "Previous");
  grabCoversPaint(tramp::WindowId::main, main, transport.play, tramp::ChromeHit::Kind::play,
                  "Play");
  grabCoversPaint(tramp::WindowId::main, main, transport.pause, tramp::ChromeHit::Kind::pause,
                  "Pause");
  grabCoversPaint(tramp::WindowId::main, main, transport.stop, tramp::ChromeHit::Kind::stop,
                  "Stop");
  grabCoversPaint(tramp::WindowId::main, main, transport.next, tramp::ChromeHit::Kind::next,
                  "Next");
  grabCoversPaint(tramp::WindowId::main, main, transport.eject, tramp::ChromeHit::Kind::eject,
                  "Eject");
  grabCoversPaint(tramp::WindowId::main, main, transport.shuffle, tramp::ChromeHit::Kind::shuffle,
                  "SHUFFLE");
  grabCoversPaint(tramp::WindowId::main, main, transport.repeat, tramp::ChromeHit::Kind::repeat,
                  "REPEAT");

  const QSize eq = specs[1].logicalSize;
  const tramp::EqHeaderRow header = tramp::layoutEqHeader(
      tramp::panelBody(eq), tramp::labelBtnWidth(QStringLiteral("ON")),
      tramp::labelBtnWidth(QStringLiteral("AUTO")),
      tramp::labelBtnWidth(QStringLiteral("PRESETS"), 16, 22));
  grabCoversPaint(tramp::WindowId::equalizer, eq, header.on, tramp::ChromeHit::Kind::eqOn, "EQ ON");
  grabCoversPaint(tramp::WindowId::equalizer, eq, header.autoBtn, tramp::ChromeHit::Kind::eqAuto,
                  "EQ AUTO");
  grabCoversPaint(tramp::WindowId::equalizer, eq, header.presets,
                  tramp::ChromeHit::Kind::eqPresets, "EQ PRESETS");

  // The gain readout is painted above the well and reads as part of the same
  // control, so it drags the band. The rect the hit carries stays the well:
  // it is the domain the gain is read off, and dragging must not squash it.
  const QRectF bandRow = tramp::eqBandRow(tramp::panelBody(eq));
  for (int i = 0; i < tramp::kEqBandCount; ++i) {
    const tramp::EqBandColumn column = tramp::eqBandColumn(bandRow, i);
    const auto kind = i == 0 ? tramp::ChromeHit::Kind::eqPreamp : tramp::ChromeHit::Kind::eqBand;
    const int index = i == 0 ? -1 : i - 1;
    const QByteArray what =
        (i == 0 ? QStringLiteral("the preamp") : QStringLiteral("band %1").arg(i - 1)).toLatin1();
    grabCoversPaint(tramp::WindowId::equalizer, eq, column.gain, kind, what.constData(), index);
    grabCoversPaint(tramp::WindowId::equalizer, eq, column.well, kind, what.constData(), index);
    // The thumb is centred on the value point, so at the ends of the range half
    // of it stands outside the well it slides in.
    for (qreal gainDb : {12.0, 0.0, -12.0}) {
      grabCoversPaint(tramp::WindowId::equalizer, eq, tramp::bandThumbRect(column.well, gainDb),
                      kind, QStringLiteral("%1's thumb at %2 dB")
                                .arg(QString::fromLatin1(what))
                                .arg(gainDb),
                      index);
    }
    const tramp::ChromeHit hit = tramp::hitTest(tramp::WindowId::equalizer, eq,
                                                column.well.center().toPoint(), view);
    QCOMPARE(hit.rect, column.well.toAlignedRect());
  }

  tramp::ChromeHit volume;
  tramp::ChromeHit seek;
  for (int y = 0; y < main.height(); ++y) {
    const auto hit =
        tramp::hitTest(tramp::WindowId::main, main, QPoint(main.width() / 2, y), view);
    if (hit.kind == tramp::ChromeHit::Kind::volume && volume.rect.isNull()) volume = hit;
    if (hit.kind == tramp::ChromeHit::Kind::seek && seek.rect.isNull()) seek = hit;
  }
  QCOMPARE(volume.kind, tramp::ChromeHit::Kind::volume);
  QCOMPARE(seek.kind, tramp::ChromeHit::Kind::seek);
  QVERIFY2(volume.rect.height() >= int(tramp::kVolumeThumbH),
           "the volume hit region must cover the painted thumb, not just the groove");
  QVERIFY2(seek.rect.height() >= int(tramp::kSeekThumbH),
           "the seek hit region must cover the painted thumb, not just the groove");

  const QSize about = specs[4].logicalSize;
  const qreal textW =
      tramp::textWidth(tramp::monoFont(10), QStringLiteral("tramp.music"));
  tramp::ChromeHit web;
  for (int x = about.width() - 1; x >= 0; --x) {
    const auto hit = tramp::hitTest(tramp::WindowId::about, about,
                                    QPoint(x, about.height() - 38), view);
    if (hit.kind == tramp::ChromeHit::Kind::aboutWeb) {
      web = hit;
      break;
    }
  }
  QCOMPARE(web.kind, tramp::ChromeHit::Kind::aboutWeb);
  QCOMPARE(web.rect.width(), int(tramp::kAboutWebPadX * 2 + textW));
}

// Dragging a panel used to re-run its whole procedural paint on every mouse
// move, which is what made drags crawl. Moving cannot change a panel's pixels,
// so it must come out of the cache; changing its content must not.
void HostWindowMoveTest::movingAPanelDoesNotRerasteriseIt() {
  const auto specs = tramp::windowSpecs();
  HostShell shell;
  HostWindow pl(specs[2], &shell);
  shell.show();
  pl.show();
  QVERIFY(QTest::qWaitForWindowExposed(&pl));

  tramp::SessionView view;
  view.playlistRefreshEnabled = true;
  pl.setSessionView(view);
  const QImage before = pl.grab().toImage();
  pl.resetPaintStats();

  pl.move(pl.pos() + QPoint(11, 7));
  const QImage after = pl.grab().toImage();
  QVERIFY2(pl.paintStats().paints > 0, "the move must actually have repainted");
  QCOMPARE(pl.paintStats().chassisBuilds, 0);
  QVERIFY2(before == after, "a move must not change a panel's pixels");

  pl.resetPaintStats();
  tramp::SessionView changed = view;
  changed.playlistRefreshing = true;
  pl.setSessionView(changed);
  pl.grab();
  QCOMPARE(pl.paintStats().chassisBuilds, 1);

  pl.resetPaintStats();
  pl.resize(pl.size() + QSize(40, 20));
  pl.grab();
  QVERIFY2(pl.paintStats().chassisBuilds >= 1, "a resize must re-rasterise at the new size");
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

void HostWindowMoveTest::waitCursorFlushShowsRefreshOn() {
  const auto specs = tramp::windowSpecs();
  HostShell shell;
  HostWindow pl(specs[2], &shell);
  shell.show();
  pl.show();
  QVERIFY(QTest::qWaitForWindowExposed(&pl));

  tramp::SessionView idle;
  idle.playlistRefreshEnabled = true;
  pl.setSessionView(idle);
  QApplication::processEvents();

  tramp::SessionView busy = idle;
  busy.playlistRefreshing = true;
  const tramp::ChromeHit hit = refreshHit(busy);
  QCOMPARE(hit.kind, tramp::ChromeHit::Kind::plRefresh);

  const tramp::ChromeTokens tokens = tramp::ChromeTokens::builtin();
  auto distanceToOn = [&]() {
    const QImage img = pl.grab(pl.widgetRectFromLogical(hit.rect)).toImage();
    const QPoint sample(img.width() / 2, qMin(4, img.height() - 1));
    return rgbDistance(img.pixel(sample), tokens.btnOn0);
  };
  const int idleToOn = distanceToOn();

  pl.setSessionView(busy);
  {
    tramp::WaitCursorScope wait;
    QVERIFY2(distanceToOn() < idleToOn,
             "Refresh on-face must flush with the wait cursor, not after blocking work");
  }
}

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
