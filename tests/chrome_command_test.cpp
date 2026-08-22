#include "chrome_command.h"

#include "collection.h"
#include "player_engine.h"
#include "playlist.h"
#include "settings.h"

#include <QTest>

/// The router is the seam: a transport command and a playlist command without
/// constructing a HostWindow or a TrampSession. Expected values are the
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
};

namespace {

struct Fixture {
  tramp::PlaylistController playlist;
  tramp::NullEngine engine;
  tramp::PlaybackController playback;
  tramp::TrampSettings settings;
  tramp::PlaylistCollection collection;

  Fixture() : playback(&playlist, &engine) {
    tramp::Track track;
    track.path = QStringLiteral("/tmp/router-track.mp3");
    playlist.setTracks({track});
  }

  tramp::ChromeCommandRouter router() { return {playback, playlist, settings, collection}; }
};

tramp::ChromeHit hit(tramp::ChromeHit::Kind kind) {
  tramp::ChromeHit out;
  out.kind = kind;
  return out;
}

}  // namespace

void ChromeCommandTest::playStartsPlaybackAndDoesNotAskToPersist() {
  Fixture f;
  const tramp::ChromeCommandOutcome out =
      f.router().handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
  QCOMPARE(out.intent, tramp::ChromeIntent::none);
}

void ChromeCommandTest::playDoesNotPauseATrackThatIsAlreadyGoing() {
  Fixture f;
  f.playback.playPause();
  QVERIFY(f.playback.playing());
  const tramp::ChromeCommandOutcome out =
      f.router().handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
}

void ChromeCommandTest::ejectAsksForFilesAndLeavesTransportAndSettingsAlone() {
  Fixture f;
  const tramp::ChromeCommandOutcome out =
      f.router().handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::eject), Qt::NoModifier,
                        {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, tramp::ChromeIntent::pickAudio);
  QVERIFY(!f.playback.playing());
  QVERIFY(!out.persist);
}

void ChromeCommandTest::volumePressBeginsASliderAndDoesNotPersist() {
  Fixture f;
  const tramp::ChromeCommandOutcome out = f.router().handle(
      tramp::WindowId::main, hit(tramp::ChromeHit::Kind::volume), Qt::NoModifier, QPoint(10, 10));
  QVERIFY(out.handled);
  QVERIFY(out.beginSlider);
  QCOMPARE(out.sliderKind, tramp::ChromeHit::Kind::volume);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::eqBandPressRemembersWhichBand() {
  Fixture f;
  tramp::ChromeHit band = hit(tramp::ChromeHit::Kind::eqBand);
  band.index = 3;
  const tramp::ChromeCommandOutcome out =
      f.router().handle(tramp::WindowId::equalizer, band, Qt::NoModifier, QPoint(8, 40));
  QVERIFY(out.handled);
  QVERIFY(out.beginSlider);
  QCOMPARE(out.sliderKind, tramp::ChromeHit::Kind::eqBand);
  QCOMPARE(out.sliderIndex, 3);
  QVERIFY(!out.persist);
}

void ChromeCommandTest::removingATrackMarksThePlaylistAlteredAndDoesNotPersistSettings() {
  Fixture f;
  QVERIFY(!f.playlist.altered());
  f.playlist.select(0);
  const tramp::ChromeCommandOutcome out = f.router().handle(
      tramp::WindowId::playlist, hit(tramp::ChromeHit::Kind::plRemove), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playlist.tracks().isEmpty());
  QVERIFY(f.playlist.altered());
  QVERIFY(!out.persist);
  QVERIFY(!out.refreshChrome);
}

void ChromeCommandTest::collapsingTheCollectionPersistsAndAsksForARefresh() {
  Fixture f;
  QVERIFY(!f.settings.playlistCollectionCollapsed);
  const tramp::ChromeCommandOutcome out = f.router().handle(
      tramp::WindowId::playlist, hit(tramp::ChromeHit::Kind::plCollapse), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.settings.playlistCollectionCollapsed);
  QVERIFY(out.persist);
  QVERIFY(out.refreshChrome);
  QVERIFY(!f.playlist.altered());
}

QTEST_APPLESS_MAIN(ChromeCommandTest)
#include "chrome_command_test.moc"
