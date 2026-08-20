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

inline QStringList pathsNeedingAudioProbe(const QVector<Track>& tracks) {
  QStringList paths;
  for (const Track& t : tracks) {
    if (trackNeedsAudioProbe(t)) paths.push_back(t.path);
  }
  return paths;
}

std::optional<qint64> probeWavDurationMs(const QByteArray& bytes);
std::optional<qint64> probeAudioDurationMs(const QString& path);

/// Probe each path; [stillWanted] is checked between files so a newer load can cancel.
void probeAudioDurations(const QStringList& paths, const std::function<bool()>& stillWanted,
                         const std::function<void(const QString&, const ProbedAudio&)>& onProbed);

}  // namespace tramp
