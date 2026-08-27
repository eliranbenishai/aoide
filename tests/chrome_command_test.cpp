#include "chrome_command.h"

#include "collection.h"
#include "docking.h"
#include "player_engine.h"
#include "playlist.h"
#include "settings.h"

#include <QTest>

/// The router is the seam: a transport command and a playlist command without
/// constructing a HostWindow or a AoideSession. Expected values are the
/// product rules in handleHit — which hits persist, which mark the list
/// altered — not a restatement of the router's internals.
class ChromeCommandTest : public QObject {
  Q_OBJECT

 private slots:
  void playStartsPlaybackAndDoesNotAskToPersist();
  void playDoesNotPauseATrackThatIsAlreadyGoing();
  void ejectAsksForFilesAndLeavesTransportAndSettingsAlone();
  void volumePressBeginsASliderAndDoesNotPersist();
  void eqBandPressRemembersWhichBand();
  void removingATrackMarksThePlaylistAlteredAndDoesNotPersistSettings();
  void collapsingTheCollectionPersistsAndAsksForARefresh();
  void togglingElapsedTimePersistsAndAsksForARefresh();
  void monoPersistsAndDoesNotMarkThePlaylistAltered();
  void skinsAddButtonAsksForTheInstallMenu();
  void skinsFolderButtonOpensTheDirectory();
  void skinsRefreshButtonAsksToRescan();
  void skinsButtonTogglesTheSkinsPanel();
  void clickingASkinPreviewAsksToActivate();
  void trashcanAsksToRemove();
  void skinsScrollbarBeginsASlider();
  void trackInfoAsksToShowWhenATrackIsLoaded();
  void trackInfoDoesNothingWhenNothingIsLoaded();
  void savingAnAlteredListAsksTheSessionToWriteIt();
  void savingAnUnalteredListDoesNothing();
  void audioDeviceAsksForTheDeviceMenuAndDoesNotPersist();
  void exclusiveOutputTogglesPersistsAndAsksForARefresh();
};

namespace {

struct Fixture {
  aoide::PlaylistController playlist;
  aoide::NullEngine engine;
  aoide::PlaybackController playback;
  aoide::AoideSettings settings;
  aoide::PlaylistCollection collection;
  aoide::DockingCoordinator docking;

  Fixture() : playback(&playlist, &engine) {
    aoide::Track track;
    track.path = QStringLiteral("/tmp/router-track.mp3");
    playlist.setTracks({track});
  }

  aoide::ChromeCommandRouter router() {
    return {playback, playlist, settings, collection, engine, docking};
  }
};

aoide::ChromeHit hit(aoide::ChromeHit::Kind kind) {
  aoide::ChromeHit out;
  out.kind = kind;
  return out;
}

}  // namespace

void ChromeCommandTest::playStartsPlaybackAndDoesNotAskToPersist() {
  Fixture f;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::main, hit(aoide::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
  QCOMPARE(out.intent, aoide::ChromeIntent::none);
}

void ChromeCommandTest::playDoesNotPauseATrackThatIsAlreadyGoing() {
  Fixture f;
  f.playback.playPause();
  QVERIFY(f.playback.playing());
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::main, hit(aoide::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
}

void ChromeCommandTest::ejectAsksForFilesAndLeavesTransportAndSettingsAlone() {
  Fixture f;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::main, hit(aoide::ChromeHit::Kind::eject), Qt::NoModifier,
                        {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::pickAudio);
  QVERIFY(!f.playback.playing());
  QVERIFY(!out.persist);
}

void ChromeCommandTest::volumePressBeginsASliderAndDoesNotPersist() {
  Fixture f;
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::main, hit(aoide::ChromeHit::Kind::volume), Qt::NoModifier, QPoint(10, 10));
  QVERIFY(out.handled);
  QVERIFY(out.beginSlider);
  QCOMPARE(out.sliderKind, aoide::ChromeHit::Kind::volume);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::eqBandPressRemembersWhichBand() {
  Fixture f;
  aoide::ChromeHit band = hit(aoide::ChromeHit::Kind::eqBand);
  band.index = 3;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::equalizer, band, Qt::NoModifier, QPoint(8, 40));
  QVERIFY(out.handled);
  QVERIFY(out.beginSlider);
  QCOMPARE(out.sliderKind, aoide::ChromeHit::Kind::eqBand);
  QCOMPARE(out.sliderIndex, 3);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::removingATrackMarksThePlaylistAlteredAndDoesNotPersistSettings() {
  Fixture f;
  QVERIFY(!f.playlist.altered());
  f.playlist.select(0);
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::playlist, hit(aoide::ChromeHit::Kind::plRemove), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playlist.tracks().isEmpty());
  QVERIFY(f.playlist.altered());
  QVERIFY(!out.persist);
  QVERIFY(!out.refreshChrome);
}

void ChromeCommandTest::collapsingTheCollectionPersistsAndAsksForARefresh() {
  Fixture f;
  QVERIFY(!f.settings.playlistCollectionCollapsed);
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::playlist, hit(aoide::ChromeHit::Kind::plCollapse), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.settings.playlistCollectionCollapsed);
  QVERIFY(out.persist);
  QVERIFY(out.refreshChrome);
  QVERIFY(!f.playlist.altered());
}

void ChromeCommandTest::togglingElapsedTimePersistsAndAsksForARefresh() {
  Fixture f;
  QVERIFY(f.settings.showElapsed);
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::main, hit(aoide::ChromeHit::Kind::timeToggle), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(!f.settings.showElapsed);
  QVERIFY(out.persist);
  QVERIFY(out.refreshChrome);
}

void ChromeCommandTest::monoPersistsAndDoesNotMarkThePlaylistAltered() {
  Fixture f;
  QVERIFY(!f.settings.forceMono);
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::main, hit(aoide::ChromeHit::Kind::mono), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.settings.forceMono);
  QVERIFY(out.persist);
  QVERIFY(out.refreshChrome);
  QVERIFY(!f.playlist.altered());
}

void ChromeCommandTest::skinsAddButtonAsksForTheInstallMenu() {
  Fixture f;
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::skins, hit(aoide::ChromeHit::Kind::settingsSkinAdd), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::showSkinInstallMenu);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::skinsFolderButtonOpensTheDirectory() {
  Fixture f;
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::skins, hit(aoide::ChromeHit::Kind::settingsSkinsFolder), Qt::NoModifier,
      {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::openSkinsDirectory);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::skinsRefreshButtonAsksToRescan() {
  Fixture f;
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::skins, hit(aoide::ChromeHit::Kind::settingsSkinsRefresh), Qt::NoModifier,
      {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::rescanSkins);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::clickingASkinPreviewAsksToActivate() {
  Fixture f;
  aoide::ChromeHit row = hit(aoide::ChromeHit::Kind::settingsSkinRow);
  row.index = 2;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::skins, row, Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::activateSkin);
  QCOMPARE(out.collectionRow, 2);
}

void ChromeCommandTest::trashcanAsksToRemove() {
  Fixture f;
  aoide::ChromeHit trash = hit(aoide::ChromeHit::Kind::settingsSkinRemove);
  trash.index = 1;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::skins, trash, Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::removeSkin);
  QCOMPARE(out.collectionRow, 1);
}

void ChromeCommandTest::skinsScrollbarBeginsASlider() {
  Fixture f;
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::skins, hit(aoide::ChromeHit::Kind::settingsSkinScroll), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(out.beginSlider);
  QCOMPARE(out.sliderKind, aoide::ChromeHit::Kind::settingsSkinScroll);
}

void ChromeCommandTest::skinsButtonTogglesTheSkinsPanel() {
  Fixture f;
  const aoide::ChromeCommandOutcome out =
      f.router().handle(aoide::WindowId::main, hit(aoide::ChromeHit::Kind::skins), Qt::NoModifier,
                        {});
  QVERIFY(out.handled);
  QCOMPARE(out.toggleVisible, aoide::WindowId::skins);
}

void ChromeCommandTest::trackInfoAsksToShowWhenATrackIsLoaded() {
  Fixture f;
  f.playback.playPause();
  QVERIFY(f.playback.currentTrack());
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::main, hit(aoide::ChromeHit::Kind::trackInfo), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::showTrackInfo);
}

void ChromeCommandTest::trackInfoDoesNothingWhenNothingIsLoaded() {
  Fixture f;
  QVERIFY(!f.playback.currentTrack());
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::main, hit(aoide::ChromeHit::Kind::trackInfo), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::none);
}

void ChromeCommandTest::savingAnAlteredListAsksTheSessionToWriteIt() {
  Fixture f;
  aoide::Track extra;
  extra.path = QStringLiteral("/tmp/router-extra.mp3");
  f.playlist.addTracks({extra});
  QVERIFY(f.playlist.altered());
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::playlist, hit(aoide::ChromeHit::Kind::plSave), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::saveCurrentPlaylist);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::savingAnUnalteredListDoesNothing() {
  Fixture f;
  QVERIFY(!f.playlist.altered());
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::playlist, hit(aoide::ChromeHit::Kind::plSave), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::none);
}

void ChromeCommandTest::audioDeviceAsksForTheDeviceMenuAndDoesNotPersist() {
  Fixture f;
  QVERIFY(!f.settings.audioExclusive);
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::settings, hit(aoide::ChromeHit::Kind::settingsAudioDevice), Qt::NoModifier,
      {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, aoide::ChromeIntent::showAudioDevices);
  QVERIFY(!out.persist);
  QVERIFY(!out.refreshChrome);
  QCOMPARE(f.settings.audioDevice, QString());
}

void ChromeCommandTest::exclusiveOutputTogglesPersistsAndAsksForARefresh() {
  Fixture f;
  QVERIFY(!f.settings.audioExclusive);
  const aoide::ChromeCommandOutcome out = f.router().handle(
      aoide::WindowId::settings, hit(aoide::ChromeHit::Kind::settingsExclusive), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.settings.audioExclusive);
  QVERIFY(out.persist);
  QVERIFY(out.refreshChrome);
  QCOMPARE(out.intent, aoide::ChromeIntent::none);
}

QTEST_APPLESS_MAIN(ChromeCommandTest)
#include "chrome_command_test.moc"
