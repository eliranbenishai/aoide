#include "chrome_command.h"

#include "player_engine.h"
#include "playlist.h"

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
};

namespace {

struct TransportFixture {
  tramp::PlaylistController playlist;
  tramp::NullEngine engine;
  tramp::PlaybackController playback;

  TransportFixture() : playback(&playlist, &engine) {
    tramp::Track track;
    track.path = QStringLiteral("/tmp/router-track.mp3");
    playlist.setTracks({track});
  }
};

tramp::ChromeHit hit(tramp::ChromeHit::Kind kind) {
  tramp::ChromeHit out;
  out.kind = kind;
  return out;
}

}  // namespace

void ChromeCommandTest::playStartsPlaybackAndDoesNotAskToPersist() {
  TransportFixture f;
  tramp::ChromeCommandRouter router(f.playback);
  const tramp::ChromeCommandOutcome out =
      router.handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
  QCOMPARE(out.intent, tramp::ChromeIntent::none);
}

void ChromeCommandTest::playDoesNotPauseATrackThatIsAlreadyGoing() {
  TransportFixture f;
  f.playback.playPause();
  QVERIFY(f.playback.playing());
  tramp::ChromeCommandRouter router(f.playback);
  const tramp::ChromeCommandOutcome out =
      router.handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::play), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QVERIFY(f.playback.playing());
  QVERIFY(!out.persist);
}

void ChromeCommandTest::ejectAsksForFilesAndLeavesTransportAndSettingsAlone() {
  TransportFixture f;
  tramp::ChromeCommandRouter router(f.playback);
  const tramp::ChromeCommandOutcome out =
      router.handle(tramp::WindowId::main, hit(tramp::ChromeHit::Kind::eject), Qt::NoModifier, {});
  QVERIFY(out.handled);
  QCOMPARE(out.intent, tramp::ChromeIntent::pickAudio);
  QVERIFY(!f.playback.playing());
  QVERIFY(!out.persist);
}

QTEST_APPLESS_MAIN(ChromeCommandTest)
#include "chrome_command_test.moc"
