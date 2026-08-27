#pragma once

#include <QFileInfo>
#include <QString>
#include <QVector>
#include <optional>

namespace aoide {

enum class RepeatMode { off, all, one };

struct Track {
  QString path;
  QString title;
  QString artist;
  QString album;
  std::optional<int> year;
  std::optional<qint64> durationMs;
  bool disabled = false;

  QString displayTitle() const {
    const QString trimmed = title.trimmed();
    if (!trimmed.isEmpty()) {
      return trimmed;
    }
    return QFileInfo(path).fileName();
  }
};

inline bool operator==(const Track& a, const Track& b) {
  return a.path == b.path && a.title == b.title && a.artist == b.artist && a.album == b.album &&
         a.year == b.year && a.durationMs == b.durationMs && a.disabled == b.disabled;
}

inline bool operator!=(const Track& a, const Track& b) { return !(a == b); }

inline QString formatClock(qint64 ms) {
  if (ms < 0) {
    ms = 0;
  }
  const qint64 totalSec = ms / 1000;
  const qint64 minutes = totalSec / 60;
  const qint64 seconds = totalSec % 60;
  return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

inline QString groupedInt(int value) {
  const QString digits = QString::number(std::abs(value));
  QString out = value < 0 ? QStringLiteral("-") : QString();
  for (int i = 0; i < digits.size(); ++i) {
    if (i > 0 && (digits.size() - i) % 3 == 0) {
      out += QLatin1Char(',');
    }
    out += digits[i];
  }
  return out;
}

inline int playableTrackCount(const QVector<Track>& tracks) {
  int n = 0;
  for (const Track& t : tracks) {
    if (!t.disabled) ++n;
  }
  return n;
}

inline qint64 playableTotalMs(const QVector<Track>& tracks) {
  qint64 total = 0;
  for (const Track& t : tracks) {
    if (t.disabled) continue;
    if (t.durationMs) total += *t.durationMs;
  }
  return total;
}

inline QString formatTotalTime(qint64 ms) {
  if (ms < 0) {
    ms = 0;
  }
  const qint64 days = ms / (24LL * 3600 * 1000);
  const qint64 hoursTotal = ms / (3600 * 1000);
  const qint64 minutesTotal = ms / (60 * 1000);
  if (days > 0) {
    return QStringLiteral("%1 d %2 h").arg(days).arg(hoursTotal - days * 24);
  }
  if (hoursTotal > 0) {
    return QStringLiteral("%1 h %2 m").arg(hoursTotal).arg(minutesTotal - hoursTotal * 60);
  }
  return QStringLiteral("%1 m").arg(minutesTotal);
}

}  // namespace aoide
