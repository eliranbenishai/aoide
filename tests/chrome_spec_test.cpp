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
  QCOMPARE(aoide::kShellHi.name(QColor::HexRgb).toLower(), QStringLiteral("#323744"));
  QCOMPARE(aoide::kShell.name(QColor::HexRgb).toLower(), QStringLiteral("#262b38"));
  QCOMPARE(aoide::kShellMid.name(QColor::HexRgb).toLower(), QStringLiteral("#1a1d26"));
  QCOMPARE(aoide::kShellLo.name(QColor::HexRgb).toLower(), QStringLiteral("#12141a"));
  QCOMPARE(aoide::kShellDeep.name(QColor::HexRgb).toLower(), QStringLiteral("#0a0b0e"));
  QCOMPARE(aoide::kInk.name(QColor::HexRgb).toLower(), QStringLiteral("#e8eaf0"));
  QCOMPARE(aoide::kPhos.name(QColor::HexRgb).toLower(), QStringLiteral("#3de7ff"));
  QCOMPARE(aoide::kAccent.name(QColor::HexRgb).toLower(), QStringLiteral("#ff3d9a"));
  QCOMPARE(aoide::kWell.name(QColor::HexRgb).toLower(), QStringLiteral("#050608"));
  QCOMPARE(aoide::kBtnOn0.name(QColor::HexRgb).toLower(), QStringLiteral("#a3f4ff"));
  QCOMPARE(aoide::kSliderFillHi.name(QColor::HexRgb).toLower(), QStringLiteral("#cbf9ff"));
}

void ChromeSpecTest::titleBarStopsMatchMockupStylesheet() {
  QCOMPARE(aoide::kTitleBar0.name(QColor::HexRgb).toLower(), QStringLiteral("#3c4356"));
  QCOMPARE(aoide::kTitleBar26.name(QColor::HexRgb).toLower(), QStringLiteral("#2c3241"));
  QCOMPARE(aoide::kTitleBar62.name(QColor::HexRgb).toLower(), QStringLiteral("#1d222c"));
  QCOMPARE(aoide::kTitleBar100.name(QColor::HexRgb).toLower(), QStringLiteral("#12151c"));
  QCOMPARE(aoide::kWordmark.name(QColor::HexRgb).toLower(), QStringLiteral("#eaf2ff"));
  QCOMPARE(aoide::kWbtnClose0.name(QColor::HexRgb).toLower(), QStringLiteral("#9c2a60"));
  QCOMPARE(aoide::kWindowName.red(), 200);
  QCOMPARE(aoide::kWindowName.green(), 214);
  QCOMPARE(aoide::kWindowName.blue(), 235);
  QCOMPARE(aoide::kWindowName.alpha(), 140);
}

void ChromeSpecTest::nativeSeedIsRoundedSeventyFivePercent() {
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kMainPlayer), QSize(619, 261));
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kEqualizer), QSize(619, 261));
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kPlaylistDefault), QSize(805, 522));
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kSettings), QSize(390, 315));
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kAbout), QSize(360, 270));
  QCOMPARE(aoide::nativeUnmappedSeed(aoide::kSkins), QSize(450, 360));
}

void ChromeSpecTest::windowSpecsUseNativeSeeds() {
  const auto specs = aoide::windowSpecs();
  QCOMPARE(specs[0].logicalSize, aoide::kMainPlayer);
  QCOMPARE(specs[1].logicalSize, aoide::kEqualizer);
  QCOMPARE(specs[2].logicalSize, aoide::kPlaylistDefault);
  QCOMPARE(specs[3].logicalSize, aoide::kSettings);
  QCOMPARE(specs[4].logicalSize, aoide::kAbout);
  QCOMPARE(specs[5].logicalSize, aoide::kSkins);
  for (const aoide::WindowSpec& spec : specs) {
    QCOMPARE(spec.size, aoide::nativeUnmappedSeed(spec.logicalSize));
  }
}

void ChromeSpecTest::gutterStacksSkinsAndTrackInfoUnderTheCog() {
  const aoide::MainDisplayRow row = aoide::layoutMainDisplay(aoide::panelBody(aoide::kMainPlayer));
  QCOMPARE(row.options.size(), QSizeF(26, 26));
  QCOMPARE(row.skins.size(), QSizeF(26, 26));
  QCOMPARE(row.trackInfo.size(), QSizeF(26, 26));
  QCOMPARE(row.skins.left(), row.options.left());
  QCOMPARE(row.trackInfo.left(), row.options.left());
  QCOMPARE(row.skins.top(), row.options.bottom() + aoide::kMainGutterBtnGap);
  QCOMPARE(row.trackInfo.top(), row.skins.bottom() + aoide::kMainGutterBtnGap);
  QVERIFY(row.trackInfo.bottom() < row.well.bottom());
}

void ChromeSpecTest::mainTitleDragExcludesWindowButtons() {
  const auto layout =
      aoide::TitleChromeLayout::forWindow(aoide::WindowId::main, aoide::kMainPlayer);
  QVERIFY(layout.showBrand);
  QVERIFY(layout.showZoom);
  QCOMPARE(layout.roleName, QStringLiteral("Main Player"));

  // `.wbtn` 26×22, gap 5, pad-right 9 — zoom cluster, then a 12px gap, then
  // minimize flush against close.
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(759, 10, 26, 22));

  const QPoint closeCenter = layout.close.center();
  QCOMPARE(layout.hit(closeCenter), aoide::TitleChromeLayout::Hit::close);
  QVERIFY(!layout.inDragRegion(closeCenter));

  const QPoint inGrips(120, 21);
  QCOMPARE(layout.hit(inGrips), aoide::TitleChromeLayout::Hit::drag);
  QVERIFY(layout.inDragRegion(inGrips));
}

void ChromeSpecTest::extrasOmitBrandAndZoomAndUseCollapse() {
  const auto eq = aoide::TitleChromeLayout::forWindow(
      aoide::WindowId::equalizer, aoide::kEqualizer);
  QVERIFY(!eq.showBrand);
  QVERIFY(!eq.showZoom);
  QCOMPARE(eq.roleName, QStringLiteral("Equalizer"));
  QCOMPARE(eq.hit(eq.minimize.center()), aoide::TitleChromeLayout::Hit::collapse);
  QCOMPARE(eq.hit(eq.close.center()), aoide::TitleChromeLayout::Hit::close);
  QVERIFY(eq.zoomOut.isEmpty());
  QVERIFY(eq.zoomIn.isEmpty());

  const auto pl = aoide::TitleChromeLayout::forWindow(
      aoide::WindowId::playlist, aoide::kPlaylistDefault);
  QCOMPARE(pl.roleName, QStringLiteral("Playlist Manager"));
  QVERIFY(!pl.showBrand);

  const auto skins = aoide::TitleChromeLayout::forWindow(aoide::WindowId::skins, aoide::kSkins);
  QCOMPARE(skins.roleName, QStringLiteral("Skins"));
  QVERIFY(!skins.showBrand);
  QVERIFY(!skins.showZoom);
  QCOMPARE(skins.hit(skins.minimize.center()), aoide::TitleChromeLayout::Hit::collapse);
}

void ChromeSpecTest::zoomStepsMoveAcrossTheDiscreteLadder() {
  // Four steps — 75, 100, 125, 150 — with 75% the default.
  QCOMPARE(aoide::kDefaultZoomPercent, 75);
  QCOMPARE(aoide::nextZoomPercent(75), 100);
  QCOMPARE(aoide::nextZoomPercent(100), 125);
  QCOMPARE(aoide::nextZoomPercent(125), 150);
  QCOMPARE(aoide::prevZoomPercent(150), 125);
  QCOMPARE(aoide::prevZoomPercent(125), 100);
  QCOMPARE(aoide::prevZoomPercent(100), 75);

  // Both ends clamp. Stepping off either one must hold, not wrap round to the
  // far end and not run past the ladder onto a percent nothing else knows.
  QCOMPARE(aoide::nextZoomPercent(150), 150);
  QCOMPARE(aoide::prevZoomPercent(75), 75);

  QCOMPARE(aoide::zoomed(aoide::kMainPlayer, 75), QSize(619, 261));
}

void ChromeSpecTest::retiredZoomStepsSnapToTheNearestSurvivor() {
  // A listener who ran the eight-step build has 50% or 200% saved.
  QCOMPARE(aoide::snapZoomPercent(50), 75);
  QCOMPARE(aoide::snapZoomPercent(200), 150);
  QCOMPARE(aoide::snapZoomPercent(250), 150);
  QCOMPARE(aoide::snapZoomPercent(300), 150);

  // Surviving steps pass through untouched; nonsense still lands on a step.
  for (int step : aoide::kZoomSteps) {
    QCOMPARE(aoide::snapZoomPercent(step), step);
  }
  QCOMPARE(aoide::snapZoomPercent(0), 75);
  QCOMPARE(aoide::snapZoomPercent(-40), 75);

  // Nearest wins on either side of the gap between two steps.
  QCOMPARE(aoide::snapZoomPercent(87), 75);
  QCOMPARE(aoide::snapZoomPercent(88), 100);
}

void ChromeSpecTest::mainTitleShowsZoomReadoutBetweenZoomButtons() {
  const auto layout =
      aoide::TitleChromeLayout::forWindow(aoide::WindowId::main, aoide::kMainPlayer);
  QCOMPARE(layout.zoomOut, QRect(641, 10, 26, 22));
  QCOMPARE(layout.zoomReadout, QRect(672, 10, 44, 22));
  QCOMPARE(layout.zoomIn, QRect(721, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(759, 10, 26, 22));
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QVERIFY(layout.zoomOut.right() < layout.zoomReadout.left());
  QVERIFY(layout.zoomReadout.right() < layout.zoomIn.left());
  QCOMPARE(layout.minimize.left(), layout.zoomIn.x() + layout.zoomIn.width() + 12);
  QCOMPARE(layout.close.left(), layout.minimize.x() + layout.minimize.width() + 5);
  QCOMPARE(layout.hit(layout.zoomReadout.center()), aoide::TitleChromeLayout::Hit::none);
  QVERIFY(!layout.inDragRegion(layout.zoomReadout.center()));

  const auto eq = aoide::TitleChromeLayout::forWindow(
      aoide::WindowId::equalizer, aoide::kEqualizer);
  QVERIFY(eq.zoomReadout.isEmpty());
}

void ChromeSpecTest::overflowingTitleMarqueeHoldsThenLoops() {
  QCOMPARE(aoide::marqueeOffset(80, 100, 5000, true), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 5000, false), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 0, true), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 1199, true), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 1200, true), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 2200, true), 40.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 7200, true), 0.0);
  QCOMPARE(aoide::marqueeOffset(200, 100, 8200, true), 40.0);
  QVERIFY(!aoide::displayTitleOnLivePass(aoide::marqueeOffset(200, 100, 0, true)));
  QVERIFY(!aoide::displayTitleOnLivePass(aoide::marqueeOffset(80, 100, 5000, true)));
  QVERIFY(aoide::displayTitleOnLivePass(aoide::marqueeOffset(200, 100, 2200, true)));
  QCOMPARE(aoide::kDisplayTitleClipW, 385.0);
}

void ChromeSpecTest::chromePaintBufferMatchesWidgetTimesDpr() {
  // 75% of 825×348 is 619×261. A 2× display must rasterize at 1238×522, not
  // upscale a 1× chassis. 1.25× DPR rounds 619×1.25 = 773.75 → 774.
  QCOMPARE(aoide::chromePaintBufferSize(QSize(619, 261), 2.0), QSize(1238, 522));
  QCOMPARE(aoide::chromePaintBufferSize(QSize(619, 261), 1.0), QSize(619, 261));
  QCOMPARE(aoide::chromePaintBufferSize(QSize(619, 261), 1.25), QSize(774, 326));
}

void ChromeSpecTest::stereoPlaylistGapHoldsForWideGlyphs() {
  const QRectF inner(0, 0, 400, 18);
  const auto compact = aoide::layoutDisplayMetaRow(inner, 0, 40, 40, 50, 70, 36);
  QVERIFY(compact.playlist.left() - compact.channels.right() >= aoide::kDisplayMetaGap);
  QVERIFY(!compact.channels.intersects(compact.playlist));

  const auto wide = aoide::layoutDisplayMetaRow(inner, 0, 80, 80, 160, 120, 48);
  QCOMPARE(wide.playlist.left() - wide.channels.right(), aoide::kDisplayMetaGap);
  QVERIFY(!wide.channels.intersects(wide.playlist));
}

void ChromeSpecTest::skinsListScrollsLastRowIntoView() {
  const auto pane = aoide::skinsPane(aoide::kSkins);
  const auto viewport = aoide::skinsListViewport(pane);
  QCOMPARE(int(viewport.height()), 372);
  QVERIFY(aoide::skinsListMaxScroll(8, viewport) > 0);

  const auto lastHidden = aoide::skinsGridCell(viewport, 7, 0);
  QVERIFY(lastHidden.bottom() > viewport.bottom());

  const int scroll = aoide::skinsListMaxScroll(8, viewport);
  const auto lastShown = aoide::skinsGridCell(viewport, 7, scroll);
  QVERIFY(lastShown.top() >= viewport.top());
  QVERIFY(lastShown.bottom() <= viewport.bottom());
}

// The install error used to be placed off the pane's bottom edge rather than
// off the list, so it landed on the last rows of any catalogue long enough to
// scroll and ran past the viewport into the scrollbar track. It now has a strip
// of its own, and nothing the pane paints may be under it.
void ChromeSpecTest::skinsErrorStripClearsTheListAndTheScrollbar() {
  const auto pane = aoide::skinsPane(aoide::kSkins);
  const auto viewport = aoide::skinsListViewport(pane);
  const auto strip = aoide::skinsErrorStrip(pane);
  const auto track = aoide::skinsListScrollTrack(viewport);
  const auto add = aoide::skinsAddBtn(pane);
  const auto folder = aoide::skinsFolderBtn(pane);
  const auto refresh = aoide::skinsRefreshBtn(pane);

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
    const int scroll = aoide::skinsListMaxScroll(count, viewport);
    const auto last = aoide::skinsGridCell(viewport, count - 1, scroll);
    QVERIFY(last.bottom() <= viewport.bottom());
    QVERIFY(!last.intersects(strip));
    QVERIFY(!last.intersects(add));
  }
}

void ChromeSpecTest::skinsFooterButtonsSitOnThePane() {
  const auto pane = aoide::skinsPane(aoide::kSkins);
  const auto add = aoide::skinsAddBtn(pane);
  const auto folder = aoide::skinsFolderBtn(pane);
  const auto refresh = aoide::skinsRefreshBtn(pane);
  QCOMPARE(add.size(), QSizeF(aoide::kSkinsToolBtn, aoide::kSkinsToolBtn));
  QCOMPARE(add.left(), pane.left() + aoide::kSkinsFooterPadX);
  QCOMPARE(add.bottom(), pane.bottom() - aoide::kSkinsFooterPadY);
  QCOMPARE(refresh.right(), pane.right() - aoide::kSkinsFooterPadX);
  QCOMPARE(refresh.bottom(), pane.bottom() - aoide::kSkinsFooterPadY);
  QCOMPARE(folder.right() + aoide::kSkinsToolGap, refresh.left());
  QVERIFY(!add.intersects(folder));
  QVERIFY(!folder.intersects(refresh));
}

void ChromeSpecTest::playlistHidesScrollbarWhenRowsFit() {
  const qreal wellH = aoide::playlistListWellHeight(aoide::kPlaylistDefault.height());
  QCOMPARE(aoide::playlistListMaxScroll(1, wellH), 0);
  QCOMPARE(aoide::playlistListMaxScroll(aoide::playlistVisibleRows(wellH), wellH), 0);
  QVERIFY(aoide::playlistListMaxScroll(aoide::playlistVisibleRows(wellH) + 1, wellH) > 0);

  const QRectF listRow(0, 0, 400, wellH);
  const QRectF fitting = aoide::playlistListWell(listRow, 3);
  QCOMPARE(fitting.width(), listRow.width());
  const QRectF overflowing = aoide::playlistListWell(listRow, aoide::playlistVisibleRows(wellH) + 8);
  QCOMPARE(overflowing.width(),
           listRow.width() - aoide::kPlaylistScrollGap - aoide::kPlaylistScrollW);
}

// Collapsed, the tab is the whole of the collection: it is the only thing
// painted down that edge and the only thing that reopens the pane. Track rows
// reaching into its column are rows the reopen region takes the left edge off,
// with nothing on screen to say why the click went elsewhere.
void ChromeSpecTest::collapsedCollectionKeepsTheColumnItsReopenTabPaintsIn() {
  const QRectF body = aoide::panelBody(aoide::kPlaylistDefault);
  const QRectF tab = aoide::playlistReopenTab(body);
  const QRectF collapsed = aoide::playlistTracksPane(body, 0);
  QVERIFY(!tab.isEmpty());
  QVERIFY(!tab.intersects(collapsed));
  QCOMPARE(collapsed.right(), body.right());

  // Expanded, the pane keeps the divider it always had and the tab is not painted.
  const QRectF expanded = aoide::playlistTracksPane(body, 240);
  QCOMPARE(expanded.left(), body.left() + 240 + aoide::kPlaylistDividerW);
  QVERIFY(expanded.width() < collapsed.width());
}

void ChromeSpecTest::playlistStripKeepsGapBeforeLengthWell() {
  // player-mockup-2.html `.pl-strip { gap: 8px }` — Next must not sit flush on TOTAL.
  QCOMPARE(aoide::kPlaylistStripGap, 8.0);

  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = aoide::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.total.left() - strip.next.right(), aoide::kPlaylistStripGap);
  QVERIFY(!strip.next.intersects(strip.total));
  QCOMPARE(strip.play.left() - strip.prev.right(), aoide::kPlaylistStripGap);
  QCOMPARE(strip.next.left() - strip.play.right(), aoide::kPlaylistStripGap);
}

void ChromeSpecTest::playlistStripRefreshSitsRightOfTotal() {
  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = aoide::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.refresh.left() - strip.total.right(), aoide::kPlaylistStripGap);
  QCOMPARE(strip.refresh.right(), deckInner.right());
  QCOMPARE(strip.total.left() - strip.next.right(), aoide::kPlaylistStripGap);
  QVERIFY(!strip.total.intersects(strip.refresh));
}

void ChromeSpecTest::playlistStripSaveSitsLeftOfAdd() {
  const QRectF deckInner(0, 0, 800, 54);
  const auto strip = aoide::layoutPlaylistStrip(deckInner, 140);
  QCOMPARE(strip.save.left(), deckInner.left());
  QCOMPARE(strip.add.left() - strip.save.right(), aoide::kPlaylistStripGap);
  QVERIFY(!strip.save.intersects(strip.add));
  QVERIFY(!strip.options.intersects(strip.prev));
}

void ChromeSpecTest::playlistStripShrinksToThePlayerGutterOnceLargeButtonsWouldOverlap() {
  const qreal totalW = 140;
  // 5 tool faces + 4 gaps + sep run + cluster gap + 3 transport + 2 gaps + TOTAL + Refresh.
  const qreal largeNeed = 5 * 52 + 4 * 8 + 6 + 1 + 14 + 8 + 3 * 52 + 2 * 8 + 8 + totalW + 8 + 34;

  const auto wide = aoide::layoutPlaylistStrip(QRectF(0, 0, largeNeed, 54), totalW);
  QCOMPARE(wide.save.width(), 52.0);
  QCOMPARE(wide.save.height(), 52.0);
  QCOMPARE(wide.prev.left() - wide.options.right(), aoide::kPlaylistStripGap);

  const auto squeezed = aoide::layoutPlaylistStrip(QRectF(0, 0, largeNeed - 1, 54), totalW);
  QCOMPARE(squeezed.save.width(), aoide::kMainOptionsSize);
  QCOMPARE(squeezed.save.height(), aoide::kMainOptionsSize);
  QCOMPARE(squeezed.prev.width(), aoide::kMainOptionsSize);
  QVERIFY(!squeezed.options.intersects(squeezed.prev));
  QVERIFY(squeezed.prev.left() - squeezed.options.right() >= aoide::kPlaylistStripGap);
}

void ChromeSpecTest::playlistMinWidthIsWhereTheCompactStripWouldOverlap() {
  const qreal totalW = 140;
  const QSize min = aoide::playlistMinLogical(0, totalW);
  const QRectF body = aoide::panelBody(min);
  const QRectF deck = aoide::playlistDeckInner(
      aoide::playlistFooter(aoide::playlistTrackInner(aoide::playlistTracksPane(body, 0))));
  const auto strip = aoide::layoutPlaylistStrip(deck, totalW);
  QCOMPARE(strip.save.width(), aoide::kMainOptionsSize);
  QCOMPARE(strip.prev.left() - strip.options.right(), aoide::kPlaylistStripGap);
  QVERIFY(!strip.options.intersects(strip.prev));

  const QSize under(min.width() - 1, min.height());
  const QRectF underDeck = aoide::playlistDeckInner(aoide::playlistFooter(
      aoide::playlistTrackInner(aoide::playlistTracksPane(aoide::panelBody(under), 0))));
  const auto tight = aoide::layoutPlaylistStrip(underDeck, totalW);
  QVERIFY(tight.prev.left() - tight.options.right() < aoide::kPlaylistStripGap);

  QCOMPARE(aoide::kPlaylistMin, aoide::playlistMinLogical(0, aoide::kPlaylistStripTotalReserve));
  QCOMPARE(aoide::kPlaylistMinWithCollection,
           aoide::playlistMinLogical(aoide::kPlaylistCollectionMinWidth,
                                     aoide::kPlaylistStripTotalReserve));
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
  const auto easy = aoide::layoutPlaylistStatus(strip, 90, tracks, playing, drop);
  QCOMPARE(easy.name, QRectF(strip.left(), strip.top(), 90, strip.height()));
  QCOMPARE(easy.nameDot,
           QPointF(easy.name.right() + aoide::kPlaylistStatusGap + aoide::kPlaylistStatusDotW / 2,
                   strip.center().y()));
  QCOMPARE(easy.tracks.left(), easy.name.right() + aoide::kPlaylistStatusSep);
  QCOMPARE(easy.playing.left(), easy.tracks.right() + aoide::kPlaylistStatusSep);
  QCOMPARE(easy.drop.right(), strip.right());
  QCOMPARE(easy.drop.width(), drop);

  // A name of any length takes only what the readouts leave, so the run ends
  // inside the strip rather than off the end of it — and the width the caller
  // elides to is the width the run hands back.
  const auto huge = aoide::layoutPlaylistStatus(strip, 40000, tracks, playing, drop);
  QCOMPARE(huge.name.width(), aoide::playlistStatusNameWidth(strip, tracks, playing));
  QVERIFY(huge.name.left() >= strip.left());
  QCOMPARE(huge.playing.right(), strip.right());
  QVERIFY(huge.drop.isEmpty());

  // The hint gives way before the name does, and gives way whole.
  const auto tight = aoide::layoutPlaylistStatus(strip, 300, tracks, playing, drop);
  QCOMPARE(tight.name.width(), 300.0);
  QVERIFY(tight.playing.right() <= strip.right());
  QVERIFY(tight.drop.isEmpty());
}

void ChromeSpecTest::buttonPhaseTakesTheWholeTransitionWhateverTheFrameRate() {
  using aoide::BtnChannel;
  using K = aoide::ChromeHit::Kind;

  // A panel that can only manage a few frames must still finish on time, so the
  // step is wall-clock and not per-frame.
  aoide::ChromePhases coarse;
  coarse.setLive(true);
  coarse.setTarget(K::shuffle, -1, BtnChannel::on, 1);
  QVERIFY(coarse.moving());
  QVERIFY(coarse.advance(aoide::kBtnTransitionMs / 2));
  QVERIFY(!coarse.advance(aoide::kBtnTransitionMs / 2));
  QVERIFY(!coarse.moving());
  QCOMPARE(coarse.face(K::shuffle).on, 1.0);

  aoide::ChromePhases fine;
  fine.setLive(true);
  fine.setTarget(K::shuffle, -1, BtnChannel::on, 1);
  int frames = 0;
  while (fine.advance(16) && frames < 1000) ++frames;
  QCOMPARE(fine.face(K::shuffle).on, 1.0);
  QVERIFY(frames >= int(aoide::kBtnTransitionMs / 16) - 1);

  // Mid-transition the face is neither of its two states, which is the point.
  aoide::ChromePhases part;
  part.setLive(true);
  part.setTarget(K::mute, -1, BtnChannel::on, 1);
  part.advance(aoide::kBtnTransitionMs / 2);
  const qreal half = part.face(K::mute).on;
  QVERIFY(half > 0.0);
  QVERIFY(half < 1.0);
}

void ChromeSpecTest::inertPhaseStoreLeavesPaintersOnPlainSessionState() {
  // Golden dumps and tests paint without a panel behind them; painters key off
  // live() to fall back to the session's booleans, so lit buttons stay lit.
  aoide::ChromePhases inert;
  QVERIFY(!inert.live());
  aoide::ChromePhases driven;
  driven.setLive(true);
  QVERIFY(driven.live());
}

void ChromeSpecTest::pointerFeedbackSkipsSlidersAndListRows() {
  using K = aoide::ChromeHit::Kind;
  QVERIFY(aoide::takesPointerFeedback(K::play));
  QVERIFY(aoide::takesPointerFeedback(K::skins));
  QVERIFY(aoide::takesPointerFeedback(K::trackInfo));
  QVERIFY(aoide::takesPointerFeedback(K::plSort));
  QVERIFY(aoide::takesPointerFeedback(K::plSave));
  QVERIFY(aoide::takesPointerFeedback(K::eqPresets));
  QVERIFY(aoide::takesPointerFeedback(K::settingsReset));
  QVERIFY(aoide::takesPointerFeedback(K::settingsAudioDevice));
  QVERIFY(aoide::takesPointerFeedback(K::settingsExclusive));
  // Hovering these would rebuild a whole panel chassis per mouse move.
  QVERIFY(!aoide::takesPointerFeedback(K::plTrackRow));
  QVERIFY(!aoide::takesPointerFeedback(K::plCollectionRow));
  QVERIFY(!aoide::takesPointerFeedback(K::settingsSkinScroll));
  QVERIFY(aoide::takesPointerFeedback(K::settingsSkinRow));
  QVERIFY(aoide::takesPointerFeedback(K::settingsSkinRemove));
  QVERIFY(!aoide::takesPointerFeedback(K::volume));
  QVERIFY(!aoide::takesPointerFeedback(K::seek));
  QVERIFY(!aoide::takesPointerFeedback(K::eqBand));
  QVERIFY(!aoide::takesPointerFeedback(K::plResize));
  QVERIFY(!aoide::takesPointerFeedback(K::none));
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
  const aoide::BtnFace under(0, 1, 1);  // hovered, and held down

  const aoide::WinBtnFace live = aoide::winBtnFace(under, false, true);
  QCOMPARE(live.hover, 1.0);
  QCOMPARE(live.press, 1.0);
  QCOMPARE(live.lift, 1 + 0.24 - 0.18);
  QVERIFY(!live.dead);

  // The same pointer, on a button with nothing to take: neither channel reaches
  // the face, so there is no hover glow and no press to be had.
  const aoide::WinBtnFace dead = aoide::winBtnFace(under, false, false);
  QVERIFY(dead.dead);
  QCOMPARE(dead.hover, 0.0);
  QCOMPARE(dead.press, 0.0);
  QCOMPARE(dead.lift, aoide::kWinBtnDeadLift);

  // Dead sits below resting rather than level with it. Minimize and close are
  // in the same row, at rest, one gap away — a dead button that painted at 1.0
  // would be a live one that happens not to respond.
  const aoide::WinBtnFace resting = aoide::winBtnFace({}, false, true);
  QCOMPARE(resting.lift, 1.0);
  QVERIFY(dead.lift < resting.lift);
  QVERIFY(aoide::kWinBtnDeadGlyphAlpha < aoide::kGlyphInk.alpha());

  // Close lifts less than a neutral button under the same hover, and being
  // loud does not buy it a different dead face.
  const aoide::BtnFace hovered(0, 1, 0);
  QVERIFY(aoide::winBtnFace(hovered, true, true).lift <
          aoide::winBtnFace(hovered, false, true).lift);
  QCOMPARE(aoide::winBtnFace(hovered, true, false).lift, aoide::kWinBtnDeadLift);

  // Phases are read mid-transition, and a face handed one off either end must
  // still land inside the gradient's stops.
  const aoide::WinBtnFace clamped = aoide::winBtnFace(aoide::BtnFace(0, 4, -2), false, true);
  QCOMPARE(clamped.hover, 1.0);
  QCOMPARE(clamped.press, 0.0);
}

void ChromeSpecTest::settledPhasesDoNotAccumulate() {
  using aoide::BtnChannel;
  using K = aoide::ChromeHit::Kind;
  aoide::ChromePhases phases;
  phases.setLive(true);
  // Asking an untouched control for zero must not record anything: every view
  // the session publishes aims every latched button, most of them at zero.
  for (int i = 0; i < 100; ++i) phases.setTarget(K::stop, -1, BtnChannel::on, 0);
  QVERIFY(!phases.moving());

  // A pointer crossing a row of buttons leaves one cooling entry each; they must
  // be reclaimed once they reach the floor.
  phases.setTarget(K::prev, -1, BtnChannel::hover, 1);
  phases.advance(aoide::kBtnTransitionMs);
  phases.releaseChannel(BtnChannel::hover);
  phases.advance(aoide::kBtnTransitionMs);
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
  const aoide::PainterState found;
  const auto leftBehind = [&](auto change) {
    aoide::PainterState after = found;
    change(after);
    return after.differencesFrom(found);
  };

  QCOMPARE(found.differencesFrom(found), QStringList());
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.pen = QPen(Qt::red, 3); }),
           QStringList{QStringLiteral("pen")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.brush = QBrush(Qt::green); }),
           QStringList{QStringLiteral("brush")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.brushOrigin = QPointF(3, 4); }),
           QStringList{QStringLiteral("brush origin")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.font = QStringLiteral("Barlow,11"); }),
           QStringList{QStringLiteral("font")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.background = QBrush(Qt::blue); }),
           QStringList{QStringLiteral("background")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.backgroundMode = Qt::OpaqueMode; }),
           QStringList{QStringLiteral("background mode")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) {
             s.composition = QPainter::CompositionMode_DestinationIn;
           }),
           QStringList{QStringLiteral("composition mode")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.opacity = 0.5; }),
           QStringList{QStringLiteral("opacity")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.clipping = true; }),
           QStringList{QStringLiteral("clip")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.clip.addRect(QRectF(0, 0, 8, 8)); }),
           QStringList{QStringLiteral("clip")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.transform.translate(2, 0); }),
           QStringList{QStringLiteral("transform")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.worldMatrix = false; }),
           QStringList{QStringLiteral("transform")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.viewport = QRect(0, 0, 4, 4); }),
           QStringList{QStringLiteral("view transform")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) {
             s.hints |= QPainter::SmoothPixmapTransform;
           }),
           QStringList{QStringLiteral("render hints")});
  QCOMPARE(leftBehind([](aoide::PainterState& s) { s.direction = Qt::RightToLeft; }),
           QStringList{QStringLiteral("layout direction")});

  // A helper that leaves two things behind has to say both, or a fix for the
  // named one reads as green while the other is still there.
  QCOMPARE(leftBehind([](aoide::PainterState& s) {
             s.pen = QPen(Qt::red, 3);
             s.hints |= QPainter::SmoothPixmapTransform;
           }),
           QStringList({QStringLiteral("pen"), QStringLiteral("render hints")}));
}

QTEST_APPLESS_MAIN(ChromeSpecTest)
#include "chrome_spec_test.moc"
