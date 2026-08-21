/// Quit under load: start a session with a large playlist, set the background
/// workers running, then destroy it while they are still in flight — over and
/// over, with the teardown moment swept across the window the workers live in.
///
/// The point is a red signal. A worker that outlives its session marshals into
/// freed memory, and the gap that lets it happen is a few instructions wide, so
/// one launch proves nothing either way. Driven by `tool/quit-under-load.sh`,
/// which builds this under AddressSanitizer and seeds the playlist.

#include "session.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QFileInfoList>
#include <QStringList>
#include <QTimer>
#include <QtGlobal>
#include <clocale>
#include <cstdio>
#include <memory>

namespace {

int envInt(const char* key, int fallback) {
  bool ok = false;
  const int value = qEnvironmentVariableIntValue(key, &ok);
  return ok && value > 0 ? value : fallback;
}

QStringList filesIn(const QString& dir) {
  if (dir.isEmpty()) return {};
  QStringList paths;
  const QFileInfoList entries = QDir(dir).entryInfoList(QDir::Files, QDir::Name);
  for (const QFileInfo& info : entries) paths.push_back(info.absoluteFilePath());
  return paths;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  std::setlocale(LC_NUMERIC, "C");  // libmpv refuses to work under any other

  const int rounds = envInt("TRAMP_QUIT_ROUNDS", 12);
  const int fixedLiveMs = envInt("TRAMP_QUIT_LIVE_MS", 0);
  const int sweepMs = envInt("TRAMP_QUIT_SWEEP_MS", 400);
  const QStringList drops = filesIn(qEnvironmentVariable("TRAMP_QUIT_DROP_DIR"));

  qint64 worstTeardownMs = 0;
  for (int round = 0; round < rounds; ++round) {
    const int liveMs = fixedLiveMs > 0 ? fixedLiveMs : (round * 37) % sweepMs;

    auto session = std::make_unique<tramp::TrampSession>();
    session->bootstrap({});   // restores the seeded playlist, starts the path verify
    session->playTrackAt(0);  // opens the track, which starts the spectrum decode
    if (!drops.isEmpty()) {
      session->applyDroppedPaths(drops, false);  // starts the async duration probe
    }

    QEventLoop live;
    QTimer::singleShot(liveMs, &live, &QEventLoop::quit);
    live.exec();

    QElapsedTimer teardown;
    teardown.start();
    session.reset();
    const qint64 tookMs = teardown.elapsed();
    worstTeardownMs = qMax(worstTeardownMs, tookMs);

    // Anything a worker managed to post at the session lands here.
    QCoreApplication::processEvents();

    std::fprintf(stdout, "round %3d  live %4d ms  teardown %5lld ms\n", round, liveMs,
                 static_cast<long long>(tookMs));
    std::fflush(stdout);
  }

  std::fprintf(stdout, "quit-loop: %d rounds survived, worst teardown %lld ms\n", rounds,
               static_cast<long long>(worstTeardownMs));
  return 0;
}
