#include "chrome_anim.h"
#include "chrome_layout.h"
#include "mockup_tokens.h"
#include "title_chrome.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QTest>

class ChromeSpecTest : public QObject {
  Q_OBJECT

 private slots:
  void tokensMatchMockupCssRoot();
  void titleBarStopsMatchMockupStylesheet();
  void nativeSeedIsRoundedSeventyFivePercent();
  void windowSpecsUseNativeSeeds();
  void mainTitleDragExcludesWindowButtons();
  void extrasOmitBrandAndZoomAndUseCollapse();
  void zoomStepsMoveAcrossTheDiscreteLadder();
  void retiredZoomStepsSnapToTheNearestSurvivor();
  void mainTitleShowsZoomReadoutBetweenZoomButtons();
  void overflowingTitleMarqueeHoldsThenLoops();
  void chromePaintBufferMatchesWidgetTimesDpr();
  void stereoPlaylistGapHoldsForWideGlyphs();
  void skinsListScrollsLastRowIntoView();
  void playlistHidesScrollbarWhenRowsFit();
  void collapsedCollectionKeepsTheColumnItsReopenTabPaintsIn();
  void playlistStripKeepsGapBeforeLengthWell();
  void playlistStripRefreshSitsRightOfTotal();
  void buttonPhaseTakesTheWholeTransitionWhateverTheFrameRate();
  void inertPhaseStoreLeavesPaintersOnPlainSessionState();
  void pointerFeedbackSkipsSlidersAndListRows();
  void settledPhasesDoNotAccumulate();
};

void ChromeSpecTest::tokensMatchMockupCssRoot() {
  QCOMPARE(tramp::kShellHi.name(QColor::HexRgb).toLower(), QStringLiteral("#323744"));
  QCOMPARE(tramp::kShell.name(QColor::HexRgb).toLower(), QStringLiteral("#262b38"));
  QCOMPARE(tramp::kShellMid.name(QColor::HexRgb).toLower(), QStringLiteral("#1a1d26"));
  QCOMPARE(tramp::kShellLo.name(QColor::HexRgb).toLower(), QStringLiteral("#12141a"));
  QCOMPARE(tramp::kShellDeep.name(QColor::HexRgb).toLower(), QStringLiteral("#0a0b0e"));
  QCOMPARE(tramp::kInk.name(QColor::HexRgb).toLower(), QStringLiteral("#e8eaf0"));
  QCOMPARE(tramp::kPhos.name(QColor::HexRgb).toLower(), QStringLiteral("#3de7ff"));
  QCOMPARE(tramp::kAccent.name(QColor::HexRgb).toLower(), QStringLiteral("#ff3d9a"));
  QCOMPARE(tramp::kWell.name(QColor::HexRgb).toLower(), QStringLiteral("#050608"));
  QCOMPARE(tramp::kBtnOn0.name(QColor::HexRgb).toLower(), QStringLiteral("#a3f4ff"));
  QCOMPARE(tramp::kSliderFillHi.name(QColor::HexRgb).toLower(), QStringLiteral("#cbf9ff"));
}

void ChromeSpecTest::titleBarStopsMatchMockupStylesheet() {
  QCOMPARE(tramp::kTitleBar0.name(QColor::HexRgb).toLower(), QStringLiteral("#3c4356"));
  QCOMPARE(tramp::kTitleBar26.name(QColor::HexRgb).toLower(), QStringLiteral("#2c3241"));
  QCOMPARE(tramp::kTitleBar62.name(QColor::HexRgb).toLower(), QStringLiteral("#1d222c"));
  QCOMPARE(tramp::kTitleBar100.name(QColor::HexRgb).toLower(), QStringLiteral("#12151c"));
  QCOMPARE(tramp::kWordmark.name(QColor::HexRgb).toLower(), QStringLiteral("#eaf2ff"));
  QCOMPARE(tramp::kWbtnClose0.name(QColor::HexRgb).toLower(), QStringLiteral("#9c2a60"));
  QCOMPARE(tramp::kWindowName.red(), 200);
  QCOMPARE(tramp::kWindowName.green(), 214);
  QCOMPARE(tramp::kWindowName.blue(), 235);
  QCOMPARE(tramp::kWindowName.alpha(), 140);
}

void ChromeSpecTest::nativeSeedIsRoundedSeventyFivePercent() {
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kMainPlayer), QSize(619, 261));
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kEqualizer), QSize(619, 261));
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kPlaylistDefault), QSize(805, 522));
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kSettings), QSize(390, 315));
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kAbout), QSize(360, 270));
}

void ChromeSpecTest::windowSpecsUseNativeSeeds() {
  const auto specs = tramp::windowSpecs();
  QCOMPARE(specs[0].logicalSize, tramp::kMainPlayer);
  QCOMPARE(specs[1].logicalSize, tramp::kEqualizer);
  QCOMPARE(specs[2].logicalSize, tramp::kPlaylistDefault);
  QCOMPARE(specs[3].logicalSize, tramp::kSettings);
  QCOMPARE(specs[4].logicalSize, tramp::kAbout);
  for (const tramp::WindowSpec& spec : specs) {
    QCOMPARE(spec.size, tramp::nativeUnmappedSeed(spec.logicalSize));
  }
}

void ChromeSpecTest::mainTitleDragExcludesWindowButtons() {
  const auto layout =
      tramp::TitleChromeLayout::forWindow(tramp::WindowId::main, tramp::kMainPlayer);
  QVERIFY(layout.showBrand);
  QVERIFY(layout.showZoom);
  QCOMPARE(layout.roleName, QStringLiteral("Main Player"));

  // `.wbtn` 26×22, gap 5, pad-right 9 — zoom cluster, then a 12px gap, then
  // minimize flush against close.
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(759, 10, 26, 22));

  const QPoint closeCenter = layout.close.center();
  QCOMPARE(layout.hit(closeCenter), tramp::TitleChromeLayout::Hit::close);
  QVERIFY(!layout.inDragRegion(closeCenter));

  const QPoint inGrips(120, 21);
  QCOMPARE(layout.hit(inGrips), tramp::TitleChromeLayout::Hit::drag);
  QVERIFY(layout.inDragRegion(inGrips));
}

void ChromeSpecTest::extrasOmitBrandAndZoomAndUseCollapse() {
  const auto eq = tramp::TitleChromeLayout::forWindow(
      tramp::WindowId::equalizer, tramp::kEqualizer);
  QVERIFY(!eq.showBrand);
  QVERIFY(!eq.showZoom);
  QCOMPARE(eq.roleName, QStringLiteral("Equalizer"));
  QCOMPARE(eq.hit(eq.minimize.center()), tramp::TitleChromeLayout::Hit::collapse);
  QCOMPARE(eq.hit(eq.close.center()), tramp::TitleChromeLayout::Hit::close);
  QVERIFY(eq.zoomOut.isEmpty());
  QVERIFY(eq.zoomIn.isEmpty());

  const auto pl = tramp::TitleChromeLayout::forWindow(
      tramp::WindowId::playlist, tramp::kPlaylistDefault);
  QCOMPARE(pl.roleName, QStringLiteral("Playlist Manager"));
  QVERIFY(!pl.showBrand);
}

void ChromeSpecTest::zoomStepsMoveAcrossTheDiscreteLadder() {
  // Four steps — 75, 100, 125, 150 — with 75% the default.
  QCOMPARE(tramp::kDefaultZoomPercent, 75);
  QCOMPARE(tramp::nextZoomPercent(75), 100);
  QCOMPARE(tramp::nextZoomPercent(100), 125);
  QCOMPARE(tramp::nextZoomPercent(125), 150);
  QCOMPARE(tramp::prevZoomPercent(150), 125);
  QCOMPARE(tramp::prevZoomPercent(125), 100);
  QCOMPARE(tramp::prevZoomPercent(100), 75);

  // Both ends clamp. Stepping off either one must hold, not wrap round to the
  // far end and not run past the ladder onto a percent nothing else knows.
  QCOMPARE(tramp::nextZoomPercent(150), 150);
  QCOMPARE(tramp::prevZoomPercent(75), 75);

  QCOMPARE(tramp::zoomed(tramp::kMainPlayer, 75), QSize(619, 261));
}

void ChromeSpecTest::retiredZoomStepsSnapToTheNearestSurvivor() {
  // A listener who ran the eight-step build has 50% or 200% saved.
  QCOMPARE(tramp::snapZoomPercent(50), 75);
  QCOMPARE(tramp::snapZoomPercent(200), 150);
  QCOMPARE(tramp::snapZoomPercent(250), 150);
  QCOMPARE(tramp::snapZoomPercent(300), 150);

  // Surviving steps pass through untouched; nonsense still lands on a step.
  for (int step : tramp::kZoomSteps) {
    QCOMPARE(tramp::snapZoomPercent(step), step);
  }
  QCOMPARE(tramp::snapZoomPercent(0), 75);
  QCOMPARE(tramp::snapZoomPercent(-40), 75);

  // Nearest wins on either side of the gap between two steps.
  QCOMPARE(tramp::snapZoomPercent(87), 75);
  QCOMPARE(tramp::snapZoomPercent(88), 100);
}

void ChromeSpecTest::mainTitleShowsZoomReadoutBetweenZoomButtons() {
  const auto layout =
      tramp::TitleChromeLayout::forWindow(tramp::WindowId::main, tramp::kMainPlayer);
  QCOMPARE(layout.zoomOut, QRect(641, 10, 26, 22));
  QCOMPARE(layout.zoomReadout, QRect(672, 10, 44, 22));
  QCOMPARE(layout.zoomIn, QRect(721, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(759, 10, 26, 22));
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QVERIFY(layout.zoomOut.right() < layout.zoomReadout.left());
  QVERIFY(layout.zoomReadout.right() < layout.zoomIn.left());
  QCOMPARE(layout.minimize.left(), layout.zoomIn.x() + layout.zoomIn.width() + 12);
  QCOMPARE(layout.close.left(), layout.minimize.x() + layout.minimize.width() + 5);
  QCOMPARE(layout.hit(layout.zoomReadout.center()), tramp::TitleChromeLayout::Hit::none);
  QVERIFY(!layout.inDragRegion(layout.zoomReadout.center()));

  const auto eq = tramp::TitleChromeLayout::forWindow(
      tramp::WindowId::equalizer, tramp::kEqualizer);
  QVERIFY(eq.zoomReadout.isEmpty());
}

void ChromeSpecTest::overflowingTitleMarqueeHoldsThenLoops() {
  QCOMPARE(tramp::marqueeOffset(80, 100, 5000, true), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 5000, false), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 0, true), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 1199, true), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 1200, true), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 2200, true), 40.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 7200, true), 0.0);
  QCOMPARE(tramp::marqueeOffset(200, 100, 8200, true), 40.0);
  QVERIFY(!tramp::displayTitleOnLivePass(tramp::marqueeOffset(200, 100, 0, true)));
  QVERIFY(!tramp::displayTitleOnLivePass(tramp::marqueeOffset(80, 100, 5000, true)));
  QVERIFY(tramp::displayTitleOnLivePass(tramp::marqueeOffset(200, 100, 2200, true)));
  QCOMPARE(tramp::kDisplayTitleClipW, 385.0);
}

void ChromeSpecTest::chromePaintBufferMatchesWidgetTimesDpr() {
  // 75% of 825×348 is 619×261. A 2× display must rasterize at 1238×522, not
  // upscale a 1× chassis. 1.25× DPR rounds 619×1.25 = 773.75 → 774.
  QCOMPARE(tramp::chromePaintBufferSize(QSize(619, 261), 2.0), QSize(1238, 522));
  QCOMPARE(tramp::chromePaintBufferSize(QSize(619, 261), 1.0), QSize(619, 261));
  QCOMPARE(tramp::chromePaintBufferSize(QSize(619, 261), 1.25), QSize(774, 326));
}

void ChromeSpecTest::stereoPlaylistGapHoldsForWideGlyphs() {
  const QRectF inner(0, 0, 400, 18);
  const auto compact = tramp::layoutDisplayMetaRow(inner, 0, 40, 40, 50, 70, 36);
  QVERIFY(compact.playlist.left() - compact.channels.right() >= tramp::kDisplayMetaGap);
  QVERIFY(!compact.channels.intersects(compact.playlist));

  const auto wide = tramp::layoutDisplayMetaRow(inner, 0, 80, 80, 160, 120, 48);
  QCOMPARE(wide.playlist.left() - wide.channels.right(), tramp::kDisplayMetaGap);
  QVERIFY(!wide.channels.intersects(wide.playlist));
}

void ChromeSpecTest::skinsListScrollsLastRowIntoView() {
  const auto pane = tramp::settingsPane(tramp::kSettings);
  const auto viewport = tramp::skinsListViewport(pane);
  QCOMPARE(int(viewport.height()), 262);
  QCOMPARE(tramp::skinsListMaxScroll(8, viewport.height()), 26);

  const auto lastHidden = tramp::skinsListRow(viewport, 7, 0);
  QVERIFY(lastHidden.bottom() > viewport.bottom());

  const int scroll = tramp::skinsListMaxScroll(8, viewport.height());
  const auto lastShown = tramp::skinsListRow(viewport, 7, scroll);
  QVERIFY(lastShown.top() >= viewport.top());
  QVERIFY(lastShown.bottom() <= viewport.bottom());
}

void ChromeSpecTest::playlistHidesScrollbarWhenRowsFit() {
  const qreal wellH = tramp::playlistListWellHeight(tramp::kPlaylistDefault.height());
  QCOMPARE(tramp::playlistListMaxScroll(1, wellH), 0);
  QCOMPARE(tramp::playlistListMaxScroll(tramp::playlistVisibleRows(wellH), wellH), 0);
  QVERIFY(tramp::playlistListMaxScroll(tramp::playlistVisibleRows(wellH) + 1, wellH) > 0);

  const QRectF listRow(0, 0, 400, wellH);
  const QRectF fitting = tramp::playlistListWell(listRow, 3);
  QCOMPARE(fitting.width(), listRow.width());
  const QRectF overflowing = tramp::playlistListWell(listRow, tramp::playlistVisibleRows(wellH) + 8);
  QCOMPARE(overflowing.width(),
           listRow.width() - tramp::kPlaylistScrollGap - tramp::kPlaylistScrollW);
}

// Collapsed, the tab is the whole of the collection: it is the only thing
// painted down that edge and the only thing that reopens the pane. Track rows
// reaching into its column are rows the reopen region takes the left edge off,
// with nothing on screen to say why the click went elsewhere.
void ChromeSpecTest::collapsedCollectionKeepsTheColumnItsReopenTabPaintsIn() {
  const QRectF body = tramp::panelBody(tramp::kPlaylistDefault);
  const QRectF tab = tramp::playlistReopenTab(body);
  const QRectF collapsed = tramp::playlistTracksPane(body, 0);
  QVERIFY(!tab.isEmpty());
  QVERIFY(!tab.intersects(collapsed));
  QCOMPARE(collapsed.right(), body.right());

  // Expanded, the pane keeps the divider it always had and the tab is not painted.
  const QRectF expanded = tramp::playlistTracksPane(body, 240);
  QCOMPARE(expanded.left(), body.left() + 240 + tramp::kPlaylistDividerW);
  QVERIFY(expanded.width() < collapsed.width());
}

void ChromeSpecTest::playlistStripKeepsGapBeforeLengthWell() {
  // player-mockup-2.html `.pl-strip { gap: 8px }` — Next must not sit flush on TOTAL.
  QCOMPARE(tramp::kPlaylistStripGap, 8.0);

  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = tramp::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.total.left() - strip.next.right(), tramp::kPlaylistStripGap);
  QVERIFY(!strip.next.intersects(strip.total));
  QCOMPARE(strip.play.left() - strip.prev.right(), tramp::kPlaylistStripGap);
  QCOMPARE(strip.next.left() - strip.play.right(), tramp::kPlaylistStripGap);
}

void ChromeSpecTest::playlistStripRefreshSitsRightOfTotal() {
  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = tramp::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.refresh.left() - strip.total.right(), tramp::kPlaylistStripGap);
  QCOMPARE(strip.refresh.right(), deckInner.right());
  QCOMPARE(strip.total.left() - strip.next.right(), tramp::kPlaylistStripGap);
  QVERIFY(!strip.total.intersects(strip.refresh));
}

void ChromeSpecTest::buttonPhaseTakesTheWholeTransitionWhateverTheFrameRate() {
  using tramp::BtnChannel;
  using K = tramp::ChromeHit::Kind;

  // A panel that can only manage a few frames must still finish on time, so the
  // step is wall-clock and not per-frame.
  tramp::ChromePhases coarse;
  coarse.setLive(true);
  coarse.setTarget(K::shuffle, -1, BtnChannel::on, 1);
  QVERIFY(coarse.moving());
  QVERIFY(coarse.advance(tramp::kBtnTransitionMs / 2));
  QVERIFY(!coarse.advance(tramp::kBtnTransitionMs / 2));
  QVERIFY(!coarse.moving());
  QCOMPARE(coarse.face(K::shuffle).on, 1.0);

  tramp::ChromePhases fine;
  fine.setLive(true);
  fine.setTarget(K::shuffle, -1, BtnChannel::on, 1);
  int frames = 0;
  while (fine.advance(16) && frames < 1000) ++frames;
  QCOMPARE(fine.face(K::shuffle).on, 1.0);
  QVERIFY(frames >= int(tramp::kBtnTransitionMs / 16) - 1);

  // Mid-transition the face is neither of its two states, which is the point.
  tramp::ChromePhases part;
  part.setLive(true);
  part.setTarget(K::mute, -1, BtnChannel::on, 1);
  part.advance(tramp::kBtnTransitionMs / 2);
  const qreal half = part.face(K::mute).on;
  QVERIFY(half > 0.0);
  QVERIFY(half < 1.0);
}

void ChromeSpecTest::inertPhaseStoreLeavesPaintersOnPlainSessionState() {
  // Golden dumps and tests paint without a panel behind them; painters key off
  // live() to fall back to the session's booleans, so lit buttons stay lit.
  tramp::ChromePhases inert;
  QVERIFY(!inert.live());
  tramp::ChromePhases driven;
  driven.setLive(true);
  QVERIFY(driven.live());
}

void ChromeSpecTest::pointerFeedbackSkipsSlidersAndListRows() {
  using K = tramp::ChromeHit::Kind;
  QVERIFY(tramp::takesPointerFeedback(K::play));
  QVERIFY(tramp::takesPointerFeedback(K::plSort));
  QVERIFY(tramp::takesPointerFeedback(K::eqPresets));
  // Hovering these would rebuild a whole panel chassis per mouse move.
  QVERIFY(!tramp::takesPointerFeedback(K::plTrackRow));
  QVERIFY(!tramp::takesPointerFeedback(K::plCollectionRow));
  QVERIFY(!tramp::takesPointerFeedback(K::settingsSkinRow));
  QVERIFY(!tramp::takesPointerFeedback(K::volume));
  QVERIFY(!tramp::takesPointerFeedback(K::seek));
  QVERIFY(!tramp::takesPointerFeedback(K::eqBand));
  QVERIFY(!tramp::takesPointerFeedback(K::plResize));
  QVERIFY(!tramp::takesPointerFeedback(K::none));
}

void ChromeSpecTest::settledPhasesDoNotAccumulate() {
  using tramp::BtnChannel;
  using K = tramp::ChromeHit::Kind;
  tramp::ChromePhases phases;
  phases.setLive(true);
  // Asking an untouched control for zero must not record anything: every view
  // the session publishes aims every latched button, most of them at zero.
  for (int i = 0; i < 100; ++i) phases.setTarget(K::stop, -1, BtnChannel::on, 0);
  QVERIFY(!phases.moving());

  // A pointer crossing a row of buttons leaves one cooling entry each; they must
  // be reclaimed once they reach the floor.
  phases.setTarget(K::prev, -1, BtnChannel::hover, 1);
  phases.advance(tramp::kBtnTransitionMs);
  phases.releaseChannel(BtnChannel::hover);
  phases.advance(tramp::kBtnTransitionMs);
  QVERIFY(!phases.moving());
  QCOMPARE(phases.face(K::prev).hover, 0.0);
}

QTEST_APPLESS_MAIN(ChromeSpecTest)
#include "chrome_spec_test.moc"
