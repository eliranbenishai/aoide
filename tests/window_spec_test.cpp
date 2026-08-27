#include "window_spec.h"

#include <QTest>

class WindowSpecTest : public QObject {
  Q_OBJECT

private slots:
  void sixPanelsInProductOrder();
  void extrasHaveDistinctTitles();
  void hostFlagsAreFramelessToplevelsNotTool();
};

void WindowSpecTest::sixPanelsInProductOrder() {
  const auto specs = aoide::windowSpecs();
  QCOMPARE(specs.size(), 6);
  QCOMPARE(specs[0].id, aoide::WindowId::main);
  QCOMPARE(specs[1].id, aoide::WindowId::equalizer);
  QCOMPARE(specs[2].id, aoide::WindowId::playlist);
  QCOMPARE(specs[3].id, aoide::WindowId::settings);
  QCOMPARE(specs[4].id, aoide::WindowId::about);
  QCOMPARE(specs[5].id, aoide::WindowId::skins);
}

void WindowSpecTest::extrasHaveDistinctTitles() {
  const auto specs = aoide::windowSpecs();
  QCOMPARE(specs[0].title, QStringLiteral("Aoide"));
  QCOMPARE(specs[1].title, QStringLiteral("Equalizer"));
  QCOMPARE(specs[2].title, QStringLiteral("Playlist"));
  QCOMPARE(specs[3].title, QStringLiteral("Settings"));
  QCOMPARE(specs[4].title, QStringLiteral("About"));
  QCOMPARE(specs[5].title, QStringLiteral("Skins"));
}

void WindowSpecTest::hostFlagsAreFramelessToplevelsNotTool() {
  const Qt::WindowFlags flags = aoide::hostWindowFlags();
  QCOMPARE(flags & Qt::WindowType_Mask, Qt::WindowFlags(Qt::Window));
  QVERIFY(flags.testFlag(Qt::FramelessWindowHint));
  QVERIFY(!flags.testFlag(Qt::Tool));
  QVERIFY(!flags.testFlag(Qt::Dialog));
  QVERIFY(!flags.testFlag(Qt::Popup));
}

QTEST_APPLESS_MAIN(WindowSpecTest)
#include "window_spec_test.moc"
