#pragma once

#include "playlist.h"
#include "settings.h"
#include "track.h"

#include <QMap>
#include <QString>
#include <QVector>
#include <optional>

namespace tramp {

struct UsageCounters {
  int spins = 0;
};

struct SessionResume {
  std::optional<int> playingIndex;
  qint64 positionMs = 0;
  bool wasPlaying = false;
};

struct AlteredPlaylist {
  QVector<Track> tracks;
  QString sourcePath;
  bool isEmpty() const { return tracks.isEmpty() && sourcePath.isEmpty(); }
};

struct SavedPlaylist {
  QString path;
  QString name;
  int trackCount = 0;
  qint64 totalDurationMs = 0;
  qint64 modifiedMs = 0;
  QString displayName() const;
};

struct CollectionTrackSets {
  QMap<QString, QStringList> byEntry;
  QMap<QString, qint64> durationsMs;
};

struct CollectionFigures {
  int playlists = 0;
  int tracks = 0;
  qint64 totalDurationMs = 0;
};

class SupportStore {
 public:
  explicit SupportStore(QString dir);

  QString dir() const { return dir_; }

  TrampSettings readSettings() const;
  void writeSettings(const TrampSettings& s) const;

  UsageCounters readUsage() const;
  void writeUsage(const UsageCounters& u) const;

  SessionResume readResume() const;
  void writeResume(const SessionResume& r) const;

  AlteredPlaylist readAltered() const;
  void writeAltered(const AlteredPlaylist& p) const;
  void clearAltered() const;

  QString readLastPlaylistPath() const;
  void writeLastPlaylistPath(const QString& path) const;

  QVector<SavedPlaylist> readCollectionIndex() const;
  void writeCollectionIndex(const QVector<SavedPlaylist>& entries) const;
  CollectionTrackSets readTrackSets() const;
  void writeTrackSets(const CollectionTrackSets& sets) const;

 private:
  QString filePath(const QString& name) const;
  QJsonObject readObject(const QString& name) const;
  void writeObject(const QString& name, const QJsonObject& o) const;

  QString dir_;
};

QString normalizePlaylistPath(const QString& path);

inline bool samePlaylistFile(const QString& a, const QString& b) {
  if (a.isEmpty() || b.isEmpty()) return false;
  return normalizePlaylistPath(a) == normalizePlaylistPath(b);
}

}  // namespace tramp
