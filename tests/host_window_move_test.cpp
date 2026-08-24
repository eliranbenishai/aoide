#include "chrome_bodies.h"
#include "chrome_hits.h"
#include "chrome_layout.h"
#include "chrome_paint.h"
#include "mockup_draw.h"
#include "title_chrome.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "session_view.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
#include "wait_cursor.h"
#include "window_spec.h"

#include <QApplication>
#include <QBrush>
#include <QElapsedTimer>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QSignalSpy>
#include <QTest>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>

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
  void hitRegionsDoNotOverlap();
  void waitCursorRepaintsEvenWhenTheRasterIsKept();
  void onlyThePanelsThatPaintAChangeRerasteriseForIt();
  void aKeptRasterIsNeverOneThatWentStale();
  void refreshButtonLightsWhilePlaylistRefreshing();
  void refreshLampLightsOnTheLiveEventLoop();
  void goldenDemoPaintsTheStateItIsHanded();
  void mockupHelpersLeaveThePainterAsTheyFoundIt();
  void panelPaintersLeaveThePainterAsTheyFoundIt();
  void panelPaintersDrawWithWhatTheySet();
  void emptyStateCopyIsTheLockedCopy();
  void paintsSameFlipsWhenAnEmptyListGainsARow();
  void paintsSameFlipsWhenSkinRadiiChange();
  void emptyWellsAreNotBlank();
  void unmeasuredSpectrumMarkFollowsTheSpectrogram();
  void missingEngineMarkStaysOnTheDisplayWell();
  void persistFailureMarkStaysUntilAWriteSucceeds();
  void skinsErrorStaysOnTheSkinsStrip();
  void eqCurveWellIgnoresPreamp();
  void wordmarkKeepsBrandFaceWhenChromeFontChanges();
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
    shell.placePanels({{&main, mainR.translated(i, 0)}, {&pl, plR}});
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

/// What one control won when a panel was walked pixel by pixel.
struct ClaimedRegion {
  QRect bounds;
  QRect carried;
  int pixels = 0;
};

/// Which control wins each logical pixel of a panel. `hitTest` answers with the
/// first region that claims a point, so a region that overlaps one checked
/// before it silently loses the shared pixels — and that shows up here as a
/// region winning fewer pixels than its own bounds hold.
QMap<QPair<int, int>, ClaimedRegion> claimedRegions(tramp::WindowId id, QSize logical,
                                                    const tramp::SessionView& view) {
  QMap<QPair<int, int>, ClaimedRegion> out;
  for (int y = 0; y < logical.height(); ++y) {
    for (int x = 0; x < logical.width(); ++x) {
      const tramp::ChromeHit hit = tramp::hitTest(id, logical, QPoint(x, y), view);
      if (hit.kind == tramp::ChromeHit::Kind::none) {
        continue;
      }
      ClaimedRegion& claimed = out[{int(hit.kind), hit.index}];
      const QRect pixel(x, y, 1, 1);
      claimed.bounds = claimed.pixels == 0 ? pixel : claimed.bounds.united(pixel);
      claimed.carried = hit.rect;
      ++claimed.pixels;
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
  grabCoversPaint(tramp::WindowId::main, main, display.skins, tramp::ChromeHit::Kind::skins,
                  "the skins button");
  grabCoversPaint(tramp::WindowId::main, main, display.trackInfo, tramp::ChromeHit::Kind::trackInfo,
                  "the track info button");
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
  const qreal textW = tramp::textWidth(tramp::monoFont(10), QStringLiteral("tramp.music"));
  const QRectF pill = tramp::aboutWebPill(
      tramp::aboutMakerPlate(tramp::aboutInner(tramp::panelBody(about))), textW);
  grabCoversPaint(tramp::WindowId::about, about, pill, tramp::ChromeHit::Kind::aboutWeb,
                  "the tramp.music pill");
  const tramp::ChromeHit web =
      tramp::hitTest(tramp::WindowId::about, about, pill.center().toPoint(), view);
  QCOMPARE(web.kind, tramp::ChromeHit::Kind::aboutWeb);
  // Official 6.8.3 (CI) gives a fractional advance; int(pad*2+textW) then
  // disagrees with the hit by a pixel. The region is the outward-rounded paint
  // rect, same as the equaliser wells above.
  QCOMPARE(web.rect, pill.toAlignedRect());
}

// Covering the paint means rounding hit regions outwards, which grows them, and
// plenty of controls sit a few pixels from a neighbour — SHUFFLE beside REPEAT,
// each equaliser band beside the next. `hitTest` answers with the first region
// that claims a point, so a region that has grown into its neighbour takes the
// shared pixels and nothing says so.
//
// The walk is in logical space because that is where the regions live: a click
// at any zoom divides down to a logical pixel, so a panel that has no overlap
// here has none at any zoom. What zoom does change is which logical pixels a
// pointer can reach, so each region is then required to still win its corners
// through the widget-pixel division at both shipping zooms.
void HostWindowMoveTest::hitRegionsDoNotOverlap() {
  const auto specs = tramp::windowSpecs();

  // The value a slider drags is read off its well, so those four carry the well
  // and grab a larger area around it; every other control grabs what it carries.
  const QSet<int> grabsWiderThanItCarries{
      int(tramp::ChromeHit::Kind::volume), int(tramp::ChromeHit::Kind::seek),
      int(tramp::ChromeHit::Kind::eqPreamp), int(tramp::ChromeHit::Kind::eqBand)};

  auto panelHoldsItsRegionsApart = [&](tramp::WindowId id, QSize logical,
                                       const tramp::SessionView& view, const QString& what,
                                       const QSet<int>& knownToLose = {}) {
    const auto claimed = claimedRegions(id, logical, view);
    QVERIFY2(!claimed.isEmpty(), qPrintable(what + QStringLiteral(" offers no hit regions at all")));
    for (auto it = claimed.constBegin(); it != claimed.constEnd(); ++it) {
      const QString where = QStringLiteral("%1 hit kind %2 index %3, bounded by %4x%5 at (%6, %7),")
                                .arg(what)
                                .arg(it.key().first)
                                .arg(it.key().second)
                                .arg(it->bounds.width())
                                .arg(it->bounds.height())
                                .arg(it->bounds.left())
                                .arg(it->bounds.top());
      if (knownToLose.contains(it.key().first)) {
        continue;
      }
      QVERIFY2(it->pixels == it->bounds.width() * it->bounds.height(),
               qPrintable(QStringLiteral("%1 wins %2 of the %3 pixels inside it, so a region "
                                         "checked before it is taking the rest")
                              .arg(where)
                              .arg(it->pixels)
                              .arg(it->bounds.width() * it->bounds.height())));

      // A region losing a whole edge to a neighbour would still win a solid
      // block, just a smaller one, so compare against the rect it hands out.
      const QRect carried = it->carried & QRect(QPoint(), logical);
      if (grabsWiderThanItCarries.contains(it.key().first)) {
        QVERIFY2(it->bounds.contains(carried),
                 qPrintable(where + QStringLiteral(" no longer reaches around its own well")));
      } else {
        QVERIFY2(it->bounds == carried,
                 qPrintable(QStringLiteral("%1 wins %2x%3 at (%4, %5) of the rect it hands out")
                                .arg(where)
                                .arg(carried.width())
                                .arg(carried.height())
                                .arg(carried.left())
                                .arg(carried.top())));
      }

      for (int zoom : {75, 150}) {
        for (const QPointF& at : paintedSamples(logical, it->bounds, zoom)) {
          const tramp::ChromeHit hit =
              tramp::hitTest(id, logical, logicalAtZoom(logical, zoom, at), view);
          QVERIFY2(int(hit.kind) == it.key().first && hit.index == it.key().second,
                   qPrintable(where + QStringLiteral(" loses (%1, %2) at %3%")
                                          .arg(at.x())
                                          .arg(at.y())
                                          .arg(zoom)));
        }
      }
    }
  };

  const tramp::SessionView plain;
  panelHoldsItsRegionsApart(tramp::WindowId::main, specs[0].logicalSize, plain,
                            QStringLiteral("the main panel's"));
  panelHoldsItsRegionsApart(tramp::WindowId::equalizer, specs[1].logicalSize, plain,
                            QStringLiteral("the equaliser's"));
  panelHoldsItsRegionsApart(tramp::WindowId::about, specs[4].logicalSize, plain,
                            QStringLiteral("the about panel's"));

  // The playlist and settings only grow their rows and buttons once there is
  // something to list, so the empty panel is the weaker case of the two.
  tramp::SessionView listed = plain;
  listed.collection = {{QStringLiteral("Nights"), 12, true, false},
                       {QStringLiteral("Drive"), 8, false, false}};
  // One more track than the well shows: the row past the bottom is painted
  // clipped, and its hit region is the one that could reach the deck below.
  const int overflowing = tramp::playlistVisibleRows(
      tramp::playlistListWell(
          tramp::playlistListRowRect(tramp::playlistTrackInner(tramp::playlistTracksPane(
              tramp::panelBody(specs[2].logicalSize), listed.collectionWidth))),
          99)
          .height()) + 4;
  listed.tracks.clear();
  for (int i = 0; i < overflowing; ++i) {
    listed.tracks.push_back({QStringLiteral("Artist"), QStringLiteral("Track %1").arg(i),
                             QStringLiteral("3:20"), false, false, false});
  }
  panelHoldsItsRegionsApart(tramp::WindowId::playlist, specs[2].logicalSize, plain,
                            QStringLiteral("the empty playlist's"));
  panelHoldsItsRegionsApart(tramp::WindowId::playlist, specs[2].logicalSize, listed,
                            QStringLiteral("the full playlist's"));
  tramp::SessionView collapsed = listed;
  collapsed.collectionCollapsed = true;
  panelHoldsItsRegionsApart(tramp::WindowId::playlist, specs[2].logicalSize, collapsed,
                            QStringLiteral("the collapsed playlist's"));

  panelHoldsItsRegionsApart(tramp::WindowId::settings, specs[3].logicalSize, plain,
                            QStringLiteral("the general settings'"));
  tramp::SessionView skins = plain;
  skins.settingsTab = 1;
  skins.skins = {{QStringLiteral("builtin"), QStringLiteral("Built-in"), {}},
                 {QStringLiteral("dusk"), QStringLiteral("Dusk"), {}}};
  skins.skins[1].canRemove = true;
  panelHoldsItsRegionsApart(tramp::WindowId::settings, specs[3].logicalSize, skins,
                            QStringLiteral("the audio settings'"));
  panelHoldsItsRegionsApart(tramp::WindowId::skins, specs[5].logicalSize, skins,
                            QStringLiteral("the skins panel's"),
                            {int(tramp::ChromeHit::Kind::settingsSkinRow)});
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

namespace {

QString panelLabel(tramp::WindowId id) {
  switch (id) {
    case tramp::WindowId::main: return QStringLiteral("main");
    case tramp::WindowId::equalizer: return QStringLiteral("eq");
    case tramp::WindowId::playlist: return QStringLiteral("playlist");
    case tramp::WindowId::settings: return QStringLiteral("settings");
    case tramp::WindowId::about: return QStringLiteral("about");
    case tramp::WindowId::skins: return QStringLiteral("skins");
  }
  return QStringLiteral("?");
}

QSize panelLogicalSize(tramp::WindowId id) {
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    if (spec.id == id) return spec.logicalSize;
  }
  return {};
}

const std::array<tramp::WindowId, 6>& everyPanel() {
  static const std::array<tramp::WindowId, 6> ids = {
      tramp::WindowId::main, tramp::WindowId::equalizer, tramp::WindowId::playlist,
      tramp::WindowId::settings, tramp::WindowId::about, tramp::WindowId::skins};
  return ids;
}

}  // namespace

// A wait cursor says a blocking load is about to start, and this is the chrome's
// last chance to reach the screen before it does. The panel is handed a view it
// already has, so the raster is kept — and the repaint still has to happen, or
// the pre-load chrome never shows at all.
void HostWindowMoveTest::waitCursorRepaintsEvenWhenTheRasterIsKept() {
  HostShell shell;
  PaintCountHost panel(tramp::windowSpecs()[0], &shell);
  shell.show();
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  tramp::SessionView view = tramp::goldenDemoView();
  // The golden flag takes a panel off the cache altogether, and the cache is
  // the thing under test.
  view.goldenDemo = false;
  panel.setSessionView(view);
  QApplication::processEvents();
  panel.resetPaintStats();
  {
    tramp::WaitCursorScope wait;
    const int beforeRefresh = panel.paints;
    panel.setSessionView(view);
    QVERIFY2(panel.paints > beforeRefresh,
             "the pre-load chrome must reach the screen before the wait cursor drops");
    QCOMPARE(panel.paintStats().chassisBuilds, 0);
  }
}

// The two the ticket names: a playlist scroll rebuilds the playlist raster and
// no other, and MONO rebuilds main's and no other. Asserted through the panels
// rather than through the comparison, because it is the raster that costs.
void HostWindowMoveTest::onlyThePanelsThatPaintAChangeRerasteriseForIt() {
  HostShell shell;
  std::vector<std::unique_ptr<HostWindow>> panels;
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    panels.push_back(std::make_unique<HostWindow>(spec, &shell));
  }
  shell.show();
  for (auto& panel : panels) panel->show();
  QVERIFY(QTest::qWaitForWindowExposed(panels.front().get()));

  tramp::SessionView base = tramp::goldenDemoView();
  base.goldenDemo = false;
  auto publish = [&](const tramp::SessionView& view) {
    for (auto& panel : panels) panel->setSessionView(view);
    for (auto& panel : panels) panel->grab();
  };
  auto rebuilt = [&]() {
    QStringList names;
    for (auto& panel : panels) {
      if (panel->paintStats().chassisBuilds > 0) names << panelLabel(panel->id());
    }
    return names.join(QLatin1Char('+'));
  };

  publish(base);
  for (auto& panel : panels) panel->resetPaintStats();
  tramp::SessionView scrolled = base;
  scrolled.trackScroll = 3;
  publish(scrolled);
  QCOMPARE(rebuilt(), QStringLiteral("playlist"));

  publish(base);
  for (auto& panel : panels) panel->resetPaintStats();
  // MONO last: it is latched, so main cross-fades and keeps rebuilding for
  // kBtnTransitionMs afterwards. Nothing is measured after this.
  tramp::SessionView mono = base;
  mono.forceMono = true;
  publish(mono);
  QCOMPARE(rebuilt(), QStringLiteral("main"));
}

namespace {

QImage paintPanel(tramp::WindowId id, QSize logical, const tramp::SessionView& view) {
  QImage img(logical, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::black);
  QPainter p(&img);
  tramp::paintWindowBody(p, id, logical, nullptr, view);
  return img;
}

/// What a panel actually keeps in its raster. Main and the equaliser cache only
/// their static chrome and redraw the live layer every frame; the other four
/// have no live layer, so the whole paint is what sits in the cache. Comparing
/// the full paint for all six would hold main to pixels its cache never held.
QImage paintCachedPass(tramp::WindowId id, QSize logical, const tramp::SessionView& view) {
  const bool live =
      id == tramp::WindowId::main || id == tramp::WindowId::equalizer;
  QImage img(logical, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::black);
  QPainter p(&img);
  tramp::paintWindowBody(p, id, logical, nullptr, view,
                         live ? tramp::BodyPaint::chassis : tramp::BodyPaint::full);
  return img;
}

QImage paintPlaylistPanel(const tramp::SessionView& view) {
  return paintPanel(tramp::WindowId::playlist, tramp::kPlaylistDefault, view);
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

struct FieldChange {
  const char* what;
  /// The panels that must re-rasterise for it, '+' joined in panel order. An
  /// empty string is a field no painter reads.
  const char* rebuilds;
  std::function<void(tramp::SessionView&)> apply;
};

/// Every field on the snapshot, and who repaints for it. The roll call is the
/// point: a field missing from the list below is a field nobody has decided
/// about, and `paintsSame` will not compile until someone has.
std::vector<FieldChange> everyFieldOfTheSnapshot() {
  const char* all = "main+eq+playlist+settings+about+skins";
  return {
      // The shell and the title bar, which every panel wears.
      {"goldenDemo", all, [](tramp::SessionView& v) { v.goldenDemo = true; }},
      {"zoomPercent", all, [](tramp::SessionView& v) { v.zoomPercent = 100; }},
      {"look", all,
       [](tramp::SessionView& v) { v.look.palette.phos = QColor(1, 2, 3); }},

      // Main: the display well, the meta row, the clusters under it.
      {"eqOn", "main", [](tramp::SessionView& v) { v.eqOn = !v.eqOn; }},
      {"plOn", "main", [](tramp::SessionView& v) { v.plOn = !v.plOn; }},
      {"skinsOn", "main", [](tramp::SessionView& v) { v.skinsOn = !v.skinsOn; }},
      {"trackInfoEnabled", "main",
       [](tramp::SessionView& v) { v.trackInfoEnabled = !v.trackInfoEnabled; }},
      {"showElapsed", "main", [](tramp::SessionView& v) { v.showElapsed = !v.showElapsed; }},
      {"positionMs", "main", [](tramp::SessionView& v) { v.positionMs += 4000; }},
      {"durationMs", "main", [](tramp::SessionView& v) { v.durationMs += 4000; }},
      {"title", "main", [](tramp::SessionView& v) { v.title = QStringLiteral("Other"); }},
      {"subtitle", "main", [](tramp::SessionView& v) { v.subtitle = QStringLiteral("OTHER"); }},
      {"bitrate", "main", [](tramp::SessionView& v) { v.bitrate = QStringLiteral("320 kbps"); }},
      {"sampleRate", "main", [](tramp::SessionView& v) { v.sampleRate = QStringLiteral("48 kHz"); }},
      {"channels", "main", [](tramp::SessionView& v) { v.channels = QStringLiteral("MONO"); }},
      {"formatChip", "main", [](tramp::SessionView& v) { v.formatChip = QStringLiteral("FLAC"); }},
      {"volume", "main", [](tramp::SessionView& v) { v.volume = 0.2; }},
      {"muted", "main", [](tramp::SessionView& v) { v.muted = !v.muted; }},
      {"forceMono", "main", [](tramp::SessionView& v) { v.forceMono = !v.forceMono; }},
      {"paused", "main", [](tramp::SessionView& v) { v.paused = !v.paused; }},
      {"shuffle", "main", [](tramp::SessionView& v) { v.shuffle = !v.shuffle; }},
      {"repeat", "main", [](tramp::SessionView& v) { v.repeat = tramp::RepeatMode::one; }},
      {"spectrum", "main", [](tramp::SessionView& v) { v.spectrum[4] = 0.11; }},
      {"spectrumPeaks", "main", [](tramp::SessionView& v) { v.spectrumPeaks[4] = 0.99; }},
      {"spectrumUnmeasured", "main",
       [](tramp::SessionView& v) { v.spectrumUnmeasured = !v.spectrumUnmeasured; }},
      {"noAudioEngine", "main",
       [](tramp::SessionView& v) { v.noAudioEngine = !v.noAudioEngine; }},

      // The marquee clock free-runs, so charging main for every value of it
      // would cost main its cache for as long as a track is loaded. It reaches
      // the cache only when the hold at the start of the line ends: past that
      // the line is painted on the live pass, and the frames are not main's
      // raster to keep.
      {"titleScrollMs while the line is already moving", "",
       [](tramp::SessionView& v) { v.titleScrollMs += 1000; }},
      {"titleScrollMs when the hold ends", "main",
       [](tramp::SessionView& v) { v.titleScrollMs = 0; }},

      // Playing lights a transport face on main and the deck button on the
      // playlist, so it is the one field two panels latch.
      {"playing", "main+playlist", [](tramp::SessionView& v) { v.playing = !v.playing; }},
      // As the marquee it runs on main; as a switch it is painted on settings.
      {"scrollTitle", "main+settings", [](tramp::SessionView& v) { v.scrollTitle = !v.scrollTitle; }},

      {"eq", "eq", [](tramp::SessionView& v) { v.eq.gains[2] = 7.5; }},

      // Playlist: the collection column, the list, the deck and the footer.
      {"tracks", "playlist",
       [](tramp::SessionView& v) { v.tracks[0].title = QStringLiteral("Renamed"); }},
      // Emptiness is what the empty-well copy and the main title swap read.
      // Clearing the demo's rows keeps its named title, so only the playlist
      // chassis turns over — main stays until the title is `No track` too.
      {"tracks emptied", "playlist", [](tramp::SessionView& v) { v.tracks.clear(); }},
      {"collection emptied", "playlist",
       [](tramp::SessionView& v) { v.collection.clear(); }},
      {"playingIndex", "playlist", [](tramp::SessionView& v) { v.playingIndex = 5; }},
      {"trackScroll", "playlist", [](tramp::SessionView& v) { v.trackScroll = 3; }},
      {"collection", "playlist", [](tramp::SessionView& v) { v.collection[0].count = 99; }},
      {"collectionWidth", "playlist", [](tramp::SessionView& v) { v.collectionWidth = 300; }},
      {"collectionCollapsed", "playlist",
       [](tramp::SessionView& v) { v.collectionCollapsed = true; }},
      {"playlistName", "playlist",
       [](tramp::SessionView& v) { v.playlistName = QStringLiteral("other.m3u"); }},
      {"playlistAltered", "playlist",
       [](tramp::SessionView& v) { v.playlistAltered = !v.playlistAltered; }},
      {"playlistTotalMs", "playlist", [](tramp::SessionView& v) { v.playlistTotalMs += 61000; }},
      {"playlistTrackCount", "playlist", [](tramp::SessionView& v) { v.playlistTrackCount += 1; }},
      {"playlistRefreshEnabled", "playlist",
       [](tramp::SessionView& v) { v.playlistRefreshEnabled = !v.playlistRefreshEnabled; }},
      {"playlistRefreshing", "playlist",
       [](tramp::SessionView& v) { v.playlistRefreshing = !v.playlistRefreshing; }},

      // Settings: both tabs, because a change on the pane that is not showing
      // still has to be there when the listener switches to it.
      {"settingsTab", "settings", [](tramp::SessionView& v) { v.settingsTab = 1; }},
      {"resumeLastSession", "settings",
       [](tramp::SessionView& v) { v.resumeLastSession = !v.resumeLastSession; }},
      {"confirmBeforeQuit", "settings",
       [](tramp::SessionView& v) { v.confirmBeforeQuit = !v.confirmBeforeQuit; }},
      {"minimizeHidesSecondaries", "settings",
       [](tramp::SessionView& v) { v.minimizeHidesSecondaries = !v.minimizeHidesSecondaries; }},
      {"dockSnap", "settings", [](tramp::SessionView& v) { v.dockSnap = 2; }},
      {"skins", "skins",
       [](tramp::SessionView& v) {
         v.skins.push_back({QStringLiteral("dusk"), QStringLiteral("Dusk"), {}});
       }},
      {"activeSkinId", "skins",
       [](tramp::SessionView& v) { v.activeSkinId = QStringLiteral("dusk"); }},
      {"skinsError", "skins",
       [](tramp::SessionView& v) { v.skinsError = QStringLiteral("no skin.json"); }},
      {"skinsScroll", "skins", [](tramp::SessionView& v) { v.skinsScroll = 12; }},
      {"persistWriteFailed", "settings",
       [](tramp::SessionView& v) { v.persistWriteFailed = !v.persistWriteFailed; }},

      // About: the four figures in the stats well.
      {"aboutPlaylists", "about", [](tramp::SessionView& v) { v.aboutPlaylists += 1; }},
      {"aboutTracks", "about", [](tramp::SessionView& v) { v.aboutTracks += 1; }},
      {"aboutTimeMs", "about", [](tramp::SessionView& v) { v.aboutTimeMs += 60000; }},
      {"aboutSpins", "about", [](tramp::SessionView& v) { v.aboutSpins += 1; }},

      // Carried on the snapshot and painted by nobody.
      {"selectedIndices", "", [](tramp::SessionView& v) { v.selectedIndices = {1, 4}; }},
      {"collectionSelected", "",
       [](tramp::SessionView& v) { v.collectionSelected = QStringLiteral("/x.m3u"); }},
      {"aboutMeasured", "", [](tramp::SessionView& v) { v.aboutMeasured = !v.aboutMeasured; }},
  };
}

int rgbDistance(QRgb a, const QColor& b) {
  const QColor c = QColor::fromRgba(a);
  return std::abs(c.red() - b.red()) + std::abs(c.green() - b.green()) +
         std::abs(c.blue() - b.blue());
}

}  // namespace

// One field at a time, against the painters themselves. Every panel that keeps
// its raster for a change has to paint that change identically — otherwise the
// listener is left looking at pixels that stopped being true, which is worse
// than any number of redundant rebuilds. The other direction is checked too, so
// a group cannot quietly narrow until a change stops reaching the panel that
// shows it.
void HostWindowMoveTest::aKeptRasterIsNeverOneThatWentStale() {
  tramp::SessionView base = tramp::goldenDemoView();
  base.goldenDemo = false;
  // Past the hold, so the marquee cases below can move the clock in both
  // directions across it.
  base.titleScrollMs = 3000;

  for (const FieldChange& change : everyFieldOfTheSnapshot()) {
    tramp::SessionView changed = base;
    change.apply(changed);
    QStringList rebuilds;
    for (tramp::WindowId id : everyPanel()) {
      if (!tramp::paintsSame(id, base, changed)) {
        rebuilds << panelLabel(id);
        continue;
      }
      const QSize logical = panelLogicalSize(id);
      QVERIFY2(paintCachedPass(id, logical, base) == paintCachedPass(id, logical, changed),
               qPrintable(QStringLiteral("%1 keeps its raster for %2, which changes what it paints")
                              .arg(panelLabel(id), QLatin1String(change.what))));
    }
    QVERIFY2(rebuilds.join(QLatin1Char('+')) == QLatin1String(change.rebuilds),
             qPrintable(QStringLiteral("%1 re-rasterises [%2], expected [%3]")
                            .arg(QLatin1String(change.what), rebuilds.join(QLatin1Char('+')),
                                 QLatin1String(change.rebuilds))));
  }
}

// The lamp used to have to flush with the wait cursor, because the work it
// announced held the event loop: nothing reached the screen until that work had
// already finished. Ingest runs on a worker now, so the lamp gets there the
// ordinary way — publish the view, turn the loop — and there is no wait cursor
// on that path at all. Which face it lights is
// `refreshButtonLightsWhilePlaylistRefreshing`.
void HostWindowMoveTest::refreshLampLightsOnTheLiveEventLoop() {
  const auto specs = tramp::windowSpecs();
  HostShell shell;
  PaintCountHost pl(specs[2], &shell);
  shell.show();
  pl.show();
  QVERIFY(QTest::qWaitForWindowExposed(&pl));

  tramp::SessionView idle;
  idle.playlistRefreshEnabled = true;
  pl.setSessionView(idle);
  QApplication::processEvents();

  tramp::SessionView busy = idle;
  busy.playlistRefreshing = true;
  const int before = pl.paints;
  pl.setSessionView(busy);
  QVERIFY2(!tramp::WaitCursorScope::showing(), "a playlist ingest must not raise a wait cursor");
  QCOMPARE(pl.paints, before);
  QTRY_VERIFY2(pl.paints > before, "the lamp must reach the screen on the next turn of the loop");
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

// A painter that overrides the golden view instead of honouring it makes the
// state it overrides unreachable, and an unreachable state is one no dump can
// photograph and no fidelity gate can watch. That is how the settings Skins
// pane went unseen, so the states the dump relies on are asserted here.
void HostWindowMoveTest::goldenDemoPaintsTheStateItIsHanded() {
  const tramp::SessionView golden = tramp::goldenDemoView();
  QVERIFY2(golden.goldenDemo, "the demo state is still the fidelity reference");

  QVERIFY2(!golden.spectrumUnmeasured, "the golden demo is a measured spectrum");
  QVERIFY2(!golden.noAudioEngine, "the golden demo has a working engine");
  QVERIFY2(!golden.persistWriteFailed, "the golden demo has writable settings");

  tramp::SessionView skins = golden;
  skins.settingsTab = 1;
  skins.skins = {{QStringLiteral("builtin"), QStringLiteral("Built-in"), {}},
                 {QStringLiteral("dusk"), QStringLiteral("Dusk"), {}}};
  QVERIFY2(paintPanel(tramp::WindowId::settings, tramp::kSettings, skins) !=
               paintPanel(tramp::WindowId::settings, tramp::kSettings, golden),
           "the golden demo must be able to open the Audio tab");
  QVERIFY2(paintPanel(tramp::WindowId::skins, tramp::kSkins, skins) !=
               paintPanel(tramp::WindowId::skins, tramp::kSkins, golden),
           "the golden demo must be able to photograph a populated Skins panel");

  // The demo list fits the default well exactly, so the clamped panel is the
  // only picture the track scrollbar appears in.
  const QRectF clampedList = tramp::playlistListRowRect(
      tramp::playlistTrackInner(tramp::playlistTracksPane(
          tramp::panelBody(tramp::kPlaylistMinWithCollection),
          tramp::kPlaylistCollectionMinWidth)));
  QVERIFY2(tramp::playlistListMaxScroll(int(golden.tracks.size()), clampedList.height()) > 0,
           "the clamped playlist must overflow, or the scrollbar loses its only picture");
}

// A helper that leaves a pen or a brush behind is invisible until a caller
// sets its state once and then draws several things through it. The playlist
// footer does exactly that, and `drawStatusDot`'s stray `Qt::NoPen` cost the
// strip its track count, its playing state and its drop hint for as long as
// the strip had existed. Every painter mockup_draw exports is checked, so the
// next one to grow a `setPen` fails here rather than in a screenshot nobody
// takes.
void HostWindowMoveTest::mockupHelpersLeaveThePainterAsTheyFoundIt() {
  QImage canvas(260, 200, QImage::Format_ARGB32_Premultiplied);
  canvas.fill(Qt::black);
  QPainter p(&canvas);
  QImage logo(8, 8, QImage::Format_ARGB32_Premultiplied);
  logo.fill(Qt::white);

  const QRectF box(30, 30, 110, 46);
  const QRectF slim(30, 30, 110, 16);
  const QRectF glyph(30, 30, 24, 24);
  const QPointF dot(60, 60);
  const QString label = QStringLiteral("Label");
  const QFont face = tramp::condensedFont(13, 0.1);

  const QVector<QPair<QString, std::function<void(QPainter&)>>> helpers = {
      {QStringLiteral("fillRound"), [&](QPainter& g) { tramp::fillRound(g, box, 4, Qt::red); }},
      {QStringLiteral("drawScreenWell"), [&](QPainter& g) { tramp::drawScreenWell(g, box); }},
      {QStringLiteral("drawScreenOverlay"), [&](QPainter& g) { tramp::drawScreenOverlay(g, box); }},
      {QStringLiteral("drawScreen"), [&](QPainter& g) { tramp::drawScreen(g, box); }},
      {QStringLiteral("drawListWell"), [&](QPainter& g) { tramp::drawListWell(g, box); }},
      {QStringLiteral("drawBtn"), [&](QPainter& g) { tramp::drawBtn(g, box, true); }},
      {QStringLiteral("drawBtn labelled"),
       [&](QPainter& g) { tramp::drawBtn(g, box, tramp::BtnFace(0.4, 0.5, 0.6), label); }},
      {QStringLiteral("drawIcon"),
       [&](QPainter& g) { tramp::drawIcon(g, glyph, tramp::MockupIcon::play, Qt::white); }},
      {QStringLiteral("drawIcon mute"),
       [&](QPainter& g) { tramp::drawIcon(g, glyph, tramp::MockupIcon::mute, Qt::white); }},
      {QStringLiteral("drawIcon options"),
       [&](QPainter& g) { tramp::drawIcon(g, glyph, tramp::MockupIcon::options, Qt::white); }},
      {QStringLiteral("drawIcon skins"),
       [&](QPainter& g) { tramp::drawIcon(g, glyph, tramp::MockupIcon::skins, Qt::white); }},
      {QStringLiteral("drawIcon trackInfo"),
       [&](QPainter& g) { tramp::drawIcon(g, glyph, tramp::MockupIcon::trackInfo, Qt::white); }},
      {QStringLiteral("drawGlyphBtn"),
       [&](QPainter& g) { tramp::drawGlyphBtn(g, box, tramp::MockupIcon::next, true); }},
      {QStringLiteral("drawSlider"), [&](QPainter& g) { tramp::drawSlider(g, slim, 0.4); }},
      {QStringLiteral("drawSlider seek"),
       [&](QPainter& g) { tramp::drawSlider(g, slim, 0.4, true); }},
      {QStringLiteral("drawVBand"), [&](QPainter& g) { tramp::drawVBand(g, box, 4.5); }},
      {QStringLiteral("drawLed"), [&](QPainter& g) { tramp::drawLed(g, dot, 0.7); }},
      {QStringLiteral("drawLed dark"), [&](QPainter& g) { tramp::drawLed(g, dot, 0); }},
      {QStringLiteral("drawToggleBtn"),
       [&](QPainter& g) { tramp::drawToggleBtn(g, box, label, true); }},
      {QStringLiteral("drawMenuCaret"), [&](QPainter& g) { tramp::drawMenuCaret(g, box); }},
      {QStringLiteral("drawReload"), [&](QPainter& g) { tramp::drawReload(g, glyph, Qt::white); }},
      {QStringLiteral("drawChevron"),
       [&](QPainter& g) { tramp::drawChevron(g, glyph, true, Qt::white); }},
      {QStringLiteral("drawCreateMark"),
       [&](QPainter& g) { tramp::drawCreateMark(g, glyph, Qt::white); }},
      {QStringLiteral("drawRenameMark"),
       [&](QPainter& g) { tramp::drawRenameMark(g, glyph, Qt::white); }},
      {QStringLiteral("drawFooterSep"), [&](QPainter& g) { tramp::drawFooterSep(g, slim); }},
      {QStringLiteral("drawStatusDot"), [&](QPainter& g) { tramp::drawStatusDot(g, dot); }},
      {QStringLiteral("drawScrollbar"),
       [&](QPainter& g) { tramp::drawScrollbar(g, QRectF(200, 30, 14, 120), 10, 40); }},
      {QStringLiteral("drawDiscLogo"),
       [&](QPainter& g) { tramp::drawDiscLogo(g, QRectF(30, 30, 30, 30), &logo); }},
      {QStringLiteral("drawDiscLogo flat"),
       [&](QPainter& g) { tramp::drawDiscLogo(g, QRectF(30, 30, 58, 58), &logo, false); }},
      {QStringLiteral("drawNoiseOverlay"),
       [&](QPainter& g) { tramp::drawNoiseOverlay(g, box, 6); }},
      {QStringLiteral("drawStyledText"),
       [&](QPainter& g) {
         tramp::drawStyledText(g, box, label, face, Qt::white, Qt::AlignCenter,
                               {{Qt::black, QPointF(0, 1), 0}});
       }},
      {QStringLiteral("drawGlowText"),
       [&](QPainter& g) {
         tramp::drawGlowText(g, box, label, face, Qt::white, Qt::cyan, 5, Qt::AlignCenter);
       }},
      {QStringLiteral("paintBlurred"),
       [&](QPainter& g) {
         tramp::paintBlurred(g, box, 0, [&](QPainter& bp) { bp.setPen(Qt::NoPen); });
       }},
  };

  const QPen pen(QColor(11, 22, 33), 3);
  const QBrush brush(QColor(44, 55, 66));
  const QFont font = tramp::monoFont(17, 0.25);
  for (const auto& helper : helpers) {
    p.setPen(pen);
    p.setBrush(brush);
    p.setFont(font);
    helper.second(p);
    QVERIFY2(p.pen() == pen, qPrintable(helper.first + QStringLiteral(" left a pen behind")));
    QVERIFY2(p.brush() == brush, qPrintable(helper.first + QStringLiteral(" left a brush behind")));
    QVERIFY2(p.font().toString() == font.toString(),
             qPrintable(helper.first + QStringLiteral(" left a font behind")));
  }
}

namespace {

/// A panel and the state it is painted in. One picture of each panel is not
/// enough: a leak that only happens once a list has rows, or once the Skins tab
/// is open, is still a leak, and these are the states the fidelity dump
/// photographs for the same reason.
struct PanelState {
  QString what;
  tramp::WindowId id;
  QSize logical;
  tramp::SessionView view;
};

QVector<PanelState> panelStates() {
  const auto specs = tramp::windowSpecs();
  const tramp::SessionView golden = tramp::goldenDemoView();
  tramp::SessionView collapsed = golden;
  collapsed.collectionCollapsed = true;
  tramp::SessionView skins = golden;
  skins.settingsTab = 1;
  skins.skins = {{QStringLiteral("builtin"), QStringLiteral("Built-in"), {}},
                 {QStringLiteral("dusk"), QStringLiteral("Dusk"),
                  QStringLiteral("Halogen Youth")}};
  skins.activeSkinId = QStringLiteral("dusk");
  skins.skinsError = QStringLiteral("dusk.zip: no skin.json at the archive root.");
  return {
      {QStringLiteral("the main player"), tramp::WindowId::main, specs[0].logicalSize, golden},
      {QStringLiteral("an empty main player"), tramp::WindowId::main, specs[0].logicalSize, {}},
      {QStringLiteral("the equaliser"), tramp::WindowId::equalizer, specs[1].logicalSize, golden},
      {QStringLiteral("the playlist"), tramp::WindowId::playlist, specs[2].logicalSize, golden},
      {QStringLiteral("the collapsed playlist"), tramp::WindowId::playlist, specs[2].logicalSize,
       collapsed},
      {QStringLiteral("an empty playlist"), tramp::WindowId::playlist, specs[2].logicalSize, {}},
      {QStringLiteral("the settings pane"), tramp::WindowId::settings, specs[3].logicalSize,
       golden},
      {QStringLiteral("the Audio tab"), tramp::WindowId::settings, specs[3].logicalSize, skins},
      {QStringLiteral("the about panel"), tramp::WindowId::about, specs[4].logicalSize, golden},
      {QStringLiteral("the Skins panel"), tramp::WindowId::skins, specs[5].logicalSize, skins},
  };
}

const char* passName(tramp::BodyPaint pass) {
  switch (pass) {
    case tramp::BodyPaint::full:
      return "the full pass";
    case tramp::BodyPaint::chassis:
      return "the chassis pass";
    case tramp::BodyPaint::live:
      return "the live pass";
  }
  return "an unknown pass";
}

}  // namespace

// Same contract as `mockupHelpersLeaveThePainterAsTheyFoundIt`, one layer up.
// Every panel painter used to leave its last pen and font behind, and the About
// plate left `QPainter::SmoothPixmapTransform` on top of that. Nothing showed,
// because each readout sets what it draws with first — which was equally true
// of the playlist footer right up until a status dot went in between two
// readouts and the ones after it lost their pen.
//
// The reading is the whole of `PainterState` rather than the pen, brush and
// font the helper pin above compares: the plate's render hint went straight
// through a check that narrow, and a check that cannot see the last leak is not
// going to see the next one.
//
// What this covers is the entry points, and only the entry points. From out
// here the six panel painters are unreachable: `paintWindowBody` holds the
// painter's state across the whole switch, so a painter that drops its own
// `PainterStateScope` still hands the caller back what it was given and this
// stays green — verified by taking one back out. So the net is the reason a
// painter losing its scope would cost nothing, and the reason nothing would say
// so: the same silence the three rounds of leaks sat in. Asserting one painter
// on its own needs a translation unit that `#include`s `chrome_bodies.cpp` to
// reach the file-local painters, which cannot be this binary — it already links
// the real one, and a second `paintWindowBody` does not link. So read a pass
// here as: the doors the chrome is drawn through are neutral, whatever happens
// behind them.
void HostWindowMoveTest::panelPaintersLeaveThePainterAsTheyFoundIt() {
  const QImage logo = tramp::loadTrampLogo();
  const QPen pen(QColor(11, 22, 33), 3);
  const QBrush brush(QColor(44, 55, 66));
  const QFont font = tramp::monoFont(17, 0.25);

  for (const PanelState& panel : panelStates()) {
    for (tramp::BodyPaint pass :
         {tramp::BodyPaint::full, tramp::BodyPaint::chassis, tramp::BodyPaint::live}) {
      QImage canvas(panel.logical, QImage::Format_ARGB32_Premultiplied);
      canvas.fill(Qt::transparent);
      QPainter p(&canvas);
      p.setRenderHint(QPainter::Antialiasing);
      p.setRenderHint(QPainter::TextAntialiasing);
      // State nothing in the chrome paints with, so a painter that happens to
      // leave behind what was already there is still caught.
      p.setPen(pen);
      p.setBrush(brush);
      p.setFont(font);

      const tramp::PainterState found = tramp::PainterState::of(p);
      tramp::paintWindowBody(p, panel.id, panel.logical, &logo, panel.view, pass);
      const QStringList moved = tramp::PainterState::of(p).differencesFrom(found);
      QVERIFY2(moved.isEmpty(),
               qPrintable(QStringLiteral("%1 left %2 behind on %3")
                              .arg(panel.what, moved.join(QStringLiteral(", ")),
                                   QLatin1String(passName(pass)))));

      // The whole window through the front door, which is the title bar layer
      // as well as the body, and the shell plate that is drawn before the
      // module takes its own clip. Its helpers were the round before this one
      // and have never had a pin of their own.
      const tramp::TitleChromeLayout title =
          tramp::TitleChromeLayout::forWindow(panel.id, panel.logical);
      const tramp::PainterState atDoor = tramp::PainterState::of(p);
      tramp::paintMockupWindow(p, panel.logical, panel.id, title, &logo, panel.view, pass);
      const QStringList escaped = tramp::PainterState::of(p).differencesFrom(atDoor);
      QVERIFY2(escaped.isEmpty(),
               qPrintable(QStringLiteral("painting %1 whole left %2 behind on %3")
                              .arg(panel.what, escaped.join(QStringLiteral(", ")),
                                   QLatin1String(passName(pass)))));
    }
  }
}

// The other half of the same contract, and the only part of the inside of a
// painter this can reach. `paintWindowBody` holds the painter's state across
// the whole call, so a readout that quietly draws with what the readout before
// it left is invisible from out here. What is visible is a readout that draws
// with state it never set at all: hand the panel a pen, a brush and a font
// nothing paints with, and if any of it reaches the canvas the picture changes.
//
// This one is green on the pre-fix painters too. It guards the opposite
// failure, so passing it is not evidence that the scopes went in — the slot
// above is where that is asserted. It is here so a readout cannot start
// leaning on a caller's pen the day someone takes a scope back out.
//
// Render hints, opacity, clip and transform are deliberately not varied here.
// A caller sets those to decide how the panel is drawn — the dump asks for
// antialiasing, the host composites at a device ratio — so a difference in the
// picture would be the painters honouring the request, not reading state they
// should have set.
void HostWindowMoveTest::panelPaintersDrawWithWhatTheySet() {
  const QImage logo = tramp::loadTrampLogo();

  auto shoot = [&](const PanelState& panel, tramp::BodyPaint pass, bool hostile) {
    QImage canvas(panel.logical, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    if (hostile) {
      p.setPen(QPen(QColor(255, 0, 255), 7));
      p.setBrush(QBrush(QColor(0, 255, 0)));
      p.setFont(tramp::monoFont(31, 0.4));
    }
    tramp::paintWindowBody(p, panel.id, panel.logical, &logo, panel.view, pass);
    p.end();
    return canvas;
  };

  for (const PanelState& panel : panelStates()) {
    for (tramp::BodyPaint pass :
         {tramp::BodyPaint::full, tramp::BodyPaint::chassis, tramp::BodyPaint::live}) {
      QVERIFY2(shoot(panel, pass, false) == shoot(panel, pass, true),
               qPrintable(QStringLiteral("%1 paints something with the pen, brush or font it "
                                         "was handed on %2")
                              .arg(panel.what, QLatin1String(passName(pass)))));
    }
  }
}

void HostWindowMoveTest::emptyStateCopyIsTheLockedCopy() {
  const tramp::EmptyWellCopy playlist = tramp::playlistEmptyCopy();
  QCOMPARE(playlist.heading, QStringLiteral("THIS LIST IS EMPTY"));
  QCOMPARE(playlist.body, QStringLiteral("Drop files here, or open one from PLAYLISTS."));
  QVERIFY(!playlist.body.contains(QStringLiteral("DROP FILES HERE TO ENQUEUE")));

  const tramp::EmptyWellCopy collection = tramp::collectionEmptyCopy();
  QCOMPARE(collection.heading, QStringLiteral("NO SAVED PLAYLISTS"));
  QCOMPARE(collection.body,
           QStringLiteral("A playlist is a file you keep. Tramp does not scan a library."));

  QCOMPARE(tramp::resumePlaybackLabel(), QStringLiteral("Resume playback"));

  tramp::SessionView empty;
  QCOMPARE(empty.title, QStringLiteral("No track"));
  QVERIFY(empty.tracks.isEmpty());
  QCOMPARE(tramp::mainEmptyTitle(empty), QStringLiteral("Drop files to play"));

  tramp::SessionView stopped = empty;
  stopped.tracks.push_back(
      {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("1:00")});
  QCOMPARE(tramp::mainEmptyTitle(stopped), QStringLiteral("No track"));

  const tramp::SessionView golden = tramp::goldenDemoView();
  QVERIFY(!golden.tracks.isEmpty());
  QCOMPARE(golden.aboutSpins, 4096);
  QCOMPARE(tramp::mainEmptyTitle(golden), golden.title);

  tramp::SessionView named;
  named.title = QStringLiteral("Velvet Static");
  QCOMPARE(tramp::mainEmptyTitle(named), QStringLiteral("Velvet Static"));

  const auto display = tramp::nowPlayingDisplay(std::nullopt, std::nullopt, 0);
  QCOMPARE(display.title, QStringLiteral("No track"));
}

void HostWindowMoveTest::paintsSameFlipsWhenAnEmptyListGainsARow() {
  tramp::SessionView empty;
  tramp::SessionView oneTrack = empty;
  oneTrack.tracks.push_back(
      {QStringLiteral("Artist"), QStringLiteral("Track"), QStringLiteral("3:20")});

  QVERIFY2(!tramp::paintsSame(tramp::WindowId::playlist, empty, oneTrack),
           "the playlist chassis must turn over when the track list leaves empty");
  QVERIFY2(!tramp::paintsSame(tramp::WindowId::main, empty, oneTrack),
           "the main chassis must turn over: Drop files to play becomes No track");

  tramp::SessionView oneList = empty;
  oneList.collection.push_back({QStringLiteral("Nights"), 12, true, false});
  QVERIFY2(!tramp::paintsSame(tramp::WindowId::playlist, empty, oneList),
           "the playlist chassis must turn over when the collection leaves empty");

  tramp::SessionView titled;
  titled.title = QStringLiteral("Velvet Static");
  tramp::SessionView titledOne = titled;
  titledOne.tracks.push_back(
      {QStringLiteral("Artist"), QStringLiteral("Track"), QStringLiteral("3:20")});
  QVERIFY2(tramp::paintsSame(tramp::WindowId::main, titled, titledOne),
           "a named title must not rebuild main just because the list gained a row");
}

void HostWindowMoveTest::paintsSameFlipsWhenSkinRadiiChange() {
  tramp::SessionView base;
  tramp::SessionView rounded = base;
  rounded.look.radii.window = 0;
  QVERIFY2(!tramp::paintsSame(tramp::WindowId::main, base, rounded),
           "a skin that sharpens window corners must rebuild every chassis");
}

namespace {

QRectF paintedTrackWell(const tramp::SessionView& view) {
  const QRectF body = tramp::panelBody(tramp::kPlaylistDefault);
  const qreal collectionW = view.collectionCollapsed ? 0 : view.collectionWidth;
  return tramp::playlistListWell(
      tramp::playlistListRowRect(
          tramp::playlistTrackInner(tramp::playlistTracksPane(body, collectionW))),
      view.tracks.size());
}

int pixelsNear(const QImage& img, const QRect& region, const QColor& target, int maxDist) {
  int n = 0;
  const QRect clipped = region.intersected(QRect(QPoint(), img.size()));
  for (int y = clipped.top(); y <= clipped.bottom(); ++y) {
    for (int x = clipped.left(); x <= clipped.right(); ++x) {
      if (rgbDistance(img.pixel(x, y), target) <= maxDist) ++n;
    }
  }
  return n;
}

}  // namespace

void HostWindowMoveTest::emptyWellsAreNotBlank() {
  tramp::loadTrampFonts();
  tramp::SessionView empty;
  tramp::SessionView withTrack = empty;
  withTrack.tracks.push_back({QStringLiteral("Cassette Mirage"),
                              QStringLiteral("Low Orbit Lullaby"), QStringLiteral("4:12")});
  tramp::SessionView withList = empty;
  withList.collection.push_back({QStringLiteral("ANALOGUE GHOSTS"), 24, false, false});

  const QImage emptyPl = paintPlaylistPanel(empty);
  const QImage trackPl = paintPlaylistPanel(withTrack);
  const QImage listPl = paintPlaylistPanel(withList);
  QVERIFY2(emptyPl != trackPl, "an empty track well must not paint like a list with a row");
  QVERIFY2(emptyPl != listPl, "an empty collection well must not paint like a list with a row");

  const tramp::ChromeTokens tokens = tramp::ChromeTokens::builtin();
  const QRect listWell = paintedTrackWell(empty).toAlignedRect();
  const int emptyInk = pixelsNear(emptyPl, listWell, tokens.inkFaint, 72);
  const int trackInk = pixelsNear(trackPl, listWell, tokens.inkFaint, 72);
  QVERIFY2(emptyInk > 80,
           qPrintable(QStringLiteral("empty track well ink-faint pixels: %1 (wash only would be ~0)")
                          .arg(emptyInk)));
  QVERIFY2(emptyInk > trackInk, "track rows are phosphor; empty-state copy is chrome ink");

  const QRect colWell =
      tramp::playlistCollectionWell(tramp::panelBody(tramp::kPlaylistDefault),
                                    empty.collectionWidth)
          .toAlignedRect();
  const int emptyColInk = pixelsNear(emptyPl, colWell, tokens.inkFaint, 72);
  const int listedColInk = pixelsNear(listPl, colWell, tokens.inkFaint, 72);
  QVERIFY2(emptyColInk > 80,
           qPrintable(QStringLiteral("empty collection well ink-faint pixels: %1")
                          .arg(emptyColInk)));
  QVERIFY2(emptyColInk > listedColInk, "a saved-playlist row is not the empty-state heading");

  const tramp::SessionView golden = tramp::goldenDemoView();
  const QImage goldenPl = paintPlaylistPanel(golden);
  QVERIFY2(goldenPl != emptyPl, "the golden playlist dump must keep its rows");
  QVERIFY2(emptyPl.copy(listWell) !=
               goldenPl.copy(paintedTrackWell(golden).toAlignedRect()),
           "empty-well copy must not appear on the golden playlist");

  const QSize mainLogical = tramp::kMainPlayer;
  const QImage emptyMain = paintCachedPass(tramp::WindowId::main, mainLogical, empty);
  const QImage stoppedMain = paintCachedPass(tramp::WindowId::main, mainLogical, withTrack);
  QVERIFY2(emptyMain != stoppedMain, "Drop files to play must leave the chassis once a row exists");

  tramp::SessionView spun = golden;
  spun.aboutSpins = 0;
  QVERIFY2(paintCachedPass(tramp::WindowId::main, mainLogical, golden) ==
               paintCachedPass(tramp::WindowId::main, mainLogical, spun),
           "the golden demo title must not follow aboutSpins");
}

void HostWindowMoveTest::unmeasuredSpectrumMarkFollowsTheSpectrogram() {
  tramp::loadTrampFonts();
  const tramp::SessionView measured = tramp::goldenDemoView();
  tramp::SessionView unmeasured = measured;
  unmeasured.spectrumUnmeasured = true;

  QVERIFY2(!tramp::paintsSame(tramp::WindowId::main, measured, unmeasured),
           "the main chassis must turn over for the unmeasured-spectrum mark");
  QVERIFY2(tramp::paintsSame(tramp::WindowId::settings, measured, unmeasured),
           "the mark is a display-well surface, not a second settings notice");

  tramp::SessionView pausedMeasured = measured;
  pausedMeasured.playing = false;
  pausedMeasured.paused = true;
  tramp::SessionView pausedUnmeasured = unmeasured;
  pausedUnmeasured.playing = false;
  pausedUnmeasured.paused = true;

  const QSize main = tramp::kMainPlayer;
  QVERIFY2(paintPanel(tramp::WindowId::main, main, unmeasured) !=
               paintPanel(tramp::WindowId::main, main, measured),
           "an unmeasured spectrum must paint a mark on the display well");
  QVERIFY2(paintPanel(tramp::WindowId::main, main, pausedUnmeasured) !=
               paintPanel(tramp::WindowId::main, main, pausedMeasured),
           "pause must not clear the unmeasured mark");
}

void HostWindowMoveTest::missingEngineMarkStaysOnTheDisplayWell() {
  tramp::loadTrampFonts();
  const tramp::SessionView working = tramp::goldenDemoView();
  tramp::SessionView missing = working;
  missing.noAudioEngine = true;

  QVERIFY2(!tramp::paintsSame(tramp::WindowId::main, working, missing),
           "the main chassis must turn over for the missing-engine mark");
  QVERIFY2(tramp::paintsSame(tramp::WindowId::settings, working, missing),
           "the mark is a display-well surface, not a second settings notice");
  QVERIFY2(paintPanel(tramp::WindowId::main, tramp::kMainPlayer, missing) !=
               paintPanel(tramp::WindowId::main, tramp::kMainPlayer, working),
           "a missing audio engine must paint a durable mark on the display well");
}

void HostWindowMoveTest::persistFailureMarkStaysUntilAWriteSucceeds() {
  tramp::loadTrampFonts();
  const tramp::SessionView ok = tramp::goldenDemoView();
  tramp::SessionView failed = ok;
  failed.persistWriteFailed = true;

  QVERIFY2(!tramp::paintsSame(tramp::WindowId::settings, ok, failed),
           "the settings chassis must turn over for a persist-write mark");
  QVERIFY2(tramp::paintsSame(tramp::WindowId::main, ok, failed),
           "the persist mark is a Settings-row surface, not a title-bar overlay");
  QVERIFY2(paintPanel(tramp::WindowId::settings, tramp::kSettings, failed) !=
               paintPanel(tramp::WindowId::settings, tramp::kSettings, ok),
           "a failed state-file write must paint a Settings-row mark");
}

void HostWindowMoveTest::skinsErrorStaysOnTheSkinsStrip() {
  tramp::loadTrampFonts();
  tramp::SessionView clean = tramp::goldenDemoView();
  clean.settingsTab = 1;
  tramp::SessionView failed = clean;
  failed.skinsError = QStringLiteral("no skin.json at the archive root");

  QVERIFY2(tramp::paintsSame(tramp::WindowId::main, clean, failed),
           "a skin install error is not a second display-well surface");
  QVERIFY2(tramp::paintsSame(tramp::WindowId::settings, clean, failed),
           "a skin install error is not a Settings surface");
  QVERIFY2(!tramp::paintsSame(tramp::WindowId::skins, clean, failed),
           "the Skins-panel strip is the transient notice");
  QVERIFY2(paintPanel(tramp::WindowId::skins, tramp::kSkins, failed) !=
               paintPanel(tramp::WindowId::skins, tramp::kSkins, clean),
           "the Skins strip must still paint skinsError");
}

void HostWindowMoveTest::eqCurveWellIgnoresPreamp() {
  tramp::SessionView a = tramp::goldenDemoView();
  tramp::SessionView b = a;
  b.eq.preamp = -8;
  tramp::SessionView shaped = a;
  shaped.eq.gains[0] = 12;

  const QImage pa = paintPanel(tramp::WindowId::equalizer, tramp::kEqualizer, a);
  const QImage pb = paintPanel(tramp::WindowId::equalizer, tramp::kEqualizer, b);
  const QImage ps = paintPanel(tramp::WindowId::equalizer, tramp::kEqualizer, shaped);

  const QRectF body = tramp::panelBody(tramp::kEqualizer);
  const tramp::EqHeaderRow header =
      tramp::layoutEqHeader(body, tramp::labelBtnWidth(QStringLiteral("ON")),
                            tramp::labelBtnWidth(QStringLiteral("AUTO")),
                            tramp::labelBtnWidth(QStringLiteral("PRESETS"), 16, 22));
  const QRect curve = header.curveWell.toRect();
  const QRect preampWell = tramp::eqBandColumn(tramp::eqBandRow(body), 0).well.toRect();

  QVERIFY2(pa.copy(curve) != ps.copy(curve),
           "the curve well must still follow the ten band gains");
  QVERIFY2(pa.copy(preampWell) != pb.copy(preampWell),
           "the preamp column must still move with preamp");
  QCOMPARE(pa.copy(curve), pb.copy(curve));
}

void HostWindowMoveTest::wordmarkKeepsBrandFaceWhenChromeFontChanges() {
  tramp::loadTrampFonts();
  tramp::SessionView view = tramp::goldenDemoView();
  const tramp::TitleChromeLayout title =
      tramp::TitleChromeLayout::forWindow(tramp::WindowId::main, tramp::kMainPlayer);
  const QImage logo = tramp::loadTrampLogo();

  auto shoot = [&]() {
    QImage img(tramp::kMainPlayer, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::black);
    QPainter p(&img);
    tramp::paintMockupWindow(p, tramp::kMainPlayer, tramp::WindowId::main, title, &logo, view);
    p.end();
    return img;
  };

  tramp::setLookFamilies({}, {});
  const QImage builtin = shoot();
  tramp::setLookFamilies(tramp::lcdFamily(), tramp::lcdFamily());
  const QImage skinned = shoot();
  tramp::setLookFamilies({}, {});

  const int wmW =
      QFontMetrics(tramp::brandFont(24)).horizontalAdvance(QStringLiteral("TRAMP"));
  const QRect wordmark(54, 0, wmW, tramp::kTitleBar);
  const QRect role(330, 0, 180, tramp::kTitleBar);
  QVERIFY2(builtin.copy(role) != skinned.copy(role),
           "a chrome-font override must still restyle the role title");
  QCOMPARE(builtin.copy(wordmark), skinned.copy(wordmark));
}

QTEST_MAIN(HostWindowMoveTest)
#include "host_window_move_test.moc"
