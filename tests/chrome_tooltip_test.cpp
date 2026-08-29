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
  void saveButtonIsDeadUntilTheListIsAltered();
  void settingsAndAboutNameControls();
  void skinsCellsStayQuiet();
  void hoverMotionHidesWhenBusyOrEmpty();
  void hoverMotionRestartsWhenTheNameChanges();
};

static aoide::ChromeHit kind(aoide::ChromeHit::Kind k, int index = -1) {
  return {k, index, {}};
}

void ChromeTooltipTest::titleButtonsKeepFlutterNames() {
  const aoide::SessionView view;
  using Hit = aoide::TitleChromeLayout::Hit;
  QCOMPARE(aoide::chromeTooltip(Hit::minimize, {}, view), QStringLiteral("Minimize"));
  QCOMPARE(aoide::chromeTooltip(Hit::collapse, {}, view), QStringLiteral("Collapse"));
  QCOMPARE(aoide::chromeTooltip(Hit::zoomOut, {}, view), QStringLiteral("Zoom out"));
  QCOMPARE(aoide::chromeTooltip(Hit::zoomIn, {}, view), QStringLiteral("Zoom in"));
  QCOMPARE(aoide::chromeTooltip(Hit::close, {}, view), QStringLiteral("Close"));
  QCOMPARE(aoide::chromeTooltip(Hit::drag, {}, view), QString());
  QCOMPARE(aoide::chromeTooltip(Hit::none, {}, view), QString());
  QCOMPARE(aoide::chromeTooltip(Hit::drag, kind(aoide::ChromeHit::Kind::play), view), QString());
}

void ChromeTooltipTest::withdrawnZoomStepsNameWhy() {
  using Hit = aoide::TitleChromeLayout::Hit;
  aoide::SessionView view;

  // The floor of the ladder is where Aoide ends. The percent is the one the
  // readout already shows, so the tip does not invent a smaller step.
  view.zoomPercent = 50;
  view.zoomOutEnabled = false;
  QCOMPARE(aoide::chromeTooltip(Hit::zoomOut, {}, view),
           QStringLiteral("50% is as small as Aoide goes"));

  // A step that will not fit is where this display ends. Closing a panel is
  // the way back — but only while one is open to close.
  view.zoomPercent = 75;
  view.zoomOutEnabled = true;
  view.zoomInEnabled = false;
  view.eqOn = true;
  view.plOn = false;
  QCOMPARE(aoide::chromeTooltip(Hit::zoomIn, {}, view),
           QStringLiteral("No room for 100% — close a panel"));

  view.eqOn = false;
  view.plOn = false;
  QCOMPARE(aoide::chromeTooltip(Hit::zoomIn, {}, view),
           QStringLiteral("No room for 100% on this display"));
}

void ChromeTooltipTest::transportGlyphsKeepFlutterNames() {
  const aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::options), view), QStringLiteral("Options"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::prev), view), QStringLiteral("Previous"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::play), view), QStringLiteral("Play"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::pause), view), QStringLiteral("Pause"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::stop), view), QStringLiteral("Stop"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::next), view), QStringLiteral("Next"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eject), view), QStringLiteral("Open files"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::shuffle), view), QStringLiteral("Shuffle"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::repeat), view), QStringLiteral("Repeat"));
}

void ChromeTooltipTest::muteAndTogglesSayWhichWay() {
  aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::mute), view), QStringLiteral("Mute"));
  view.muted = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::mute), view), QStringLiteral("Unmute"));

  view.forceMono = false;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::mono), view),
           QStringLiteral("Fold both channels to mono"));
  view.forceMono = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::mono), view), QStringLiteral("Play in stereo"));

  view.eqOn = false;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqToggle), view), QStringLiteral("Show equalizer"));
  view.eqOn = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqToggle), view), QStringLiteral("Hide equalizer"));

  view.plOn = false;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plToggle), view),
           QStringLiteral("Show Playlist Manager"));
  view.plOn = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plToggle), view),
           QStringLiteral("Hide Playlist Manager"));
}

void ChromeTooltipTest::slidersAndListsStayQuiet() {
  aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::volume), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::seek), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::timeToggle), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqPreamp), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqBand, 0), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plCollectionRow, 0), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plTrackRow, 2), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plDivider), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plResize), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinScroll), view), QString());
}

void ChromeTooltipTest::skinsCellsStayQuiet() {
  aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  view.skins = {{QStringLiteral("builtin"), QStringLiteral("Aoide"),
                 QStringLiteral("Proxima Magnifica")},
                {QStringLiteral("arc"), QStringLiteral("Arc"), QStringLiteral("Proxima Magnifica")}};
  // Name and author paint on the preview; the hover label would cover it.
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinRow, 0), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinRow, 1), view), QString());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinRemove, 1), view),
           QStringLiteral("Remove Arc"));
}

void ChromeTooltipTest::playlistButtonsKeepFlutterNames() {
  aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plCollapse), view),
           QStringLiteral("Collapse playlist collection"));
  view.collectionCollapsed = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plCollapse), view),
           QStringLiteral("Show playlist collection"));

  QCOMPARE(aoide::chromeTooltip({}, kind(K::plAddCollection), view),
           QStringLiteral("Add playlist to collection"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plCreate), view), QStringLiteral("Create playlist"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plRename), view), QStringLiteral("Rename playlist"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plRemoveCollection), view),
           QStringLiteral("Remove playlist from collection"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plAdd), view), QStringLiteral("Add tracks"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plRemove), view),
           QStringLiteral("Remove selected tracks"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plSort), view), QStringLiteral("Sort playlist"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plOptions), view), QStringLiteral("Playlist options"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plSave), view), QStringLiteral("No changes to save"));
  view.playlistAltered = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plSave), view), QStringLiteral("Save playlist"));
  view.playlistAltered = false;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plPrev), view), QStringLiteral("Previous"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plNext), view), QStringLiteral("Next"));
  view.playing = false;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plPlay), view), QStringLiteral("Play"));
  view.playing = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plPlay), view), QStringLiteral("Pause"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::plRefresh), view), QStringLiteral("Refresh playlist"));

  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqOn), view), QStringLiteral("Equalizer on"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqAuto), view), QStringLiteral("Auto"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::eqPresets), view), QStringLiteral("Presets"));
}

void ChromeTooltipTest::saveButtonIsDeadUntilTheListIsAltered() {
  aoide::ChromeHit hit;
  hit.kind = aoide::ChromeHit::Kind::plSave;
  aoide::SessionView view;
  QVERIFY(!aoide::chromeHitEnabled(hit, view));
  view.playlistAltered = true;
  QVERIFY(aoide::chromeHitEnabled(hit, view));
}

void ChromeTooltipTest::settingsAndAboutNameControls() {
  const aoide::SessionView view;
  using K = aoide::ChromeHit::Kind;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsGeneral), view), QStringLiteral("General"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsAudio), view), QStringLiteral("Audio"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::skins), view), QStringLiteral("Show Skins"));
  aoide::SessionView skinsOpen = view;
  skinsOpen.skinsOn = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::skins), skinsOpen), QStringLiteral("Hide Skins"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::trackInfo), view), QStringLiteral("No track loaded."));
  aoide::SessionView loaded = view;
  loaded.trackInfoEnabled = true;
  QCOMPARE(aoide::chromeTooltip({}, kind(K::trackInfo), loaded), QStringLiteral("Track info"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsResume), view),
           aoide::resumePlaybackLabel());
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsConfirm), view),
           QStringLiteral("Confirm before quit"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsScroll), view), QStringLiteral("Scroll title"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsMinimize), view),
           QStringLiteral("Minimize hides secondaries"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSnapOff), view), QStringLiteral("Dock snap off"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSnapNormal), view),
           QStringLiteral("Dock snap normal"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSnapStrong), view),
           QStringLiteral("Dock snap strong"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsReset), view), QStringLiteral("Reset Settings"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsAudioDevice), view),
           QStringLiteral("Output device"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsExclusive), view),
           QStringLiteral("Exclusive output"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinAdd), view),
           QStringLiteral("Install skin"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinsFolder), view),
           QStringLiteral("Open skins folder"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::settingsSkinsRefresh), view),
           QStringLiteral("Refresh skins"));
  QCOMPARE(aoide::chromeTooltip({}, kind(K::aboutWeb), view), QStringLiteral("Open aoide.music"));
}

void ChromeTooltipTest::hoverMotionHidesWhenBusyOrEmpty() {
  QCOMPARE(aoide::tooltipMotion(QStringLiteral("Play"), QString(), false),
           aoide::TooltipMotion::hide);
  QCOMPARE(aoide::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Play"), true),
           aoide::TooltipMotion::hide);
  QCOMPARE(aoide::tooltipMotion(QString(), QString(), false), aoide::TooltipMotion::hide);
}

void ChromeTooltipTest::hoverMotionRestartsWhenTheNameChanges() {
  QCOMPARE(aoide::tooltipMotion(QString(), QStringLiteral("Play"), false, true),
           aoide::TooltipMotion::restartWait);
  QCOMPARE(aoide::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Pause"), false, true),
           aoide::TooltipMotion::restartWait);
  QCOMPARE(aoide::tooltipMotion(QStringLiteral("Play"), QStringLiteral("Play"), false, true),
           aoide::TooltipMotion::keep);
  QCOMPARE(aoide::tooltipMotion(QStringLiteral("Previous"), QStringLiteral("Previous"), false, false),
           aoide::TooltipMotion::restartWait);
}

QTEST_MAIN(ChromeTooltipTest)
#include "chrome_tooltip_test.moc"
