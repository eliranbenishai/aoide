#include "m3u.h"

#include <QStringDecoder>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace aoide {
namespace {

// The C1 range is the difference between Windows-1252 and Latin-1. Undefined
// bytes retain their control value so an uncertain import never loses bytes.
constexpr char16_t windows1252C1[] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
};

// Windows-1255 is deliberately explicit: QStringDecoder's legacy codecs
// depend on the platform/ICU build. 0xFFFF marks an undefined byte.
constexpr char16_t windows1255High[] = {
    0x20AC, 0xFFFF, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0xFFFF, 0x2039, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0xFFFF, 0x203A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AA, 0x00A5, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7,
    0x05B8, 0x05B9, 0xFFFF, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
    0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05F0, 0x05F1, 0x05F2, 0x05F3,
    0x05F4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
    0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
    0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
    0x05E8, 0x05E9, 0x05EA, 0xFFFF, 0xFFFF, 0x200E, 0x200F, 0xFFFF,
};

QString decodeWindows1252(const QByteArray& bytes) {
  QString text;
  text.reserve(bytes.size());
  for (const char raw : bytes) {
    const uchar byte = static_cast<uchar>(raw);
    text += QChar(byte >= 0x80 && byte < 0xA0 ? windows1252C1[byte - 0x80] : byte);
  }
  return text;
}

std::optional<QString> decodeWindows1255(const QByteArray& bytes) {
  QString text;
  text.reserve(bytes.size());
  for (const char raw : bytes) {
    const uchar byte = static_cast<uchar>(raw);
    const char16_t value = byte >= 0x80 ? windows1255High[byte - 0x80] : byte;
    if (value == 0xFFFF) return std::nullopt;
    text += QChar(value);
  }
  return text;
}

std::optional<QString> undoMojibake(const QString& text, bool windows1252) {
  QByteArray bytes;
  bytes.reserve(text.size());
  for (const QChar character : text) {
    const ushort value = character.unicode();
    if (!windows1252) {
      if (value > 0xFF) return std::nullopt;
      bytes += char(value);
      continue;
    }
    if (value < 0x80 || (value >= 0xA0 && value <= 0xFF)) {
      bytes += char(value);
      continue;
    }
    bool found = false;
    for (int i = 0; i < 32; ++i) {
      if (value == windows1252C1[i]) {
        bytes += char(0x80 + i);
        found = true;
        break;
      }
    }
    if (!found) return std::nullopt;
  }
  QStringDecoder utf8(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
  const QString repaired = utf8(bytes);
  if (utf8.hasError() || repaired == text) return std::nullopt;
  return repaired;
}

std::optional<QString> undoPlaylistMojibake(const QString& text, bool windows1252) {
  bool changed = false;
  const auto repairUnit = [&](const QString& unit) {
    if (const auto repaired = undoMojibake(unit, windows1252)) {
      changed = true;
      return *repaired;
    }
    return unit;
  };
  QString repaired;
  repaired.reserve(text.size());
  bool first = true;
  for (const auto lineView : QStringView(text).tokenize(QLatin1Char('\n'))) {
    if (!first) repaired += QLatin1Char('\n');
    first = false;
    const QString line = lineView.toString();
    const auto trimmed = lineView.trimmed();
    if (trimmed.startsWith(QLatin1String("#EXTINF:"))) {
      const auto comma = line.indexOf(QLatin1Char(','));
      if (comma < 0) {
        repaired += line;
        continue;
      }
      const QString label = line.mid(comma + 1);
      const auto split = label.indexOf(QLatin1String(" - "));
      repaired += line.left(comma + 1);
      // Artist and title can have different histories. A correct Unicode
      // field must not block review of reversible text in its neighbour.
      repaired += split < 0 ? repairUnit(label)
                            : repairUnit(label.left(split)) + QStringLiteral(" - ") +
                                  repairUnit(label.mid(split + 3));
    } else {
      // Treat each path as one unit. Preserve comments and line endings.
      repaired += trimmed.startsWith(QLatin1Char('#')) ? line : repairUnit(line);
    }
  }
  return changed ? std::optional<QString>(repaired) : std::nullopt;
}

}  // namespace

QString decodeM3uBytes(const QByteArray& bytes) {
  return M3uCodec().decode(bytes).text;
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

M3uDecodeResult M3uCodec::decode(const QByteArray& bytes, const QString& playlistFilePath) const {
  M3uDecodeResult result;
  const auto choose = [&]() {
    const auto& chosen = result.alternatives.front();
    result.text = chosen.text;
    result.encoding = chosen.encoding;
    result.matchedPaths = chosen.matchedPaths;
  };
  if (bytes.startsWith("\xFF\xFE") || bytes.startsWith("\xFE\xFF")) {
    QStringDecoder utf16(QStringConverter::Utf16, QStringConverter::Flag::Stateless);
    const QString text = utf16(bytes);
    result.alternatives.push_back({text, QStringLiteral("UTF-16")});
    result.needsReview = utf16.hasError();
    choose();
    return result;
  }
  const bool hasUtf8Bom = bytes.startsWith("\xEF\xBB\xBF");
  QStringDecoder utf8(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
  const QString unicode = utf8(hasUtf8Bom ? bytes.mid(3) : bytes);
  if (!utf8.hasError() || hasUtf8Bom) {
    result.alternatives.push_back({unicode, QStringLiteral("UTF-8")});
    if (utf8.hasError()) {
      result.needsReview = true;
      choose();
      return result;
    }
    const auto repaired = undoPlaylistMojibake(unicode, true);
    if (repaired) {
      result.alternatives.push_back(
          {*repaired, QStringLiteral("UTF-8 (recovered from Windows-1252)")});
    } else if (const auto latin1 = undoPlaylistMojibake(unicode, false)) {
      result.alternatives.push_back(
          {*latin1, QStringLiteral("UTF-8 (recovered from Latin-1)")});
    }
  } else {
    result.alternatives.push_back({decodeWindows1252(bytes), QStringLiteral("Windows-1252")});
    const auto hebrew = decodeWindows1255(bytes);
    if (hebrew && *hebrew != result.alternatives.front().text)
      result.alternatives.push_back({*hebrew, QStringLiteral("Windows-1255")});
  }

  QVector<QSet<int>> matchedLines(result.alternatives.size());
  if (result.alternatives.size() > 1 && !playlistFilePath.isEmpty()) {
    const QString dir = QFileInfo(playlistFilePath).path();
    for (int i = 0; i < result.alternatives.size(); ++i) {
      auto& candidate = result.alternatives[i];
      if (!isPlaylistText(candidate.text)) continue;
      QSet<QString> checked;
      int lineNumber = 0;
      for (const auto lineView : QStringView(candidate.text).tokenize(QLatin1Char('\n'))) {
        ++lineNumber;
        const QString line = lineView.trimmed().toString();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || checked.contains(line)) continue;
        checked.insert(line);
        if (exists_(resolve(line, dir))) {
          ++candidate.matchedPaths;
          matchedLines[i].insert(lineNumber);
        }
        // Evidence gathering must not stat an unbounded playlist repeatedly.
        if (checked.size() == 256) break;
      }
    }
  }
  int best = 0;
  for (int i = 1; i < result.alternatives.size(); ++i)
    if (result.alternatives[i].matchedPaths > result.alternatives[best].matchedPaths) best = i;
  for (int i = 0; i < result.alternatives.size(); ++i) {
    if (i == best) continue;
    auto competing = matchedLines[i];
    competing.subtract(matchedLines[best]);
    // More matches are not conclusive if another interpretation finds tracks
    // this one loses. Identical ASCII paths provide no encoding evidence.
    if (!competing.isEmpty() ||
        result.alternatives[i].matchedPaths == result.alternatives[best].matchedPaths)
      result.needsReview = true;
  }
  // A BOM is an explicit encoding declaration. Leave it alone unless actual
  // path evidence supports repairing a prior, reversible mis-decoding.
  if (hasUtf8Bom && best == 0) {
    result.alternatives.resize(1);
    result.needsReview = false;
  }
  if (best != 0) result.alternatives.swapItemsAt(0, best);
  choose();
  result.recovered = result.encoding != QStringLiteral("UTF-8");
  return result;
}

QVector<Track> M3uCodec::parse(const QString& contents,
                               const QString& playlistFilePath) const {
  if (!isPlaylistText(contents)) return {};
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
  // A line that is not a path can be tens of thousands of slashes; walking
  // every tail is unbounded string work and stat calls.
  const int maxTake = qMin(int(segs.size()), 32);
  for (int take = maxTake; take >= 1; --take) {
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

}  // namespace aoide
