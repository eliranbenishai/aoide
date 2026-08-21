#include "m3u.h"

#include <QStringDecoder>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace tramp {

QString decodeM3uBytes(const QByteArray& bytes) {
  if (bytes.startsWith("\xEF\xBB\xBF")) {
    return QString::fromUtf8(bytes.mid(3));
  }
  if (bytes.size() >= 2) {
    const uchar b0 = uchar(bytes[0]);
    const uchar b1 = uchar(bytes[1]);
    if ((b0 == 0xFF && b1 == 0xFE) || (b0 == 0xFE && b1 == 0xFF)) {
      QStringDecoder utf16(QStringConverter::Utf16);
      QString out = utf16(bytes);
      if (!utf16.hasError()) return out;
    }
  }
  QStringDecoder utf8(QStringConverter::Utf8);
  QString out = utf8(bytes);
  if (!utf8.hasError()) return out;
  // Not UTF-8. Latin-1 never fails and keeps every byte distinguishable, which
  // is the best available guess for a legacy playlist.
  return QString::fromLatin1(bytes);
}
namespace {

bool fileExists(const QString& path) {
  return QFileInfo::exists(path) && QFileInfo(path).isFile();
}

QString posixJoin(const QString& dir, const QString& relative) {
  if (dir.isEmpty()) {
    return relative;
  }
  if (relative.isEmpty()) {
    return dir;
  }
  if (dir.endsWith(QLatin1Char('/')) || dir.endsWith(QLatin1Char('\\'))) {
    return dir + relative;
  }
  return dir + QLatin1Char('/') + relative;
}

QString posixNormalize(QString path) {
  path.replace(QLatin1Char('\\'), QLatin1Char('/'));
  return QDir::cleanPath(path);
}

}  // namespace

M3uCodec::M3uCodec(Exists exists) : exists_(std::move(exists)) {
  if (!exists_) {
    exists_ = fileExists;
  }
}

QVector<Track> M3uCodec::parse(const QString& contents,
                               const QString& playlistFilePath) const {
  const QString dir = QFileInfo(playlistFilePath).path();
  QVector<Track> tracks;
  std::optional<qint64> pendingDuration;
  QString pendingTitle;
  QString pendingArtist;

  const QStringList lines = contents.split(QRegularExpression(QStringLiteral("\\r?\\n")));
  for (QString rawLine : lines) {
    const QString line = rawLine.trimmed();
    if (line.isEmpty()) {
      continue;
    }
    if (line.startsWith(QStringLiteral("#EXTINF:"))) {
      const QString rest = line.mid(int(QStringLiteral("#EXTINF:").size()));
      const int comma = rest.indexOf(QLatin1Char(','));
      const QString durationPart = comma >= 0 ? rest.left(comma) : rest;
      const QString meta = comma >= 0 ? rest.mid(comma + 1).trimmed() : QString();
      bool ok = false;
      const int secs = durationPart.trimmed().toInt(&ok);
      pendingDuration = qint64(ok ? secs : 0) * 1000;
      if (meta.contains(QStringLiteral(" - "))) {
        const int split = meta.indexOf(QStringLiteral(" - "));
        pendingArtist = meta.left(split).trimmed();
        pendingTitle = meta.mid(split + 3).trimmed();
      } else if (!meta.isEmpty()) {
        pendingTitle = meta;
        pendingArtist.clear();
      }
      continue;
    }
    if (line.startsWith(QLatin1Char('#'))) {
      continue;
    }

    Track track;
    track.path = resolve(line, dir);
    track.title = pendingTitle;
    track.artist = pendingArtist;
    track.durationMs = pendingDuration;
    tracks.push_back(track);
    pendingDuration.reset();
    pendingTitle.clear();
    pendingArtist.clear();
  }
  return tracks;
}

QString M3uCodec::resolve(const QString& line, const QString& dir) const {
  const QFileInfo info(line);
  const QString direct = info.isAbsolute() ? posixNormalize(line)
                                           : posixNormalize(posixJoin(dir, line));
  if (exists_(direct)) {
    return direct;
  }
  const QStringList segs = segments(line);
  for (int take = segs.size(); take >= 1; --take) {
    QString tail = segs.mid(segs.size() - take).join(QLatin1Char('/'));
    const QString candidate = posixNormalize(posixJoin(dir, tail));
    if (candidate != direct && exists_(candidate)) {
      return candidate;
    }
  }
  return direct;
}

QStringList M3uCodec::segments(const QString& line) {
  QStringList out;
  const QStringList parts = line.split(QRegularExpression(QStringLiteral("[\\\\/]+")));
  for (const QString& part : parts) {
    if (!part.isEmpty() && part != QLatin1Char('.')) {
      out.push_back(part);
    }
  }
  return out;
}

QString M3uCodec::encode(const QVector<Track>& tracks) const {
  QString buf = QStringLiteral("#EXTM3U\n");
  for (const Track& t : tracks) {
    QStringList labelParts;
    if (!t.artist.trimmed().isEmpty()) {
      labelParts << t.artist.trimmed();
    }
    if (!t.title.trimmed().isEmpty()) {
      labelParts << t.title.trimmed();
    }
    const QString label = labelParts.join(QStringLiteral(" - "));
    if (t.durationMs.has_value()) {
      const qint64 secs = t.durationMs.value() / 1000;
      buf += QStringLiteral("#EXTINF:%1,%2\n")
                 .arg(secs)
                 .arg(label.isEmpty() ? t.displayTitle() : label);
    }
    buf += t.path;
    buf += QLatin1Char('\n');
  }
  return buf;
}

}  // namespace tramp
