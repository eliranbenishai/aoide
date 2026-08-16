#include "window_spec.h"

#include <QTest>

class WindowSpecTest : public QObject {
  Q_OBJECT

private slots:
  void fiveWindowsInProductOrder();
  void onlyMainAppearsOnTheTaskbar();
  void extrasHaveDistinctTitles();
  void hostFlagsAreFramelessToplevelsNotTool();
};

void WindowSpecTest::fiveWindowsInProductOrder() {
  const auto specs = tramp::windowSpecs();
  QCOMPARE(specs.size(), 5);
  QCOMPARE(specs[0].id, tramp::WindowId::main);
  QCOMPARE(specs[1].id, tramp::WindowId::equalizer);
  QCOMPARE(specs[2].id, tramp::WindowId::playlist);
  QCOMPARE(specs[3].id, tramp::WindowId::settings);
  QCOMPARE(specs[4].id, tramp::WindowId::about);
}

void WindowSpecTest::onlyMainAppearsOnTheTaskbar() {
  const auto specs = tramp::windowSpecs();
  QCOMPARE(specs[0].skipTaskbar, false);
  for (int i = 1; i < 5; ++i) {
    QCOMPARE(specs[i].skipTaskbar, true);
  }
}

void WindowSpecTest::extrasHaveDistinctTitles() {
  const auto specs = tramp::windowSpecs();
  QCOMPARE(specs[0].title, QStringLiteral("Tramp"));
  QCOMPARE(specs[1].title, QStringLiteral("Equalizer"));
  QCOMPARE(specs[2].title, QStringLiteral("Playlist"));
  QCOMPARE(specs[3].title, QStringLiteral("Settings"));
  QCOMPARE(specs[4].title, QStringLiteral("About"));
}

void WindowSpecTest::hostFlagsAreFramelessToplevelsNotTool() {
  const Qt::WindowFlags flags = tramp::hostWindowFlags();
  QCOMPARE(flags & Qt::WindowType_Mask, Qt::WindowFlags(Qt::Window));
  QVERIFY(flags.testFlag(Qt::FramelessWindowHint));
  QVERIFY(!flags.testFlag(Qt::Tool));
}

QTEST_APPLESS_MAIN(WindowSpecTest)
#include "window_spec_test.moc"
