#include "chrome_tooltip.h"

#include <QTest>

class ChromeTooltipTest : public QObject {
  Q_OBJECT

 private slots:
  void titleButtonsKeepFlutterNames();
  void withdrawnZoomStepsNameWhy();
  void transportGlyphsKeepFlutterNames();
  void muteAndTogglesSayWhichWay();
  void slidersAndListsStayQuiet();
  void playlistButtonsKeepFlutterNames();
  void settingsAndAboutNameControls();
  void hoverMotionHidesWhenBusyOrEmpty();
  void hoverMotionRestartsWhenTheNameChanges();
};

static tramp::ChromeHit kind(tramp::ChromeHit::Kind k, int index = -1) {
  return {k, index, {}};
}

void ChromeTooltipTest::titleButtonsKeepFlutterNames() {
  const tramp::SessionView view;
  using Hit = tramp::TitleChromeLayout::Hit;
  QCOMPARE(tramp::chromeTooltip(Hit::minimize, {}, view), QStringLiteral("Minimize"));
  QCOMPARE(tramp::chromeTooltip(Hit::collapse, {}, view), QStringLiteral("Collapse"));
  QCOMPARE(tramp::chromeTooltip(Hit::zoomOut, {}, view), QStringLiteral("Zoom out"));
  QCOMPARE(tramp::chromeTooltip(Hit::zoomIn, {}, view), QStringLiteral("Zoom in"));
  QCOMPARE(tramp::chromeTooltip(Hit::close, {}, view), QStringLiteral("Close"));
  QCOMPARE(tramp::chromeTooltip(Hit::drag, {}, view), QString());
  QCOMPARE(tramp::chromeTooltip(Hit::none, {}, view), QString());
  QCOMPARE(tramp::chromeTooltip(Hit::drag, kind(tramp::ChromeHit::Kind::play), view), QString());
}

void ChromeTooltipTest::withdrawnZoomStepsNameWhy() {
  using Hit = tramp::TitleChromeLayout::Hit;
  tramp::SessionView view;

  // The floor of the ladder is where Tramp ends. The percent is the one the
  // readout already shows, so the tip does not invent a smaller step.
  view.zoomPercent = 75;
  view.zoomOutEnabled = false;
  QCOMPARE(tramp::chromeTooltip(Hit::zoomOut, {}, view),
           QStringLiteral("75% is as small as Tramp goes"));

  // A step that will not fit is where this display ends. Closing a panel is
  // the way back — but only while one is open to close.
  view.zoomOutEnabled = true;
  view.zoomInEnabled = false;
  view.eqOn = true;
  view.plOn = false;
  QCOMPARE(tramp::chromeTooltip(Hit::zoomIn, {}, view),
           QStringLiteral("No room for 100% — close a panel"));

  view.eqOn = false;
  view.plOn = false;
  QCOMPARE(tramp::chromeTooltip(Hit::zoomIn, {}, view),
           QStringLiteral("No room for 100% on this display"));
}

void ChromeTooltipTest::transportGlyphsKeepFlutterNames() {
  const tramp::SessionView view;
  using K = tramp::ChromeHit::Kind;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::options), view), QStringLiteral("Options"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::prev), view), QStringLiteral("Previous"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::play), view), QStringLiteral("Play"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::pause), view), QStringLiteral("Pause"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::stop), view), QStringLiteral("Stop"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::next), view), QStringLiteral("Next"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eject), view), QStringLiteral("Open files"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::shuffle), view), QStringLiteral("Shuffle"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::repeat), view), QStringLiteral("Repeat"));
}

void ChromeTooltipTest::muteAndTogglesSayWhichWay() {
  tramp::SessionView view;
  using K = tramp::ChromeHit::Kind;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::mute), view), QStringLiteral("Mute"));
  view.muted = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::mute), view), QStringLiteral("Unmute"));

  view.forceMono = false;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::mono), view),
           QStringLiteral("Fold both channels to mono"));
  view.forceMono = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::mono), view), QStringLiteral("Play in stereo"));

  view.eqOn = false;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqToggle), view), QStringLiteral("Show equalizer"));
  view.eqOn = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqToggle), view), QStringLiteral("Hide equalizer"));

  view.plOn = false;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plToggle), view),
           QStringLiteral("Show Playlist Manager"));
  view.plOn = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plToggle), view),
           QStringLiteral("Hide Playlist Manager"));
}

void ChromeTooltipTest::slidersAndListsStayQuiet() {
  tramp::SessionView view;
  using K = tramp::ChromeHit::Kind;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::volume), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::seek), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::timeToggle), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqPreamp), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqBand, 0), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plCollectionRow, 0), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plTrackRow, 2), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plDivider), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plResize), view), QString());
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSkinRow, 1), view), QString());
}

void ChromeTooltipTest::playlistButtonsKeepFlutterNames() {
  tramp::SessionView view;
  using K = tramp::ChromeHit::Kind;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plCollapse), view),
           QStringLiteral("Collapse playlist collection"));
  view.collectionCollapsed = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plCollapse), view),
           QStringLiteral("Show playlist collection"));

  QCOMPARE(tramp::chromeTooltip({}, kind(K::plAddCollection), view),
           QStringLiteral("Add playlist to collection"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plCreate), view), QStringLiteral("Create playlist"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plRename), view), QStringLiteral("Rename playlist"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plRemoveCollection), view),
           QStringLiteral("Remove playlist from collection"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plAdd), view), QStringLiteral("Add tracks"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plRemove), view),
           QStringLiteral("Remove selected tracks"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plSort), view), QStringLiteral("Sort playlist"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plOptions), view), QStringLiteral("Playlist options"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plPrev), view), QStringLiteral("Previous"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plNext), view), QStringLiteral("Next"));
  view.playing = false;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plPlay), view), QStringLiteral("Play"));
  view.playing = true;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plPlay), view), QStringLiteral("Pause"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::plRefresh), view), QStringLiteral("Refresh playlist"));

  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqOn), view), QStringLiteral("Equalizer on"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqAuto), view), QStringLiteral("Auto"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::eqPresets), view), QStringLiteral("Presets"));
}

void ChromeTooltipTest::settingsAndAboutNameControls() {
  const tramp::SessionView view;
  using K = tramp::ChromeHit::Kind;
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsGeneral), view), QStringLiteral("General"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSkins), view), QStringLiteral("Skins"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsResume), view),
           QStringLiteral("Resume last session"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsConfirm), view),
           QStringLiteral("Confirm before quit"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsScroll), view), QStringLiteral("Scroll title"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsMinimize), view),
           QStringLiteral("Minimize hides secondaries"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSnapOff), view), QStringLiteral("Dock snap off"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSnapNormal), view),
           QStringLiteral("Dock snap normal"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSnapStrong), view),
           QStringLiteral("Dock snap strong"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsReset), view), QStringLiteral("Reset Settings"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsInstallZip), view),
           QStringLiteral("Install zip"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsInstallFolder), view),
           QStringLiteral("Install folder"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsSkinsFolder), view),
           QStringLiteral("Skins folder"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::settingsResetSkinsFolder), view),
           QStringLiteral("Reset folder"));
  QCOMPARE(tramp::chromeTooltip({}, kind(K::aboutWeb), view), QStringLiteral("Open tramp.music"));
}

void ChromeTooltipTest::hoverMotionHidesWhenBusyOrEmpty() {
  QCOMPARE(tramp::tooltipMotion(QStringLiteral("Play"), QString(), false),
           tramp::TooltipMotion::hide);
  QCOMPARE(tramp::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Play"), true),
           tramp::TooltipMotion::hide);
  QCOMPARE(tramp::tooltipMotion(QString(), QString(), false), tramp::TooltipMotion::hide);
}

void ChromeTooltipTest::hoverMotionRestartsWhenTheNameChanges() {
  QCOMPARE(tramp::tooltipMotion(QString(), QStringLiteral("Play"), false, true),
           tramp::TooltipMotion::restartWait);
  QCOMPARE(tramp::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Pause"), false, true),
           tramp::TooltipMotion::restartWait);
  QCOMPARE(tramp::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Play"), false, true),
           tramp::TooltipMotion::keep);
  QCOMPARE(tramp::tooltipMotion(QStringLiteral("Previous"), QStringLiteral("Previous"), false, false),
           tramp::TooltipMotion::restartWait);
}

QTEST_MAIN(ChromeTooltipTest)
#include "chrome_tooltip_test.moc"
