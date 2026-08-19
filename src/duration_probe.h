#pragma once

#include <QString>
#include <QStringList>
#include <functional>
#include <optional>

namespace tramp {

std::optional<qint64> probeWavDurationMs(const QByteArray& bytes);
std::optional<qint64> probeAudioDurationMs(const QString& path);

/// Probe each path; [stillWanted] is checked between files so a newer load can cancel.
void probeAudioDurations(const QStringList& paths, const std::function<bool()>& stillWanted,
                         const std::function<void(const QString&, qint64)>& onDuration);

}  // namespace tramp
