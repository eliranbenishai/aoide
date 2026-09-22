#include "window_spec.h"
#include "panel_registry.h"
#include "title_chrome.h"

#include <QTest>

class WindowSpecTest : public QObject {
  Q_OBJECT

private slots:
  void sevenPanelsInProductOrder();
  void extrasHaveDistinctTitles();
  void trackInfoIsAHiddenFreestandingPanel();
  void hostFlagsAreFramelessToplevelsNotTool();
};

void WindowSpecTest::sevenPanelsInProductOrder() {
  const auto specs = aoide::windowSpecs();
  QCOMPARE(specs.size(), 7);
  QCOMPARE(specs[0].id, aoide::WindowId::main);
  QCOMPARE(specs[1].id, aoide::WindowId::equalizer);
  QCOMPARE(specs[2].id, aoide::WindowId::playlist);
  QCOMPARE(specs[3].id, aoide::WindowId::settings);
  QCOMPARE(specs[4].id, aoide::WindowId::about);
  QCOMPARE(specs[5].id, aoide::WindowId::skins);
  QCOMPARE(specs[6].id, aoide::WindowId::trackInfo);
}

void WindowSpecTest::extrasHaveDistinctTitles() {
  const auto specs = aoide::windowSpecs();
  QCOMPARE(specs[0].title, QStringLiteral("Aoide"));
  QCOMPARE(specs[1].title, QStringLiteral("Equalizer"));
  QCOMPARE(specs[2].title, QStringLiteral("Playlist"));
  QCOMPARE(specs[3].title, QStringLiteral("Settings"));
  QCOMPARE(specs[4].title, QStringLiteral("About"));
  QCOMPARE(specs[5].title, QStringLiteral("Skins"));
  QCOMPARE(specs[6].title, QStringLiteral("Track info"));
}

void WindowSpecTest::trackInfoIsAHiddenFreestandingPanel() {
  const auto& panel = aoide::panelSpec(aoide::WindowId::trackInfo);
  QCOMPARE(panel.logicalSize, QSize(620, 550));
  QVERIFY(!panel.docks);
  QVERIFY(!panel.resizable);
  QVERIFY(!(aoide::AoideSettings{}.*panel.settingsFrame).visible);
  QVERIFY(!(aoide::DockLayout{}.*panel.layoutFrame).visible);
  QCOMPARE(aoide::panelForName(QStringLiteral("track-info")), aoide::WindowId::trackInfo);
  QCOMPARE(aoide::panelForPersistKey(QStringLiteral("trackInfo")), aoide::WindowId::trackInfo);
  QCOMPARE(aoide::roleTitle(panel.id), QStringLiteral("Track info"));
  QCOMPARE(panel.dumpName, QStringLiteral("track_info_window"));
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
