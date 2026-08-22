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

struct CachedTrackMeta {
  QString title;
  QString artist;
  QString album;
};

struct CollectionTrackSets {
  QMap<QString, QStringList> byEntry;
  QMap<QString, qint64> durationsMs;
  QMap<QString, CachedTrackMeta> meta;
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

  // Writes report whether the file actually landed. A failure leaves the
  // previous contents intact rather than truncating them.
  TrampSettings readSettings() const;
  bool writeSettings(const TrampSettings& s) const;

  UsageCounters readUsage() const;
  bool writeUsage(const UsageCounters& u) const;

  SessionResume readResume() const;
  bool writeResume(const SessionResume& r) const;

  AlteredPlaylist readAltered() const;
  bool writeAltered(const AlteredPlaylist& p) const;
  void clearAltered() const;

  QString readLastPlaylistPath() const;
  bool writeLastPlaylistPath(const QString& path) const;

  QVector<SavedPlaylist> readCollectionIndex() const;
  bool writeCollectionIndex(const QVector<SavedPlaylist>& entries) const;
  CollectionTrackSets readTrackSets() const;
  bool writeTrackSets(const CollectionTrackSets& sets) const;

 private:
  QString filePath(const QString& name) const;
  QJsonObject readObject(const QString& name) const;
  bool writeObject(const QString& name, const QJsonObject& o) const;

  QString dir_;
};

/// Last write of each session state file. A failure stays until that file lands.
struct PersistHealth {
  bool settingsOk = true;
  bool resumeOk = true;
  bool usageOk = true;
  bool alteredOk = true;
  bool lastPlaylistOk = true;

  bool anyFailed() const {
    return !settingsOk || !resumeOk || !usageOk || !alteredOk || !lastPlaylistOk;
  }
};

/// The write path `persistNow` uses. Each attempted file updates only its own
/// slot, so a later success on that file is what clears the mark.
void writeSessionPersist(const SupportStore& store, PersistHealth& health,
                         const TrampSettings& settings, const SessionResume& resume,
                         const UsageCounters& usage, const QString& lastPlaylistPath,
                         const AlteredPlaylist* altered);

QString normalizePlaylistPath(const QString& path);

inline bool samePlaylistFile(const QString& a, const QString& b) {
  if (a.isEmpty() || b.isEmpty()) return false;
  return normalizePlaylistPath(a) == normalizePlaylistPath(b);
}

}  // namespace tramp
