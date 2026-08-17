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
  void mainTitleShowsZoomReadoutBetweenZoomButtons();
  void chromePaintBufferMatchesWidgetTimesDpr();
  void stereoPlaylistGapHoldsForWideGlyphs();
  void skinsListScrollsLastRowIntoView();
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

  // `.wbtn` 26×22, gap 5, pad-right 9 — close is last (min − % + close).
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(648, 10, 26, 22));

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
  QCOMPARE(tramp::nextZoomPercent(75), 100);
  QCOMPARE(tramp::prevZoomPercent(75), 50);
  QCOMPARE(tramp::nextZoomPercent(300), 300);
  QCOMPARE(tramp::prevZoomPercent(50), 50);
  QCOMPARE(tramp::zoomed(tramp::kMainPlayer, 75), QSize(619, 261));
}

void ChromeSpecTest::mainTitleShowsZoomReadoutBetweenZoomButtons() {
  const auto layout =
      tramp::TitleChromeLayout::forWindow(tramp::WindowId::main, tramp::kMainPlayer);
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(648, 10, 26, 22));
  QCOMPARE(layout.zoomOut, QRect(679, 10, 26, 22));
  QCOMPARE(layout.zoomReadout, QRect(710, 10, 44, 22));
  QCOMPARE(layout.zoomIn, QRect(759, 10, 26, 22));
  QVERIFY(layout.zoomOut.right() < layout.zoomReadout.left());
  QVERIFY(layout.zoomReadout.right() < layout.zoomIn.left());
  QCOMPARE(layout.hit(layout.zoomReadout.center()), tramp::TitleChromeLayout::Hit::none);
  QVERIFY(!layout.inDragRegion(layout.zoomReadout.center()));

  const auto eq = tramp::TitleChromeLayout::forWindow(
      tramp::WindowId::equalizer, tramp::kEqualizer);
  QVERIFY(eq.zoomReadout.isEmpty());
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

QTEST_APPLESS_MAIN(ChromeSpecTest)
#include "chrome_spec_test.moc"
