#pragma once

#include <QFileInfo>
#include <QMap>
#include <QString>
#include <QVector>
#include <optional>

namespace aoide {

enum class RepeatMode { off, all, one };

struct TrackMetadata {
  QString title;
  QString artist;
  QString album;
  std::optional<qint64> durationMs = {};
  std::optional<int> year = {};
  QString albumArtist = {};
  QString genre = {};
  QString trackNumber = {};
  QString discNumber = {};
  QString composer = {};

  bool hasTags() const {
    return !title.isEmpty() || !artist.isEmpty() || !album.isEmpty() || year ||
           !albumArtist.isEmpty() || !genre.isEmpty() || !trackNumber.isEmpty() ||
           !discNumber.isEmpty() || !composer.isEmpty();
  }
};

struct Track {
  QString path;
  QString title;
  QString artist;
  QString album;
  std::optional<int> year = {};
  std::optional<qint64> durationMs = {};
  bool disabled = false;
  QString albumArtist = {};
  QString genre = {};
  QString trackNumber = {};
  QString discNumber = {};
  QString composer = {};

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
         a.year == b.year && a.durationMs == b.durationMs && a.disabled == b.disabled &&
         a.albumArtist == b.albumArtist && a.genre == b.genre &&
         a.trackNumber == b.trackNumber && a.discNumber == b.discNumber && a.composer == b.composer;
}

inline bool operator!=(const Track& a, const Track& b) { return !(a == b); }

inline TrackMetadata trackMetadata(const Track& t) {
  return {t.title, t.artist, t.album, t.durationMs, t.year, t.albumArtist, t.genre,
          t.trackNumber, t.discNumber, t.composer};
}

/// Empty tags never erase known values; Refresh and playback prefer the file's
/// nonempty tags, while background ingest only fills gaps.
inline void applyTrackMetadata(Track& t, const TrackMetadata& metadata, bool overwrite) {
  auto take = [&](const QString& src, QString& dest) {
    const QString trimmed = src.trimmed();
    if (trimmed.isEmpty() || (!overwrite && !dest.trimmed().isEmpty())) return;
    dest = trimmed;
  };
  take(metadata.title, t.title);
  take(metadata.artist, t.artist);
  take(metadata.album, t.album);
  take(metadata.albumArtist, t.albumArtist);
  take(metadata.genre, t.genre);
  take(metadata.trackNumber, t.trackNumber);
  take(metadata.discNumber, t.discNumber);
  take(metadata.composer, t.composer);
  if (metadata.year && *metadata.year > 0 && (overwrite || !t.year)) t.year = metadata.year;
  if (metadata.durationMs && *metadata.durationMs > 0 &&
      (overwrite || !t.durationMs || *t.durationMs <= 0)) t.durationMs = metadata.durationMs;
}

/// Audio containers use different spellings for the same tags. Preserve track
/// and disc fractions ("3/12") and read the year from either a year or date tag.
inline TrackMetadata trackMetadataFromTags(const QMap<QString, QString>& tags) {
  QMap<QString, QString> normalized;
  for (auto it = tags.cbegin(); it != tags.cend(); ++it) {
    QString key = it.key().toLower();
    key.remove(QLatin1Char(' '));
    key.remove(QLatin1Char('_'));
    key.remove(QLatin1Char('-'));
    const QString value = it.value().trimmed();
    if (!value.isEmpty()) normalized.insert(key, value);
  }
  auto get = [&](const char* key, const char* alternate = nullptr) {
    const QString value = normalized.value(QLatin1String(key));
    return value.isEmpty() && alternate ? normalized.value(QLatin1String(alternate)) : value;
  };
  TrackMetadata out;
  out.title = get("title");
  out.artist = get("artist");
  out.album = get("album");
  out.albumArtist = get("albumartist");
  out.genre = get("genre");
  out.trackNumber = get("tracknumber", "track");
  out.discNumber = get("discnumber", "disc");
  out.composer = get("composer");
  const auto addTotal = [&](QString& number, const QString& total) {
    if (!number.isEmpty() && !number.contains(QLatin1Char('/')) && !total.isEmpty())
      number += QLatin1Char('/') + total;
  };
  addTotal(out.trackNumber, get("tracktotal", "totaltracks"));
  addTotal(out.discNumber, get("disctotal", "totaldiscs"));
  for (const char* key : {"year", "date"}) {
    const QString date = get(key);
    if (date.size() < 4 || (date.size() > 4 && date[4].isDigit())) continue;
    bool ok = false;
    const int year = date.left(4).toInt(&ok);
    if (ok && year > 0) {
      out.year = year;
      break;
    }
  }
  return out;
}

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
