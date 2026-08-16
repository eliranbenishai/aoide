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

  // `.wbtn` 26×22, gap 5, pad-right 9 — close is the last of four.
  QCOMPARE(layout.close, QRect(790, 10, 26, 22));
  QCOMPARE(layout.minimize, QRect(697, 10, 26, 22));

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

QTEST_APPLESS_MAIN(ChromeSpecTest)
#include "chrome_spec_test.moc"
