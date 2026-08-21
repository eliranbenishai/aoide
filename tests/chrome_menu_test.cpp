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
static QVector<tramp::ChromeMenuItem> sampleMenu() {
  return {
      tramp::ChromeMenuItem::action(QStringLiteral("Select all")),
      tramp::ChromeMenuItem::action(QStringLiteral("From selection"), false),
      tramp::ChromeMenuItem::separator(),
      tramp::ChromeMenuItem::check(QStringLiteral("Always on top"), true),
  };
}

void ChromeMenuTest::metricsScaleWithZoomAndNeverCollapse() {
  const tramp::ChromeMenuMetrics one = tramp::chromeMenuMetrics(1.0);
  const tramp::ChromeMenuMetrics two = tramp::chromeMenuMetrics(2.0);
  QCOMPARE(two.rowHeight, one.rowHeight * 2);
  QCOMPARE(two.ruleHeight, one.ruleHeight * 2);
  QCOMPARE(two.checkColumn, one.checkColumn * 2);
  QCOMPARE(two.labelPx, one.labelPx * 2);

  const tramp::ChromeMenuMetrics tiny = tramp::chromeMenuMetrics(0.01);
  QVERIFY(tiny.rowHeight >= 1);
  QVERIFY(tiny.padX >= 1);
  QVERIFY(tiny.labelPx >= 1);
}

void ChromeMenuTest::sizeCountsRulesAndTheCheckGutter() {
  const tramp::ChromeMenuMetrics m = tramp::chromeMenuMetrics(1.0);
  const QVector<tramp::ChromeMenuItem> items = sampleMenu();
  const QSize size = tramp::chromeMenuSize(items, 100.4, m);
  QCOMPARE(size.height(), m.padY * 2 + m.rowHeight * 3 + m.ruleHeight);
  QCOMPARE(size.width(), m.padX * 2 + m.checkColumn + 101 + m.trailing);

  QCOMPARE(tramp::chromeMenuSize({}, 0, m).height(), m.padY * 2);
}

void ChromeMenuTest::rowsStackFromTheTopPadding() {
  const tramp::ChromeMenuMetrics m = tramp::chromeMenuMetrics(1.0);
  const QVector<tramp::ChromeMenuItem> items = sampleMenu();
  QCOMPARE(tramp::chromeMenuRowTop(items, 0, m), m.padY);
  QCOMPARE(tramp::chromeMenuRowTop(items, 1, m), m.padY + m.rowHeight);
  QCOMPARE(tramp::chromeMenuRowTop(items, 2, m), m.padY + m.rowHeight * 2);
  QCOMPARE(tramp::chromeMenuRowTop(items, 3, m), m.padY + m.rowHeight * 2 + m.ruleHeight);
}

void ChromeMenuTest::hitTestingIgnoresPaddingRulesAndDisabledRows() {
  const tramp::ChromeMenuMetrics m = tramp::chromeMenuMetrics(1.0);
  const QVector<tramp::ChromeMenuItem> items = sampleMenu();
  const int last = tramp::chromeMenuRowTop(items, 3, m);
  QCOMPARE(tramp::chromeMenuRowAt(items, 0, m), tramp::kChromeMenuNone);
  QCOMPARE(tramp::chromeMenuRowAt(items, m.padY, m), 0);
  QCOMPARE(tramp::chromeMenuRowAt(items, m.padY + m.rowHeight - 1, m), 0);
  QCOMPARE(tramp::chromeMenuRowAt(items, m.padY + m.rowHeight, m), tramp::kChromeMenuNone);
  QCOMPARE(tramp::chromeMenuRowAt(items, m.padY + m.rowHeight * 2, m), tramp::kChromeMenuNone);
  QCOMPARE(tramp::chromeMenuRowAt(items, last, m), 3);
  QCOMPARE(tramp::chromeMenuRowAt(items, last + m.rowHeight - 1, m), 3);
  QCOMPARE(tramp::chromeMenuRowAt(items, last + m.rowHeight, m), tramp::kChromeMenuNone);
}

void ChromeMenuTest::keyboardStepSkipsRulesAndDisabledRows() {
  const QVector<tramp::ChromeMenuItem> items = sampleMenu();
  QCOMPARE(tramp::chromeMenuStep(items, tramp::kChromeMenuNone, 1), 0);
  QCOMPARE(tramp::chromeMenuStep(items, tramp::kChromeMenuNone, -1), 3);
  QCOMPARE(tramp::chromeMenuStep(items, 0, 1), 3);
  QCOMPARE(tramp::chromeMenuStep(items, 3, 1), 0);
  QCOMPARE(tramp::chromeMenuStep(items, 3, -1), 0);
  QCOMPARE(tramp::chromeMenuStep(items, 0, -1), 3);
}

void ChromeMenuTest::keyboardStepGivesUpWhenNothingIsSelectable() {
  const QVector<tramp::ChromeMenuItem> none{
      tramp::ChromeMenuItem::separator(),
      tramp::ChromeMenuItem::action(QStringLiteral("Clear"), false),
  };
  QCOMPARE(tramp::chromeMenuStep(none, tramp::kChromeMenuNone, 1), tramp::kChromeMenuNone);
  QCOMPARE(tramp::chromeMenuStep(none, tramp::kChromeMenuNone, -1), tramp::kChromeMenuNone);
  QCOMPARE(tramp::chromeMenuStep({}, tramp::kChromeMenuNone, 1), tramp::kChromeMenuNone);

  const QVector<tramp::ChromeMenuItem> single{tramp::ChromeMenuItem::action(QStringLiteral("Quit"))};
  QCOMPARE(tramp::chromeMenuStep(single, 0, 1), 0);
  QCOMPARE(tramp::chromeMenuStep(single, 0, -1), 0);
}

QTEST_GUILESS_MAIN(ChromeMenuTest)
#include "chrome_menu_test.moc"
