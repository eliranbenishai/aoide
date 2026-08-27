#include "files.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace aoide {
namespace {

const QStringList kAudio = {QStringLiteral("mp3"), QStringLiteral("m4a"), QStringLiteral("aac"),
                            QStringLiteral("flac"), QStringLiteral("wav"), QStringLiteral("ogg"),
                            QStringLiteral("opus")};
const QStringList kPlaylist = {QStringLiteral("m3u"), QStringLiteral("m3u8")};

QString extOf(const QString& path) {
  return QFileInfo(path).suffix().toLower();
}

QStringList collectAudioFiles(const QString& dirPath) {
  QStringList paths;
  QDir dir(dirPath);
  const auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                         QDir::Name | QDir::IgnoreCase);
  for (const QFileInfo& info : entries) {
    if (info.isDir()) {
      paths += collectAudioFiles(info.absoluteFilePath());
    } else if (isAudioPath(info.absoluteFilePath())) {
      paths.push_back(info.absoluteFilePath());
    }
  }
  return paths;
}

}  // namespace

bool isPlaylistPath(const QString& path) { return kPlaylist.contains(extOf(path)); }
bool isAudioPath(const QString& path) { return kAudio.contains(extOf(path)); }
QStringList audioExtensions() { return kAudio; }
QStringList playlistExtensions() { return kPlaylist; }

QVector<Track> tracksFromPaths(const QStringList& paths) {
  QStringList audioPaths;
  for (const QString& path : paths) {
    const QFileInfo info(path);
    if (info.isDir()) {
      audioPaths += collectAudioFiles(info.absoluteFilePath());
    } else if (info.isFile() && isAudioPath(path)) {
      audioPaths.push_back(info.absoluteFilePath());
    }
  }
  std::sort(audioPaths.begin(), audioPaths.end(), [](const QString& a, const QString& b) {
    return QFileInfo(a).fileName().toLower() < QFileInfo(b).fileName().toLower();
  });
  QVector<Track> tracks;
  tracks.reserve(audioPaths.size());
  for (const QString& p : audioPaths) {
    Track t;
    t.path = p;
    tracks.push_back(t);
  }
  return tracks;
}

}  // namespace aoide
