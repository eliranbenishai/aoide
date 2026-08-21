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

/// Duration of a WAV from its header, never longer than its bytes allow.
/// [fileBytes] is the length of the whole file when [bytes] is only the head of
/// it; leave it at -1 when [bytes] is everything there is.
std::optional<qint64> probeWavDurationMs(const QByteArray& bytes, qint64 fileBytes = -1);
std::optional<qint64> probeAudioDurationMs(const QString& path);

/// Probe each path; [stillWanted] is checked between files so a newer load can cancel.
void probeAudioDurations(const QStringList& paths, const std::function<bool()>& stillWanted,
                         const std::function<void(const QString&, const ProbedAudio&)>& onProbed);

}  // namespace tramp
