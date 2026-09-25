#include "persist.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace aoide {
namespace {

QJsonObject metadataToJson(const TrackMetadata& metadata) {
  QJsonObject o;
  auto put = [&](const char* key, const QString& value) {
    if (!value.isEmpty()) o.insert(QLatin1String(key), value);
  };
  put("title", metadata.title);
  put("artist", metadata.artist);
  put("album", metadata.album);
  put("albumArtist", metadata.albumArtist);
  put("genre", metadata.genre);
  put("trackNumber", metadata.trackNumber);
  put("discNumber", metadata.discNumber);
  put("composer", metadata.composer);
  if (metadata.year) o.insert(QStringLiteral("year"), *metadata.year);
  return o;
}

TrackMetadata metadataFromJson(const QJsonObject& o) {
  TrackMetadata metadata;
  metadata.title = o.value(QStringLiteral("title")).toString();
  metadata.artist = o.value(QStringLiteral("artist")).toString();
  metadata.album = o.value(QStringLiteral("album")).toString();
  metadata.albumArtist = o.value(QStringLiteral("albumArtist")).toString();
  metadata.genre = o.value(QStringLiteral("genre")).toString();
  metadata.trackNumber = o.value(QStringLiteral("trackNumber")).toString();
  metadata.discNumber = o.value(QStringLiteral("discNumber")).toString();
  metadata.composer = o.value(QStringLiteral("composer")).toString();
  const int year = o.value(QStringLiteral("year")).toInt();
  if (year > 0) metadata.year = year;
  return metadata;
}

QJsonObject trackToJson(const Track& t) {
  QJsonObject o = metadataToJson(trackMetadata(t));
  o.insert(QStringLiteral("path"), t.path);
  if (t.durationMs) o.insert(QStringLiteral("durationMs"), qint64(*t.durationMs));
  // A disabled row paints faint and is left out of the footer figures. Dropping
  // that on the way to disk made every restored row look playable until the
  // background check caught up.
  if (t.disabled) o.insert(QStringLiteral("disabled"), true);
  return o;
}

Track trackFromJson(const QJsonObject& o) {
  Track t;
  t.path = o.value(QStringLiteral("path")).toString();
  applyTrackMetadata(t, metadataFromJson(o), true);
  if (o.contains(QStringLiteral("durationMs"))) {
    t.durationMs = qint64(o.value(QStringLiteral("durationMs")).toDouble());
  }
  t.disabled = o.value(QStringLiteral("disabled")).toBool();
  return t;
}

/// Everything the state files hold is keyed by path, and a relative one is
/// keyed on whatever directory Aoide was started from. Absolute on the way in
/// and on the way out, so the same collection reads the same from anywhere.
QString absoluteKey(const QString& path) {
  if (path.isEmpty()) return path;
  return normalizePlaylistPath(path);
}

}  // namespace

QString SavedPlaylist::displayName() const {
  if (!name.trimmed().isEmpty()) {
    return name.trimmed();
  }
  return QFileInfo(path).completeBaseName();
}

QVector<PlaylistGroup> defaultPlaylistGroups() {
  return {{0, QStringLiteral("On repeat")}, {1, QStringLiteral("Road trips")},
          {2, QStringLiteral("Weekend")}, {3, QStringLiteral("At home")},
          {4, QStringLiteral("Focus")}, {5, QStringLiteral("After dark")},
          {6, QStringLiteral("To explore")}};
}

QString normalizePlaylistPath(const QString& path) {
  if (isFavoritesPlaylist(path)) return favoritesPlaylistPath();
  const QFileInfo info(path);
  const QString abs = info.isAbsolute() ? info.absoluteFilePath()
                                        : QDir::current().absoluteFilePath(path);
  return QDir::cleanPath(abs);
}

QString favoritesPlaylistPath() { return QStringLiteral("aoide:favorites"); }

bool isFavoritesPlaylist(const QString& path) { return path == favoritesPlaylistPath(); }

bool isReservedPlaylistName(const QString& name) {
  return name.normalized(QString::NormalizationForm_KC).trimmed().toCaseFolded() ==
         QStringLiteral("favorites");
}

SupportStore::SupportStore(QString dir) : dir_(std::move(dir)) {
  QDir().mkpath(dir_);
}

QString SupportStore::filePath(const QString& name) const {
  return QDir(dir_).filePath(name);
}

QJsonObject SupportStore::readObject(const QString& name) const {
  const QString path = filePath(name);
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  const QByteArray raw = file.readAll();
  file.close();
  if (raw.isEmpty()) {
    return {};
  }
  const auto doc = QJsonDocument::fromJson(raw);
  if (doc.isObject()) {
    return doc.object();
  }
  // Present but unreadable. Returning defaults here is how a single bad byte
  // used to erase a collection: the caller persists those defaults straight
  // back over the file. Keep the bytes so the listener can recover them.
  const QString aside = path + QStringLiteral(".corrupt");
  if (!QFile::exists(aside)) {
    QFile::rename(path, aside);
  }
  qWarning("aoide: %s is not valid JSON; kept a copy at %s", qPrintable(name),
           qPrintable(aside));
  return {};
}

bool SupportStore::writeObject(const QString& name, const QJsonObject& o) const {
  // QSaveFile writes to a temporary beside the target and renames on commit, so
  // an interrupted write leaves the previous file untouched instead of empty.
  QSaveFile file(filePath(name));
  if (!file.open(QIODevice::WriteOnly)) {
    qWarning("aoide: cannot open %s for writing", qPrintable(name));
    return false;
  }
  const QByteArray payload = QJsonDocument(o).toJson(QJsonDocument::Compact);
  if (file.write(payload) != payload.size()) {
    file.cancelWriting();
    qWarning("aoide: short write to %s", qPrintable(name));
    return false;
  }
  if (!file.commit()) {
    qWarning("aoide: cannot commit %s", qPrintable(name));
    return false;
  }
  return true;
}

AoideSettings SupportStore::readSettings() const {
  const QJsonObject o = readObject(QStringLiteral("settings.json"));
  if (o.isEmpty()) return {};
  return AoideSettings::fromJson(o);
}

bool SupportStore::writeSettings(const AoideSettings& s) const {
  return writeObject(QStringLiteral("settings.json"), s.toJson());
}

UsageCounters SupportStore::readUsage() const {
  const QJsonObject o = readObject(QStringLiteral("usage.json"));
  UsageCounters u;
  const int spins = o.value(QStringLiteral("spins")).toInt();
  u.spins = spins > 0 ? spins : 0;
  return u;
}

bool SupportStore::writeUsage(const UsageCounters& u) const {
  QJsonObject o;
  o.insert(QStringLiteral("spins"), u.spins);
  return writeObject(QStringLiteral("usage.json"), o);
}

SessionResume SupportStore::readResume() const {
  const QJsonObject o = readObject(QStringLiteral("session_resume.json"));
  SessionResume r;
  if (o.contains(QStringLiteral("playingIndex")) && !o.value(QStringLiteral("playingIndex")).isNull()) {
    r.playingIndex = o.value(QStringLiteral("playingIndex")).toInt();
  }
  r.playingPath = o.value(QStringLiteral("playingPath")).toString();
  r.positionMs = qint64(o.value(QStringLiteral("positionMs")).toDouble());
  r.wasPlaying = o.value(QStringLiteral("wasPlaying")).toBool();
  return r;
}

bool SupportStore::writeResume(const SessionResume& r) const {
  QJsonObject o;
  if (r.playingIndex) {
    o.insert(QStringLiteral("playingIndex"), *r.playingIndex);
  } else {
    o.insert(QStringLiteral("playingIndex"), QJsonValue::Null);
  }
  // Quitting while the transport holds an off-list track used to persist a
  // null index and drop the path, so relaunch had nothing to reopen.
  if (!r.playingPath.isEmpty()) {
    o.insert(QStringLiteral("playingPath"), r.playingPath);
  }
  o.insert(QStringLiteral("positionMs"), r.positionMs);
  o.insert(QStringLiteral("wasPlaying"), r.wasPlaying);
  return writeObject(QStringLiteral("session_resume.json"), o);
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

bool SupportStore::writeAltered(const AlteredPlaylist& p) const {
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
  return writeObject(QStringLiteral("altered_playlist.json"), o);
}

bool SupportStore::clearAltered() const {
  const QString path = filePath(QStringLiteral("altered_playlist.json"));
  return !QFileInfo::exists(path) || QFile::remove(path);
}

QString SupportStore::readLastPlaylistPath() const {
  return readObject(QStringLiteral("session.json")).value(QStringLiteral("lastPlaylistPath")).toString();
}

bool SupportStore::writeLastPlaylistPath(const QString& path) const {
  QJsonObject o;
  if (path.isEmpty()) {
    o.insert(QStringLiteral("lastPlaylistPath"), QJsonValue::Null);
  } else {
    o.insert(QStringLiteral("lastPlaylistPath"), path);
  }
  return writeObject(QStringLiteral("session.json"), o);
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
    if (e.path.isEmpty() || isFavoritesPlaylist(e.path)) continue;
    e.path = normalizePlaylistPath(e.path);
    e.name = o.value(QStringLiteral("name")).toString();
    e.trackCount = o.value(QStringLiteral("trackCount")).toInt();
    e.totalDurationMs = qint64(o.value(QStringLiteral("totalDurationMs")).toDouble());
    e.modifiedMs = qint64(o.value(QStringLiteral("modifiedMs")).toDouble());
    for (const QJsonValue& group : o.value(QStringLiteral("groupIds")).toArray()) {
      const int id = group.toInt(-1);
      if (id >= 0 && id < 7) e.groupIds.insert(id);
    }
    out.push_back(e);
  }
  return out;
}

bool SupportStore::writeCollectionIndex(const QVector<SavedPlaylist>& entries) const {
  QJsonArray raw;
  for (const SavedPlaylist& e : entries) {
    if (e.path.isEmpty() || isFavoritesPlaylist(e.path)) continue;
    QJsonObject o;
    o.insert(QStringLiteral("path"), absoluteKey(e.path));
    if (!e.name.isEmpty()) o.insert(QStringLiteral("name"), e.name);
    o.insert(QStringLiteral("trackCount"), e.trackCount);
    o.insert(QStringLiteral("totalDurationMs"), e.totalDurationMs);
    if (e.modifiedMs != 0) o.insert(QStringLiteral("modifiedMs"), e.modifiedMs);
    QJsonArray groups;
    for (int id = 0; id < 7; ++id) {
      if (e.groupIds.contains(id)) groups.append(id);
    }
    if (!groups.isEmpty()) o.insert(QStringLiteral("groupIds"), groups);
    raw.append(o);
  }
  QJsonObject root;
  root.insert(QStringLiteral("entries"), raw);
  return writeObject(QStringLiteral("playlists.json"), root);
}

QVector<PlaylistGroup> SupportStore::readPlaylistGroups() const {
  QVector<PlaylistGroup> groups = defaultPlaylistGroups();
  const QJsonArray raw =
      readObject(QStringLiteral("playlist_groups.json")).value(QStringLiteral("groups")).toArray();
  for (const QJsonValue& value : raw) {
    const QJsonObject group = value.toObject();
    const int id = group.value(QStringLiteral("id")).toInt(-1);
    const QString name = group.value(QStringLiteral("name")).toString().trimmed();
    if (id >= 0 && id < groups.size() && !name.isEmpty()) groups[id].name = name;
  }
  return groups;
}

bool SupportStore::writePlaylistGroups(const QVector<PlaylistGroup>& groups) const {
  QJsonArray raw;
  for (const PlaylistGroup& group : groups) {
    if (group.id < 0 || group.id >= 7 || group.name.trimmed().isEmpty()) continue;
    QJsonObject value;
    value.insert(QStringLiteral("id"), group.id);
    value.insert(QStringLiteral("name"), group.name.trimmed());
    raw.append(value);
  }
  QJsonObject root;
  root.insert(QStringLiteral("groups"), raw);
  return writeObject(QStringLiteral("playlist_groups.json"), root);
}

QVector<Track> SupportStore::readFavorites() const {
  QVector<Track> tracks;
  QSet<QString> seen;
  const QJsonArray raw =
      readObject(QStringLiteral("favorites.json")).value(QStringLiteral("tracks")).toArray();
  for (const QJsonValue& value : raw) {
    if (!value.isObject()) continue;
    Track track = trackFromJson(value.toObject());
    if (track.path.isEmpty() || track.path.contains(QChar::Null) ||
        isFavoritesPlaylist(track.path)) continue;
    track.path = absoluteKey(track.path);
    if (seen.contains(track.path)) continue;
    seen.insert(track.path);
    tracks.push_back(track);
  }
  return tracks;
}

bool SupportStore::writeFavorites(const QVector<Track>& tracks) const {
  QJsonArray raw;
  QSet<QString> seen;
  for (Track track : tracks) {
    if (track.path.isEmpty() || track.path.contains(QChar::Null) ||
        isFavoritesPlaylist(track.path)) continue;
    track.path = absoluteKey(track.path);
    if (seen.contains(track.path)) continue;
    seen.insert(track.path);
    raw.append(trackToJson(track));
  }
  QJsonObject root;
  root.insert(QStringLiteral("tracks"), raw);
  return writeObject(QStringLiteral("favorites.json"), root);
}

CollectionTrackSets SupportStore::readTrackSets() const {
  CollectionTrackSets sets;
  const QJsonObject o = readObject(QStringLiteral("playlist_tracks.json"));
  const QJsonObject trackSets = o.value(QStringLiteral("trackSets")).toObject();
  for (auto it = trackSets.begin(); it != trackSets.end(); ++it) {
    if (it.key().isEmpty()) continue;
    QStringList paths;
    for (const QJsonValue& v : it.value().toArray()) {
      const QString path = v.toString();
      // No filesystem allows NUL in a name, so a cached path carrying one was
      // never a track. A build before playlist ingest refused binary files
      // could write thousands of them from a single mis-added audio file.
      if (path.isEmpty() || path.contains(QChar::Null)) continue;
      paths.push_back(absoluteKey(path));
    }
    sets.byEntry.insert(absoluteKey(it.key()), paths);
  }
  const QJsonObject durations = o.value(QStringLiteral("durationsMs")).toObject();
  for (auto it = durations.begin(); it != durations.end(); ++it) {
    if (it.key().isEmpty()) continue;
    sets.durationsMs.insert(absoluteKey(it.key()), qint64(it.value().toDouble()));
  }
  const QJsonObject meta = o.value(QStringLiteral("meta")).toObject();
  for (auto it = meta.begin(); it != meta.end(); ++it) {
    if (it.key().isEmpty()) continue;
    const QJsonObject m = it.value().toObject();
    const CachedTrackMeta tags = metadataFromJson(m);
    if (tags.hasTags()) {
      sets.meta.insert(absoluteKey(it.key()), tags);
    }
  }
  return sets;
}

bool SupportStore::writeTrackSets(const CollectionTrackSets& sets) const {
  QJsonObject trackSets;
  for (auto it = sets.byEntry.begin(); it != sets.byEntry.end(); ++it) {
    if (it.key().isEmpty()) continue;
    QJsonArray arr;
    for (const QString& p : it.value()) {
      if (p.isEmpty()) continue;
      arr.append(absoluteKey(p));
    }
    trackSets.insert(absoluteKey(it.key()), arr);
  }
  QJsonObject durations;
  for (auto it = sets.durationsMs.begin(); it != sets.durationsMs.end(); ++it) {
    if (it.key().isEmpty()) continue;
    durations.insert(absoluteKey(it.key()), it.value());
  }
  QJsonObject meta;
  for (auto it = sets.meta.begin(); it != sets.meta.end(); ++it) {
    if (it.key().isEmpty()) continue;
    const QJsonObject m = metadataToJson(it.value());
    if (!m.isEmpty()) meta.insert(absoluteKey(it.key()), m);
  }
  QJsonObject root;
  root.insert(QStringLiteral("trackSets"), trackSets);
  root.insert(QStringLiteral("durationsMs"), durations);
  if (!meta.isEmpty()) root.insert(QStringLiteral("meta"), meta);
  return writeObject(QStringLiteral("playlist_tracks.json"), root);
}

void writeSessionPersist(const SupportStore& store, PersistHealth& health,
                         const AoideSettings& settings, const SessionResume& resume,
                         const UsageCounters& usage, const QString& lastPlaylistPath,
                         const AlteredPlaylist* altered) {
  health.settingsOk = store.writeSettings(settings);
  health.resumeOk = store.writeResume(resume);
  health.usageOk = store.writeUsage(usage);
  if (!lastPlaylistPath.isEmpty()) {
    health.lastPlaylistOk = store.writeLastPlaylistPath(lastPlaylistPath);
  }
  if (altered) {
    health.alteredOk = store.writeAltered(*altered);
  } else if (!lastPlaylistPath.isEmpty() && health.lastPlaylistOk) {
    // A quit may precede the debounced cleanup after loading Favorites (or
    // another saved list). Do not let an older altered list win next launch.
    health.alteredOk = store.clearAltered();
  }
}

}  // namespace aoide
