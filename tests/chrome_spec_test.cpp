#include "chrome_anim.h"
#include "chrome_layout.h"
#include "chrome_paint.h"
#include "mockup_draw.h"
#include "mockup_tokens.h"
#include "title_chrome.h"
#include "window_spec.h"

#include <QTest>

class ChromeSpecTest : public QObject {
  Q_OBJECT

 private slots:
  void tokensMatchMockupCssRoot();
  void titleBarStopsMatchMockupStylesheet();
  void nativeSeedIsRoundedSeventyFivePercent();
  void windowSpecsUseNativeSeeds();
  void gutterStacksSkinsAndTrackInfoUnderTheCog();
  void mainTitleDragExcludesWindowButtons();
  void extrasOmitBrandAndZoomAndUseCollapse();
  void zoomStepsMoveAcrossTheDiscreteLadder();
  void retiredZoomStepsSnapToTheNearestSurvivor();
  void mainTitleShowsZoomReadoutBetweenZoomButtons();
  void overflowingTitleMarqueeHoldsThenLoops();
  void chromePaintBufferMatchesWidgetTimesDpr();
  void stereoPlaylistGapHoldsForWideGlyphs();
  void skinsListScrollsLastRowIntoView();
  void skinsErrorStripClearsTheListAndTheScrollbar();
  void skinsFooterButtonsSitOnThePane();
  void playlistHidesScrollbarWhenRowsFit();
  void collapsedCollectionKeepsTheColumnItsReopenTabPaintsIn();
  void playlistStripKeepsGapBeforeLengthWell();
  void playlistStripRefreshSitsRightOfTotal();
  void playlistStripSaveSitsLeftOfAdd();
  void playlistStripShrinksToThePlayerGutterOnceLargeButtonsWouldOverlap();
  void playlistMinWidthIsWhereTheCompactStripWouldOverlap();
  void longPlaylistNameGivesWayBeforeTheFooterStripDoes();
  void buttonPhaseTakesTheWholeTransitionWhateverTheFrameRate();
  void inertPhaseStoreLeavesPaintersOnPlainSessionState();
  void pointerFeedbackSkipsSlidersAndListRows();
  void aWithdrawnZoomStepLeavesAFaceThePointerCannotLight();
  void settledPhasesDoNotAccumulate();
  void thePainterReadingNamesEveryKindOfLeak();
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
  QCOMPARE(tramp::nativeUnmappedSeed(tramp::kSkins), QSize(450, 360));
}

void ChromeSpecTest::windowSpecsUseNativeSeeds() {
  const auto specs = tramp::windowSpecs();
  QCOMPARE(specs[0].logicalSize, tramp::kMainPlayer);
  QCOMPARE(specs[1].logicalSize, tramp::kEqualizer);
  QCOMPARE(specs[2].logicalSize, tramp::kPlaylistDefault);
  QCOMPARE(specs[3].logicalSize, tramp::kSettings);
  QCOMPARE(specs[4].logicalSize, tramp::kAbout);
  QCOMPARE(specs[5].logicalSize, tramp::kSkins);
  for (const tramp::WindowSpec& spec : specs) {
    QCOMPARE(spec.size, tramp::nativeUnmappedSeed(spec.logicalSize));
  }
}

void ChromeSpecTest::gutterStacksSkinsAndTrackInfoUnderTheCog() {
  const tramp::MainDisplayRow row = tramp::layoutMainDisplay(tramp::panelBody(tramp::kMainPlayer));
  QCOMPARE(row.options.size(), QSizeF(26, 26));
  QCOMPARE(row.skins.size(), QSizeF(26, 26));
  QCOMPARE(row.trackInfo.size(), QSizeF(26, 26));
  QCOMPARE(row.skins.left(), row.options.left());
  QCOMPARE(row.trackInfo.left(), row.options.left());
  QCOMPARE(row.skins.top(), row.options.bottom() + tramp::kMainGutterBtnGap);
  QCOMPARE(row.trackInfo.top(), row.skins.bottom() + tramp::kMainGutterBtnGap);
  QVERIFY(row.trackInfo.bottom() < row.well.bottom());
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

  const auto skins = tramp::TitleChromeLayout::forWindow(tramp::WindowId::skins, tramp::kSkins);
  QCOMPARE(skins.roleName, QStringLiteral("Skins"));
  QVERIFY(!skins.showBrand);
  QVERIFY(!skins.showZoom);
  QCOMPARE(skins.hit(skins.minimize.center()), tramp::TitleChromeLayout::Hit::collapse);
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
  const auto pane = tramp::skinsPane(tramp::kSkins);
  const auto viewport = tramp::skinsListViewport(pane);
  QCOMPARE(int(viewport.height()), 372);
  QVERIFY(tramp::skinsListMaxScroll(8, viewport) > 0);

  const auto lastHidden = tramp::skinsGridCell(viewport, 7, 0);
  QVERIFY(lastHidden.bottom() > viewport.bottom());

  const int scroll = tramp::skinsListMaxScroll(8, viewport);
  const auto lastShown = tramp::skinsGridCell(viewport, 7, scroll);
  QVERIFY(lastShown.top() >= viewport.top());
  QVERIFY(lastShown.bottom() <= viewport.bottom());
}

// The install error used to be placed off the pane's bottom edge rather than
// off the list, so it landed on the last rows of any catalogue long enough to
// scroll and ran past the viewport into the scrollbar track. It now has a strip
// of its own, and nothing the pane paints may be under it.
void ChromeSpecTest::skinsErrorStripClearsTheListAndTheScrollbar() {
  const auto pane = tramp::skinsPane(tramp::kSkins);
  const auto viewport = tramp::skinsListViewport(pane);
  const auto strip = tramp::skinsErrorStrip(pane);
  const auto track = tramp::skinsListScrollTrack(viewport);
  const auto add = tramp::skinsAddBtn(pane);
  const auto folder = tramp::skinsFolderBtn(pane);
  const auto refresh = tramp::skinsRefreshBtn(pane);

  QVERIFY(!strip.intersects(viewport));
  QVERIFY(!strip.intersects(track));
  QVERIFY(!strip.intersects(add));
  QVERIFY(!strip.intersects(folder));
  QVERIFY(!strip.intersects(refresh));
  QVERIFY(strip.left() >= add.right());
  QVERIFY(strip.right() <= folder.left());
  QVERIFY(strip.height() > 0);

  // Every row the list can scroll to stays clear of the footer, however long
  // the catalogue is: the scroll domain is measured off the same shortened
  // viewport.
  for (const int count : {1, 7, 8, 40}) {
    const int scroll = tramp::skinsListMaxScroll(count, viewport);
    const auto last = tramp::skinsGridCell(viewport, count - 1, scroll);
    QVERIFY(last.bottom() <= viewport.bottom());
    QVERIFY(!last.intersects(strip));
    QVERIFY(!last.intersects(add));
  }
}

void ChromeSpecTest::skinsFooterButtonsSitOnThePane() {
  const auto pane = tramp::skinsPane(tramp::kSkins);
  const auto add = tramp::skinsAddBtn(pane);
  const auto folder = tramp::skinsFolderBtn(pane);
  const auto refresh = tramp::skinsRefreshBtn(pane);
  QCOMPARE(add.size(), QSizeF(tramp::kSkinsToolBtn, tramp::kSkinsToolBtn));
  QCOMPARE(add.left(), pane.left() + tramp::kSkinsFooterPadX);
  QCOMPARE(add.bottom(), pane.bottom() - tramp::kSkinsFooterPadY);
  QCOMPARE(refresh.right(), pane.right() - tramp::kSkinsFooterPadX);
  QCOMPARE(refresh.bottom(), pane.bottom() - tramp::kSkinsFooterPadY);
  QCOMPARE(folder.right() + tramp::kSkinsToolGap, refresh.left());
  QVERIFY(!add.intersects(folder));
  QVERIFY(!folder.intersects(refresh));
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

void ChromeSpecTest::playlistStripSaveSitsLeftOfAdd() {
  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = tramp::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.save.left(), deckInner.left());
  QCOMPARE(strip.add.left() - strip.save.right(), tramp::kPlaylistStripGap);
  QVERIFY(!strip.save.intersects(strip.add));
  QVERIFY(!strip.options.intersects(strip.prev));
}

void ChromeSpecTest::playlistStripShrinksToThePlayerGutterOnceLargeButtonsWouldOverlap() {
  const qreal totalW = 140;
  // 5 tool faces + 4 gaps + sep run + cluster gap + 3 transport + 2 gaps + TOTAL + Refresh.
  const qreal largeNeed = 5 * 52 + 4 * 8 + 6 + 1 + 14 + 8 + 3 * 52 + 2 * 8 + 8 + totalW + 8 + 34;

  const auto wide = tramp::layoutPlaylistStrip(QRectF(0, 0, largeNeed, 54), totalW);
  QCOMPARE(wide.save.width(), 52.0);
  QCOMPARE(wide.save.height(), 52.0);
  QCOMPARE(wide.prev.left() - wide.options.right(), tramp::kPlaylistStripGap);

  const auto squeezed = tramp::layoutPlaylistStrip(QRectF(0, 0, largeNeed - 1, 54), totalW);
  QCOMPARE(squeezed.save.width(), tramp::kMainOptionsSize);
  QCOMPARE(squeezed.save.height(), tramp::kMainOptionsSize);
  QCOMPARE(squeezed.prev.width(), tramp::kMainOptionsSize);
  QVERIFY(!squeezed.options.intersects(squeezed.prev));
  QVERIFY(squeezed.prev.left() - squeezed.options.right() >= tramp::kPlaylistStripGap);
}

void ChromeSpecTest::playlistMinWidthIsWhereTheCompactStripWouldOverlap() {
  const qreal totalW = 140;
  const QSize min = tramp::playlistMinLogical(0, totalW);
  const QRectF body = tramp::panelBody(min);
  const QRectF deck = tramp::playlistDeckInner(
      tramp::playlistFooter(tramp::playlistTrackInner(tramp::playlistTracksPane(body, 0))));
  const auto strip = tramp::layoutPlaylistStrip(deck, totalW);
  QCOMPARE(strip.save.width(), tramp::kMainOptionsSize);
  QCOMPARE(strip.prev.left() - strip.options.right(), tramp::kPlaylistStripGap);
  QVERIFY(!strip.options.intersects(strip.prev));

  const QSize under(min.width() - 1, min.height());
  const QRectF underDeck = tramp::playlistDeckInner(tramp::playlistFooter(
      tramp::playlistTrackInner(tramp::playlistTracksPane(tramp::panelBody(under), 0))));
  const auto tight = tramp::layoutPlaylistStrip(underDeck, totalW);
  QVERIFY(tight.prev.left() - tight.options.right() < tramp::kPlaylistStripGap);

  QCOMPARE(tramp::kPlaylistMin, tramp::playlistMinLogical(0, tramp::kPlaylistStripTotalReserve));
  QCOMPARE(tramp::kPlaylistMinWithCollection,
           tramp::playlistMinLogical(tramp::kPlaylistCollectionMinWidth,
                                     tramp::kPlaylistStripTotalReserve));
}

// The footer's status run flowed from the left at its measured width with
// nothing clipping it to the strip, so a long enough playlist name painted past
// the strip's right edge and off the panel. The narrowest photographed state
// fits, which is why no picture ever showed it.
void ChromeSpecTest::longPlaylistNameGivesWayBeforeTheFooterStripDoes() {
  const QRectF strip(20, 100, 600, 26);
  const qreal tracks = 70;
  const qreal playing = 78;
  const qreal drop = 210;

  // A name with room flows as it always did: gap-dot-gap between each pair of
  // readouts, and the hint pinned to the right edge.
  const auto easy = tramp::layoutPlaylistStatus(strip, 90, tracks, playing, drop);
  QCOMPARE(easy.name, QRectF(strip.left(), strip.top(), 90, strip.height()));
  QCOMPARE(easy.nameDot,
           QPointF(easy.name.right() + tramp::kPlaylistStatusGap + tramp::kPlaylistStatusDotW / 2,
                   strip.center().y()));
  QCOMPARE(easy.tracks.left(), easy.name.right() + tramp::kPlaylistStatusSep);
  QCOMPARE(easy.playing.left(), easy.tracks.right() + tramp::kPlaylistStatusSep);
  QCOMPARE(easy.drop.right(), strip.right());
  QCOMPARE(easy.drop.width(), drop);

  // A name of any length takes only what the readouts leave, so the run ends
  // inside the strip rather than off the end of it — and the width the caller
  // elides to is the width the run hands back.
  const auto huge = tramp::layoutPlaylistStatus(strip, 40000, tracks, playing, drop);
  QCOMPARE(huge.name.width(), tramp::playlistStatusNameWidth(strip, tracks, playing));
  QVERIFY(huge.name.left() >= strip.left());
  QCOMPARE(huge.playing.right(), strip.right());
  QVERIFY(huge.drop.isEmpty());

  // The hint gives way before the name does, and gives way whole.
  const auto tight = tramp::layoutPlaylistStatus(strip, 300, tracks, playing, drop);
  QCOMPARE(tight.name.width(), 300.0);
  QVERIFY(tight.playing.right() <= strip.right());
  QVERIFY(tight.drop.isEmpty());
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
  QVERIFY(tramp::takesPointerFeedback(K::skins));
  QVERIFY(tramp::takesPointerFeedback(K::trackInfo));
  QVERIFY(tramp::takesPointerFeedback(K::plSort));
  QVERIFY(tramp::takesPointerFeedback(K::plSave));
  QVERIFY(tramp::takesPointerFeedback(K::eqPresets));
  // Hovering these would rebuild a whole panel chassis per mouse move.
  QVERIFY(!tramp::takesPointerFeedback(K::plTrackRow));
  QVERIFY(!tramp::takesPointerFeedback(K::plCollectionRow));
  QVERIFY(!tramp::takesPointerFeedback(K::settingsSkinScroll));
  QVERIFY(tramp::takesPointerFeedback(K::settingsSkinRow));
  QVERIFY(tramp::takesPointerFeedback(K::settingsSkinRemove));
  QVERIFY(!tramp::takesPointerFeedback(K::volume));
  QVERIFY(!tramp::takesPointerFeedback(K::seek));
  QVERIFY(!tramp::takesPointerFeedback(K::eqBand));
  QVERIFY(!tramp::takesPointerFeedback(K::plResize));
  QVERIFY(!tramp::takesPointerFeedback(K::none));
}

// A zoom step the layout has withdrawn takes its button's lift and its glyph's
// ink, and nothing else: the button keeps its rectangle, or the title bar
// reflows to the eye every time a step goes. The panel still records a hover on
// it — the hover label is the whole point, and it has to be asked for — so the
// pointer channels are dropped at the face instead, which is also the only
// place a golden dump or a benchmark passes through. The painted result answers
// to `host_window_move_test` and the fidelity gate; the arithmetic under it
// belongs here.
void ChromeSpecTest::aWithdrawnZoomStepLeavesAFaceThePointerCannotLight() {
  const tramp::BtnFace under(0, 1, 1);  // hovered, and held down

  const tramp::WinBtnFace live = tramp::winBtnFace(under, false, true);
  QCOMPARE(live.hover, 1.0);
  QCOMPARE(live.press, 1.0);
  QCOMPARE(live.lift, 1 + 0.24 - 0.18);
  QVERIFY(!live.dead);

  // The same pointer, on a button with nothing to take: neither channel reaches
  // the face, so there is no hover glow and no press to be had.
  const tramp::WinBtnFace dead = tramp::winBtnFace(under, false, false);
  QVERIFY(dead.dead);
  QCOMPARE(dead.hover, 0.0);
  QCOMPARE(dead.press, 0.0);
  QCOMPARE(dead.lift, tramp::kWinBtnDeadLift);

  // Dead sits below resting rather than level with it. Minimize and close are
  // in the same row, at rest, one gap away — a dead button that painted at 1.0
  // would be a live one that happens not to respond.
  const tramp::WinBtnFace resting = tramp::winBtnFace({}, false, true);
  QCOMPARE(resting.lift, 1.0);
  QVERIFY(dead.lift < resting.lift);
  QVERIFY(tramp::kWinBtnDeadGlyphAlpha < tramp::kGlyphInk.alpha());

  // Close lifts less than a neutral button under the same hover, and being
  // loud does not buy it a different dead face.
  const tramp::BtnFace hovered(0, 1, 0);
  QVERIFY(tramp::winBtnFace(hovered, true, true).lift <
          tramp::winBtnFace(hovered, false, true).lift);
  QCOMPARE(tramp::winBtnFace(hovered, true, false).lift, tramp::kWinBtnDeadLift);

  // Phases are read mid-transition, and a face handed one off either end must
  // still land inside the gradient's stops.
  const tramp::WinBtnFace clamped = tramp::winBtnFace(tramp::BtnFace(0, 4, -2), false, true);
  QCOMPARE(clamped.hover, 1.0);
  QCOMPARE(clamped.press, 0.0);
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

// A pin on the painters is only ever as broad as the reading behind it. The
// leak that cost the playlist footer its track count, its playing state and its
// drop hint was a stray `Qt::NoPen`, so the reading became pen, brush and font
// — and the About plate's stray `QPainter::SmoothPixmapTransform` then walked
// through that check reporting all-clear, because a render hint is none of the
// three. The painters themselves are held to this in `host_window_move_test`,
// which is the binary that links them and builds an application to paint with;
// what belongs here is the reading, which is the part that decides whether that
// pin can see the next leak or only the last one.
void ChromeSpecTest::thePainterReadingNamesEveryKindOfLeak() {
  const tramp::PainterState found;
  const auto leftBehind = [&](auto change) {
    tramp::PainterState after = found;
    change(after);
    return after.differencesFrom(found);
  };

  QCOMPARE(found.differencesFrom(found), QStringList());
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.pen = QPen(Qt::red, 3); }),
           QStringList{QStringLiteral("pen")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.brush = QBrush(Qt::green); }),
           QStringList{QStringLiteral("brush")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.brushOrigin = QPointF(3, 4); }),
           QStringList{QStringLiteral("brush origin")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.font = QStringLiteral("Barlow,11"); }),
           QStringList{QStringLiteral("font")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.background = QBrush(Qt::blue); }),
           QStringList{QStringLiteral("background")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.backgroundMode = Qt::OpaqueMode; }),
           QStringList{QStringLiteral("background mode")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) {
             s.composition = QPainter::CompositionMode_DestinationIn;
           }),
           QStringList{QStringLiteral("composition mode")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.opacity = 0.5; }),
           QStringList{QStringLiteral("opacity")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.clipping = true; }),
           QStringList{QStringLiteral("clip")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.clip.addRect(QRectF(0, 0, 8, 8)); }),
           QStringList{QStringLiteral("clip")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.transform.translate(2, 0); }),
           QStringList{QStringLiteral("transform")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.worldMatrix = false; }),
           QStringList{QStringLiteral("transform")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.viewport = QRect(0, 0, 4, 4); }),
           QStringList{QStringLiteral("view transform")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) {
             s.hints |= QPainter::SmoothPixmapTransform;
           }),
           QStringList{QStringLiteral("render hints")});
  QCOMPARE(leftBehind([](tramp::PainterState& s) { s.direction = Qt::RightToLeft; }),
           QStringList{QStringLiteral("layout direction")});

  // A helper that leaves two things behind has to say both, or a fix for the
  // named one reads as green while the other is still there.
  QCOMPARE(leftBehind([](tramp::PainterState& s) {
             s.pen = QPen(Qt::red, 3);
             s.hints |= QPainter::SmoothPixmapTransform;
           }),
           QStringList({QStringLiteral("pen"), QStringLiteral("render hints")}));
}

QTEST_APPLESS_MAIN(ChromeSpecTest)
#include "chrome_spec_test.moc"
