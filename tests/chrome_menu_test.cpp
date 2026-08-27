#include "chrome_menu.h"

#include <QTest>

class ChromeMenuTest : public QObject {
  Q_OBJECT

 private slots:
  void metricsScaleWithZoomAndNeverCollapse();
  void sizeCountsRulesAndTheCheckGutter();
  void rowsStackFromTheTopPadding();
  void hitTestingIgnoresPaddingRulesAndDisabledRows();
  void keyboardStepSkipsRulesAndDisabledRows();
  void keyboardStepGivesUpWhenNothingIsSelectable();
};

/// Rules and disabled rows both sit between two live rows, which is the shape
/// keyboard movement and hit-testing have to survive.
static QVector<aoide::ChromeMenuItem> sampleMenu() {
  return {
      aoide::ChromeMenuItem::action(QStringLiteral("Select all")),
      aoide::ChromeMenuItem::action(QStringLiteral("From selection"), false),
      aoide::ChromeMenuItem::separator(),
      aoide::ChromeMenuItem::check(QStringLiteral("Always on top"), true),
  };
}

void ChromeMenuTest::metricsScaleWithZoomAndNeverCollapse() {
  const aoide::ChromeMenuMetrics one = aoide::chromeMenuMetrics(1.0);
  const aoide::ChromeMenuMetrics two = aoide::chromeMenuMetrics(2.0);
  QCOMPARE(two.rowHeight, one.rowHeight * 2);
  QCOMPARE(two.ruleHeight, one.ruleHeight * 2);
  QCOMPARE(two.checkColumn, one.checkColumn * 2);
  QCOMPARE(two.labelPx, one.labelPx * 2);

  const aoide::ChromeMenuMetrics tiny = aoide::chromeMenuMetrics(0.01);
  QVERIFY(tiny.rowHeight >= 1);
  QVERIFY(tiny.padX >= 1);
  QVERIFY(tiny.labelPx >= 1);
}

void ChromeMenuTest::sizeCountsRulesAndTheCheckGutter() {
  const aoide::ChromeMenuMetrics m = aoide::chromeMenuMetrics(1.0);
  const QVector<aoide::ChromeMenuItem> items = sampleMenu();
  const QSize size = aoide::chromeMenuSize(items, 100.4, m);
  QCOMPARE(size.height(), m.padY * 2 + m.rowHeight * 3 + m.ruleHeight);
  QCOMPARE(size.width(), m.padX * 2 + m.checkColumn + 101 + m.trailing);

  QCOMPARE(aoide::chromeMenuSize({}, 0, m).height(), m.padY * 2);
}

void ChromeMenuTest::rowsStackFromTheTopPadding() {
  const aoide::ChromeMenuMetrics m = aoide::chromeMenuMetrics(1.0);
  const QVector<aoide::ChromeMenuItem> items = sampleMenu();
  QCOMPARE(aoide::chromeMenuRowTop(items, 0, m), m.padY);
  QCOMPARE(aoide::chromeMenuRowTop(items, 1, m), m.padY + m.rowHeight);
  QCOMPARE(aoide::chromeMenuRowTop(items, 2, m), m.padY + m.rowHeight * 2);
  QCOMPARE(aoide::chromeMenuRowTop(items, 3, m), m.padY + m.rowHeight * 2 + m.ruleHeight);
}

void ChromeMenuTest::hitTestingIgnoresPaddingRulesAndDisabledRows() {
  const aoide::ChromeMenuMetrics m = aoide::chromeMenuMetrics(1.0);
  const QVector<aoide::ChromeMenuItem> items = sampleMenu();
  const int last = aoide::chromeMenuRowTop(items, 3, m);
  QCOMPARE(aoide::chromeMenuRowAt(items, 0, m), aoide::kChromeMenuNone);
  QCOMPARE(aoide::chromeMenuRowAt(items, m.padY, m), 0);
  QCOMPARE(aoide::chromeMenuRowAt(items, m.padY + m.rowHeight - 1, m), 0);
  QCOMPARE(aoide::chromeMenuRowAt(items, m.padY + m.rowHeight, m), aoide::kChromeMenuNone);
  QCOMPARE(aoide::chromeMenuRowAt(items, m.padY + m.rowHeight * 2, m), aoide::kChromeMenuNone);
  QCOMPARE(aoide::chromeMenuRowAt(items, last, m), 3);
  QCOMPARE(aoide::chromeMenuRowAt(items, last + m.rowHeight - 1, m), 3);
  QCOMPARE(aoide::chromeMenuRowAt(items, last + m.rowHeight, m), aoide::kChromeMenuNone);
}

void ChromeMenuTest::keyboardStepSkipsRulesAndDisabledRows() {
  const QVector<aoide::ChromeMenuItem> items = sampleMenu();
  QCOMPARE(aoide::chromeMenuStep(items, aoide::kChromeMenuNone, 1), 0);
  QCOMPARE(aoide::chromeMenuStep(items, aoide::kChromeMenuNone, -1), 3);
  QCOMPARE(aoide::chromeMenuStep(items, 0, 1), 3);
  QCOMPARE(aoide::chromeMenuStep(items, 3, 1), 0);
  QCOMPARE(aoide::chromeMenuStep(items, 3, -1), 0);
  QCOMPARE(aoide::chromeMenuStep(items, 0, -1), 3);
}

void ChromeMenuTest::keyboardStepGivesUpWhenNothingIsSelectable() {
  const QVector<aoide::ChromeMenuItem> none{
      aoide::ChromeMenuItem::separator(),
      aoide::ChromeMenuItem::action(QStringLiteral("Clear"), false),
  };
  QCOMPARE(aoide::chromeMenuStep(none, aoide::kChromeMenuNone, 1), aoide::kChromeMenuNone);
  QCOMPARE(aoide::chromeMenuStep(none, aoide::kChromeMenuNone, -1), aoide::kChromeMenuNone);
  QCOMPARE(aoide::chromeMenuStep({}, aoide::kChromeMenuNone, 1), aoide::kChromeMenuNone);

  const QVector<aoide::ChromeMenuItem> single{aoide::ChromeMenuItem::action(QStringLiteral("Quit"))};
  QCOMPARE(aoide::chromeMenuStep(single, 0, 1), 0);
  QCOMPARE(aoide::chromeMenuStep(single, 0, -1), 0);
}

QTEST_GUILESS_MAIN(ChromeMenuTest)
#include "chrome_menu_test.moc"
