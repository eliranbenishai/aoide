#pragma once

#include "track.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>
#include <optional>

namespace tramp {

struct ProbedAudio {
  QString title;
  QString artist;
  QString album;
  std::optional<qint64> durationMs;
};

inline bool trackNeedsAudioProbe(const Track& t) {
  if (!t.durationMs || *t.durationMs <= 0) return true;
  return t.title.trimmed().isEmpty();
}

inline void applyProbedAudio(Track& t, const ProbedAudio& probed, bool overwrite) {
  auto take = [&](const QString& src, QString& dest) {
    const QString trimmed = src.trimmed();
    if (trimmed.isEmpty()) return;
    if (!overwrite && !dest.trimmed().isEmpty()) return;
    dest = trimmed;
  };
  take(probed.title, t.title);
  take(probed.artist, t.artist);
  take(probed.album, t.album);
  if (probed.durationMs && *probed.durationMs > 0) {
    if (overwrite || !t.durationMs || *t.durationMs <= 0) t.durationMs = probed.durationMs;
  }
}

inline QStringList pathsNeedingAudioProbe(const QVector<Track>& tracks) {
  QStringList paths;
  for (const Track& t : tracks) {
    if (trackNeedsAudioProbe(t)) paths.push_back(t.path);
  }
  return paths;
}

/// Answered on the probing thread. False means stop now, not at the next file.
/// An empty function means the run cannot be cancelled.
using ProbeCancelFn = std::function<bool()>;

/// How one file is probed. The default reads a WAV header and otherwise makes a
/// throwaway libmpv pass. Tests swap in a loader that takes as long as a network
/// mount does, which is the only way to hold a probe open on purpose.
using ProbeOneFn =
    std::function<std::optional<ProbedAudio>(const QString& path, const ProbeCancelFn& stillWanted)>;

/// Duration of a WAV from its header, never longer than its bytes allow.
/// [fileBytes] is the length of the whole file when [bytes] is only the head of
/// it; leave it at -1 when [bytes] is everything there is.
std::optional<qint64> probeWavDurationMs(const QByteArray& bytes, qint64 fileBytes = -1);
std::optional<qint64> probeAudioDurationMs(const QString& path);

/// Probe each path, reporting each answer as it arrives so a list can fill in
/// while the rest is still being asked about.
///
/// [stillWanted] is asked before every file **and** on every tick of the libmpv
/// wait, because a file mpv accepts but never reports loaded owns the whole
/// twenty-second budget on its own — and whoever is waiting for this thread to
/// finish waits out every second of it.
void probeAudioDurations(const QStringList& paths, const ProbeCancelFn& stillWanted,
                         const std::function<void(const QString&, const ProbedAudio&)>& onProbed,
                         const ProbeOneFn& probeOne = {});

}  // namespace tramp
