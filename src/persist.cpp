#include "persist.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace tramp {
namespace {

QJsonObject trackToJson(const Track& t) {
  QJsonObject o;
  o.insert(QStringLiteral("path"), t.path);
  if (!t.title.isEmpty()) o.insert(QStringLiteral("title"), t.title);
  if (!t.artist.isEmpty()) o.insert(QStringLiteral("artist"), t.artist);
  if (!t.album.isEmpty()) o.insert(QStringLiteral("album"), t.album);
  if (t.year) o.insert(QStringLiteral("year"), *t.year);
  if (t.durationMs) o.insert(QStringLiteral("durationMs"), qint64(*t.durationMs));
  return o;
}

Track trackFromJson(const QJsonObject& o) {
  Track t;
  t.path = o.value(QStringLiteral("path")).toString();
  t.title = o.value(QStringLiteral("title")).toString();
  t.artist = o.value(QStringLiteral("artist")).toString();
  t.album = o.value(QStringLiteral("album")).toString();
  if (o.contains(QStringLiteral("year"))) t.year = o.value(QStringLiteral("year")).toInt();
  if (o.contains(QStringLiteral("durationMs"))) {
    t.durationMs = qint64(o.value(QStringLiteral("durationMs")).toDouble());
  }
  return t;
}

}  // namespace

QString SavedPlaylist::displayName() const {
  if (!name.trimmed().isEmpty()) {
    return name.trimmed();
  }
  return QFileInfo(path).completeBaseName();
}

QString normalizePlaylistPath(const QString& path) {
  const QFileInfo info(path);
  const QString abs = info.isAbsolute() ? info.absoluteFilePath()
                                        : QDir::current().absoluteFilePath(path);
  return QDir::cleanPath(abs);
}

SupportStore::SupportStore(QString dir) : dir_(std::move(dir)) {
  QDir().mkpath(dir_);
}

QString SupportStore::filePath(const QString& name) const {
  return QDir(dir_).filePath(name);
}

QJsonObject SupportStore::readObject(const QString& name) const {
  QFile file(filePath(name));
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  const auto doc = QJsonDocument::fromJson(file.readAll());
  return doc.isObject() ? doc.object() : QJsonObject{};
}

void SupportStore::writeObject(const QString& name, const QJsonObject& o) const {
  QFile file(filePath(name));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return;
  }
  file.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

TrampSettings SupportStore::readSettings() const {
  const QJsonObject o = readObject(QStringLiteral("settings.json"));
  if (o.isEmpty()) return {};
  return TrampSettings::fromJson(o);
}

void SupportStore::writeSettings(const TrampSettings& s) const {
  writeObject(QStringLiteral("settings.json"), s.toJson());
}

UsageCounters SupportStore::readUsage() const {
  const QJsonObject o = readObject(QStringLiteral("usage.json"));
  UsageCounters u;
  const int spins = o.value(QStringLiteral("spins")).toInt();
  u.spins = spins > 0 ? spins : 0;
  return u;
}

void SupportStore::writeUsage(const UsageCounters& u) const {
  QJsonObject o;
  o.insert(QStringLiteral("spins"), u.spins);
  writeObject(QStringLiteral("usage.json"), o);
}

SessionResume SupportStore::readResume() const {
  const QJsonObject o = readObject(QStringLiteral("session_resume.json"));
  SessionResume r;
  if (o.contains(QStringLiteral("playingIndex")) && !o.value(QStringLiteral("playingIndex")).isNull()) {
    r.playingIndex = o.value(QStringLiteral("playingIndex")).toInt();
  }
  r.positionMs = qint64(o.value(QStringLiteral("positionMs")).toDouble());
  r.wasPlaying = o.value(QStringLiteral("wasPlaying")).toBool();
  return r;
}

void SupportStore::writeResume(const SessionResume& r) const {
  QJsonObject o;
  if (r.playingIndex) {
    o.insert(QStringLiteral("playingIndex"), *r.playingIndex);
  } else {
    o.insert(QStringLiteral("playingIndex"), QJsonValue::Null);
  }
  o.insert(QStringLiteral("positionMs"), r.positionMs);
  o.insert(QStringLiteral("wasPlaying"), r.wasPlaying);
  writeObject(QStringLiteral("session_resume.json"), o);
}

AlteredPlaylist SupportStore::readAltered() const {
  const QJsonObject o = readObject(QStringLiteral("altered_playlist.json"));
  AlteredPlaylist p;
  p.sourcePath = o.value(QStringLiteral("sourcePath")).toString();
  const QJsonArray tracks = o.value(QStringLiteral("tracks")).toArray();
  for (const QJsonValue& v : tracks) {
    if (!v.isObject()) continue;
    Track t = trackFromJson(v.toObject());
    if (t.path.isEmpty()) continue;
    p.tracks.push_back(t);
  }
  return p;
}

void SupportStore::writeAltered(const AlteredPlaylist& p) const {
  QJsonObject o;
  if (!p.sourcePath.isEmpty()) {
    o.insert(QStringLiteral("sourcePath"), p.sourcePath);
  } else {
    o.insert(QStringLiteral("sourcePath"), QJsonValue::Null);
  }
  QJsonArray tracks;
  for (const Track& t : p.tracks) {
    tracks.append(trackToJson(t));
  }
  o.insert(QStringLiteral("tracks"), tracks);
  writeObject(QStringLiteral("altered_playlist.json"), o);
}

void SupportStore::clearAltered() const {
  QFile::remove(filePath(QStringLiteral("altered_playlist.json")));
}

QString SupportStore::readLastPlaylistPath() const {
  return readObject(QStringLiteral("session.json")).value(QStringLiteral("lastPlaylistPath")).toString();
}

void SupportStore::writeLastPlaylistPath(const QString& path) const {
  QJsonObject o;
  if (path.isEmpty()) {
    o.insert(QStringLiteral("lastPlaylistPath"), QJsonValue::Null);
  } else {
    o.insert(QStringLiteral("lastPlaylistPath"), path);
  }
  writeObject(QStringLiteral("session.json"), o);
}

QVector<SavedPlaylist> SupportStore::readCollectionIndex() const {
  QVector<SavedPlaylist> out;
  const QJsonArray raw =
      readObject(QStringLiteral("playlists.json")).value(QStringLiteral("entries")).toArray();
  for (const QJsonValue& v : raw) {
    if (!v.isObject()) continue;
    const QJsonObject o = v.toObject();
    SavedPlaylist e;
    e.path = o.value(QStringLiteral("path")).toString();
    if (e.path.isEmpty()) continue;
    e.path = normalizePlaylistPath(e.path);
    e.name = o.value(QStringLiteral("name")).toString();
    e.trackCount = o.value(QStringLiteral("trackCount")).toInt();
    e.totalDurationMs = qint64(o.value(QStringLiteral("totalDurationMs")).toDouble());
    e.modifiedMs = qint64(o.value(QStringLiteral("modifiedMs")).toDouble());
    out.push_back(e);
  }
  return out;
}

void SupportStore::writeCollectionIndex(const QVector<SavedPlaylist>& entries) const {
  QJsonArray raw;
  for (const SavedPlaylist& e : entries) {
    QJsonObject o;
    o.insert(QStringLiteral("path"), e.path);
    if (!e.name.isEmpty()) o.insert(QStringLiteral("name"), e.name);
    o.insert(QStringLiteral("trackCount"), e.trackCount);
    o.insert(QStringLiteral("totalDurationMs"), e.totalDurationMs);
    if (e.modifiedMs != 0) o.insert(QStringLiteral("modifiedMs"), e.modifiedMs);
    raw.append(o);
  }
  QJsonObject root;
  root.insert(QStringLiteral("entries"), raw);
  writeObject(QStringLiteral("playlists.json"), root);
}

CollectionTrackSets SupportStore::readTrackSets() const {
  CollectionTrackSets sets;
  const QJsonObject o = readObject(QStringLiteral("playlist_tracks.json"));
  const QJsonObject trackSets = o.value(QStringLiteral("trackSets")).toObject();
  for (auto it = trackSets.begin(); it != trackSets.end(); ++it) {
    QStringList paths;
    for (const QJsonValue& v : it.value().toArray()) {
      paths.push_back(v.toString());
    }
    sets.byEntry.insert(it.key(), paths);
  }
  const QJsonObject durations = o.value(QStringLiteral("durationsMs")).toObject();
  for (auto it = durations.begin(); it != durations.end(); ++it) {
    sets.durationsMs.insert(it.key(), qint64(it.value().toDouble()));
  }
  const QJsonObject meta = o.value(QStringLiteral("meta")).toObject();
  for (auto it = meta.begin(); it != meta.end(); ++it) {
    const QJsonObject m = it.value().toObject();
    CachedTrackMeta tags;
    tags.title = m.value(QStringLiteral("title")).toString();
    tags.artist = m.value(QStringLiteral("artist")).toString();
    tags.album = m.value(QStringLiteral("album")).toString();
    if (!tags.title.isEmpty() || !tags.artist.isEmpty() || !tags.album.isEmpty()) {
      sets.meta.insert(it.key(), tags);
    }
  }
  return sets;
}

void SupportStore::writeTrackSets(const CollectionTrackSets& sets) const {
  QJsonObject trackSets;
  for (auto it = sets.byEntry.begin(); it != sets.byEntry.end(); ++it) {
    QJsonArray arr;
    for (const QString& p : it.value()) {
      arr.append(p);
    }
    trackSets.insert(it.key(), arr);
  }
  QJsonObject durations;
  for (auto it = sets.durationsMs.begin(); it != sets.durationsMs.end(); ++it) {
    durations.insert(it.key(), it.value());
  }
  QJsonObject meta;
  for (auto it = sets.meta.begin(); it != sets.meta.end(); ++it) {
    QJsonObject m;
    if (!it.value().title.isEmpty()) m.insert(QStringLiteral("title"), it.value().title);
    if (!it.value().artist.isEmpty()) m.insert(QStringLiteral("artist"), it.value().artist);
    if (!it.value().album.isEmpty()) m.insert(QStringLiteral("album"), it.value().album);
    if (!m.isEmpty()) meta.insert(it.key(), m);
  }
  QJsonObject root;
  root.insert(QStringLiteral("trackSets"), trackSets);
  root.insert(QStringLiteral("durationsMs"), durations);
  if (!meta.isEmpty()) root.insert(QStringLiteral("meta"), meta);
  writeObject(QStringLiteral("playlist_tracks.json"), root);
}

}  // namespace tramp
