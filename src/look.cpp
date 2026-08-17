#include "look.h"

#include "tramp_fonts.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <cmath>

namespace tramp {
namespace {

thread_local const ChromeTokens* g_currentLook = nullptr;

const QRegularExpression kLookIdRe(QStringLiteral("^[a-z0-9]+(-[a-z0-9]+)*$"));
const QRegularExpression kColorHexRe(
    QStringLiteral("^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$"));
constexpr int kMaxChain = 8;
const QStringList kBundledCatalogOrder = {
    QStringLiteral("arc"),       QStringLiteral("shield"), QStringLiteral("thunder"),
    QStringLiteral("gamma"),     QStringLiteral("widow"),  QStringLiteral("marksman"),
    QStringLiteral("chaos"),
};
const QStringList kRetiredBundledIds = {QStringLiteral("amber-terminal"),
                                        QStringLiteral("violet-pulse")};

QString canonicalActiveSkinId(const QString& id) {
  if (id.isEmpty() || kRetiredBundledIds.contains(id)) return QStringLiteral("builtin");
  return id;
}

[[noreturn]] void fail(const QString& message) { throw LookError(message); }

bool isValidLookId(const QString& id) {
  return id != QLatin1String("builtin") && kLookIdRe.match(id).hasMatch();
}

QColor colorFromHex(const QString& hex, const QString& path) {
  if (!kColorHexRe.match(hex).hasMatch()) {
    fail(path + QStringLiteral(" must be #RRGGBB or #RRGGBBAA"));
  }
  const QString body = hex.mid(1);
  bool ok = false;
  const uint rgb = body.left(6).toUInt(&ok, 16);
  int a = 255;
  if (body.size() == 8) {
    a = int(body.mid(6, 2).toUInt(&ok, 16));
  }
  return QColor(int((rgb >> 16) & 255), int((rgb >> 8) & 255), int(rgb & 255), a);
}

QString parseColorValue(const QJsonValue& value, const QString& path) {
  if (!value.isString()) {
    fail(path + QStringLiteral(" must be #RRGGBB or #RRGGBBAA"));
  }
  const QString hex = value.toString();
  colorFromHex(hex, path);
  return hex;
}

qreal parseOpacity(const QJsonValue& value, const QString& path) {
  if (!value.isDouble() && !value.isString()) {
    fail(path + QStringLiteral(" must be a number from 0 to 1"));
  }
  const qreal n = value.toDouble();
  if (n < 0 || n > 1) {
    fail(path + QStringLiteral(" must be a number from 0 to 1"));
  }
  return n;
}

QJsonArray parseStops(const QJsonValue& value, const QString& path) {
  if (!value.isArray() || value.toArray().isEmpty()) {
    fail(path + QStringLiteral(" must be a non-empty list of colors"));
  }
  QJsonArray out;
  const QJsonArray in = value.toArray();
  for (int i = 0; i < in.size(); ++i) {
    out.append(parseColorValue(in.at(i), path + QStringLiteral("[%1]").arg(i)));
  }
  return out;
}

QString parseFontFile(const QJsonValue& value, const QString& path) {
  if (!value.isString()) {
    fail(path + QStringLiteral(" must be a pack-relative .ttf or .otf path"));
  }
  const QString raw = value.toString();
  const QString normalized = QString(raw).replace(QLatin1Char('\\'), QLatin1Char('/'));
  if (!normalized.contains(QRegularExpression(QStringLiteral("\\.(ttf|otf)$"),
                                              QRegularExpression::CaseInsensitiveOption))) {
    fail(path + QStringLiteral(" must be a pack-relative .ttf or .otf path"));
  }
  if (QFileInfo(normalized).isAbsolute() || normalized.startsWith(QLatin1Char('/')) ||
      QRegularExpression(QStringLiteral("^[a-zA-Z]:")).match(normalized).hasMatch()) {
    fail(path + QStringLiteral(" must not be an absolute path"));
  }
  for (const QString& segment : normalized.split(QLatin1Char('/'))) {
    if (segment == QLatin1String("..")) {
      fail(path + QStringLiteral(" must not contain .. segments"));
    }
  }
  return raw;
}

int parseFontWeight(const QJsonValue& value, const QString& path) {
  if (!value.isDouble()) {
    fail(path + QStringLiteral(" must be an integer"));
  }
  return value.toInt();
}

const QStringList kShellKeys = {QStringLiteral("highlight"), QStringLiteral("base"),
                                QStringLiteral("mid"), QStringLiteral("low"),
                                QStringLiteral("deep")};
const QStringList kInkKeys = {QStringLiteral("default"), QStringLiteral("dim"),
                              QStringLiteral("faint")};
const QStringList kPhosKeys = {QStringLiteral("default"), QStringLiteral("hot"),
                               QStringLiteral("dim"), QStringLiteral("deep")};
const QStringList kAccentKeys = {QStringLiteral("default"), QStringLiteral("dim")};

QStringList knownColorKeys(const QString& group) {
  if (group == QLatin1String("shell")) return kShellKeys;
  if (group == QLatin1String("ink")) return kInkKeys;
  if (group == QLatin1String("phosphor")) return kPhosKeys;
  if (group == QLatin1String("accent")) return kAccentKeys;
  return {};
}

QJsonObject parseColors(const QJsonValue& value) {
  if (value.isUndefined() || value.isNull()) return {};
  if (!value.isObject()) fail(QStringLiteral("colors must be an object"));
  QJsonObject result;
  const QJsonObject obj = value.toObject();
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    const QString key = it.key();
    if (key == QLatin1String("well")) {
      result.insert(key, parseColorValue(it.value(), QStringLiteral("colors.well")));
      continue;
    }
    const QStringList known = knownColorKeys(key);
    if (known.isEmpty()) {
      fail(QStringLiteral("unknown color key: ") + key);
    }
    if (!it.value().isObject()) {
      fail(QStringLiteral("colors.") + key + QStringLiteral(" must be an object"));
    }
    QJsonObject group;
    const QJsonObject raw = it.value().toObject();
    for (auto git = raw.begin(); git != raw.end(); ++git) {
      if (!known.contains(git.key())) {
        fail(QStringLiteral("unknown color key: ") + key + QLatin1Char('.') + git.key());
      }
      group.insert(git.key(), parseColorValue(
                                  git.value(), QStringLiteral("colors.%1.%2").arg(key, git.key())));
    }
    result.insert(key, group);
  }
  return result;
}

QJsonObject parseMaterials(const QJsonValue& value) {
  if (value.isUndefined() || value.isNull()) return {};
  if (!value.isObject()) fail(QStringLiteral("materials must be an object"));
  QJsonObject result;
  const QJsonObject obj = value.toObject();
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    const QString key = it.key();
    QStringList known;
    if (key == QLatin1String("bevel")) {
      known = {QStringLiteral("lightOpacity"), QStringLiteral("softOpacity")};
    } else if (key == QLatin1String("spectrum") || key == QLatin1String("rail")) {
      known = {QStringLiteral("stops")};
    } else {
      fail(QStringLiteral("unknown material key: ") + key);
    }
    if (!it.value().isObject()) {
      fail(QStringLiteral("materials.") + key + QStringLiteral(" must be an object"));
    }
    QJsonObject group;
    const QJsonObject raw = it.value().toObject();
    for (auto git = raw.begin(); git != raw.end(); ++git) {
      if (!known.contains(git.key())) {
        fail(QStringLiteral("unknown material key: ") + key + QLatin1Char('.') + git.key());
      }
      const QString path = QStringLiteral("materials.%1.%2").arg(key, git.key());
      if (git.key() == QLatin1String("lightOpacity") || git.key() == QLatin1String("softOpacity")) {
        group.insert(git.key(), parseOpacity(git.value(), path));
      } else {
        group.insert(git.key(), parseStops(git.value(), path));
      }
    }
    result.insert(key, group);
  }
  return result;
}

QJsonObject parseFonts(const QJsonValue& value) {
  if (value.isUndefined() || value.isNull()) return {};
  if (!value.isObject()) fail(QStringLiteral("fonts must be an object"));
  QJsonObject result;
  const QJsonObject obj = value.toObject();
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    const QString key = it.key();
    if (key != QLatin1String("chrome") && key != QLatin1String("lcd")) {
      fail(QStringLiteral("unknown font role: ") + key);
    }
    if (!it.value().isObject()) {
      fail(QStringLiteral("fonts.") + key + QStringLiteral(" must be an object"));
    }
    QJsonObject role;
    const QJsonObject raw = it.value().toObject();
    for (auto rit = raw.begin(); rit != raw.end(); ++rit) {
      if (rit.key() == QLatin1String("file")) {
        role.insert(QStringLiteral("file"),
                    parseFontFile(rit.value(), QStringLiteral("fonts.%1.file").arg(key)));
      } else if (rit.key() == QLatin1String("weight")) {
        role.insert(QStringLiteral("weight"),
                    parseFontWeight(rit.value(), QStringLiteral("fonts.%1.weight").arg(key)));
      } else {
        fail(QStringLiteral("unknown font key: ") + key + QLatin1Char('.') + rit.key());
      }
    }
    if (!role.contains(QStringLiteral("file"))) {
      fail(QStringLiteral("fonts.") + key + QStringLiteral(".file is required"));
    }
    result.insert(key, role);
  }
  return result;
}

QJsonObject deepMerge(const QJsonObject& base, const QJsonObject& overlay) {
  QJsonObject result = base;
  for (auto it = overlay.begin(); it != overlay.end(); ++it) {
    const QJsonValue existing = result.value(it.key());
    if (it.value().isObject() && existing.isObject()) {
      result.insert(it.key(), deepMerge(existing.toObject(), it.value().toObject()));
    } else {
      result.insert(it.key(), it.value());
    }
  }
  return result;
}

QColor groupColor(const QJsonObject& colors, const QString& group, const QString& key) {
  const QJsonValue g = colors.value(group);
  if (!g.isObject() || !g.toObject().value(key).isString()) {
    fail(QStringLiteral("missing color: ") + group + QLatin1Char('.') + key);
  }
  return colorFromHex(g.toObject().value(key).toString(),
                      QStringLiteral("colors.%1.%2").arg(group, key));
}

QColor scalarColor(const QJsonObject& colors, const QString& key) {
  if (!colors.value(key).isString()) {
    fail(QStringLiteral("missing color: ") + key);
  }
  return colorFromHex(colors.value(key).toString(), QStringLiteral("colors.") + key);
}

QColor lift(const QColor& base, int dr, int dg, int db) {
  return QColor(qBound(0, base.red() + dr, 255), qBound(0, base.green() + dg, 255),
                qBound(0, base.blue() + db, 255), base.alpha());
}

QColor scaleColor(const QColor& accent, double rScale, double gScale, double bScale) {
  return QColor(qBound(0, int(std::round(accent.red() * rScale)), 255),
                qBound(0, int(std::round(accent.green() * gScale)), 255),
                qBound(0, int(std::round(accent.blue() * bScale)), 255));
}

QColor tintTowardWhite(const QColor& color, double greenPull, double bluePull) {
  return QColor(255,
                qBound(0, int(std::round(255 - (255 - color.green()) * greenPull)), 255),
                qBound(0, int(std::round(255 - (255 - color.blue()) * bluePull)), 255));
}

bool copyDir(const QString& src, const QString& dst) {
  QDir().mkpath(dst);
  const QFileInfoList entries =
      QDir(src).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QFileInfo& info : entries) {
    QString destName = info.fileName();
    if (destName == QLatin1String("look.json") && info.isFile()) {
      destName = QStringLiteral("skin.json");
    }
    const QString destPath = QDir(dst).filePath(destName);
    if (info.isDir()) {
      if (!copyDir(info.absoluteFilePath(), destPath)) return false;
    } else {
      QFile::remove(destPath);
      if (!QFile::copy(info.absoluteFilePath(), destPath)) return false;
    }
  }
  return true;
}

QString packRootOf(const QString& filePath) {
  return QFileInfo(filePath).absolutePath();
}

LookManifest readManifestFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    fail(QStringLiteral("failed to read manifest"));
  }
  const auto doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) fail(QStringLiteral("manifest is not an object"));
  LookManifest m = parseLookManifest(doc.object());
  m.packRoot = packRootOf(path);
  return m;
}

}  // namespace

LookPaintScope::LookPaintScope(const ChromeTokens& tokens) : prev_(g_currentLook) {
  g_currentLook = &tokens;
}

LookPaintScope::~LookPaintScope() { g_currentLook = prev_; }

const ChromeTokens& currentLook() {
  return g_currentLook ? *g_currentLook : ChromeTokens::builtin();
}

LookManifest parseLookManifest(const QJsonObject& json, bool allowBuiltin) {
  const int formatVersion = json.value(QStringLiteral("formatVersion")).toInt();
  if (formatVersion != 1) fail(QStringLiteral("formatVersion must be 1"));

  const QString id = json.value(QStringLiteral("id")).toString();
  const bool idOk =
      allowBuiltin && id == QLatin1String("builtin") ? true : isValidLookId(id);
  if (!idOk) fail(QStringLiteral("invalid look pack id: ") + id);

  const QString name = json.value(QStringLiteral("name")).toString();
  if (name.isEmpty()) fail(QStringLiteral("name is required"));

  const QString extendsId = json.value(QStringLiteral("extends")).toString();
  if (extendsId.isEmpty()) fail(QStringLiteral("extends is required"));

  const QJsonValue authorVal = json.value(QStringLiteral("author"));
  if (!authorVal.isUndefined() && !authorVal.isNull() && !authorVal.isString()) {
    fail(QStringLiteral("author must be a string"));
  }

  LookManifest m;
  m.formatVersion = formatVersion;
  m.id = id;
  m.name = name;
  m.author = authorVal.toString();
  m.extendsId = extendsId;
  m.colors = parseColors(json.value(QStringLiteral("colors")));
  m.materials = parseMaterials(json.value(QStringLiteral("materials")));
  m.fonts = parseFonts(json.value(QStringLiteral("fonts")));
  return m;
}

LookManifest builtinLookManifest() {
  static const char* kJson = R"({
    "formatVersion": 1,
    "id": "builtin",
    "name": "Tramp",
    "author": "Tramp",
    "extends": "builtin",
    "colors": {
      "shell": {
        "highlight": "#323744",
        "base": "#262b38",
        "mid": "#1a1d26",
        "low": "#12141a",
        "deep": "#0a0b0e"
      },
      "ink": {
        "default": "#e8eaf0",
        "dim": "#8b919e",
        "faint": "#5b6270"
      },
      "phosphor": {
        "default": "#3de7ff",
        "hot": "#b8f6ff",
        "dim": "#1a7a88",
        "deep": "#0d3d46"
      },
      "accent": {
        "default": "#ff3d9a",
        "dim": "#8a2258"
      },
      "well": "#050608"
    },
    "materials": {
      "bevel": { "lightOpacity": 0.15, "softOpacity": 0.06 },
      "spectrum": { "stops": ["#cbf9ff", "#3de7ff", "#1b9ec4", "#ff3d9a"] },
      "rail": { "stops": ["#1a7a88", "#8a2258", "#1a7a88"] }
    }
  })";
  static const LookManifest cached = [] {
    const auto doc = QJsonDocument::fromJson(kJson);
    return parseLookManifest(doc.object(), true);
  }();
  return cached;
}

LookPalette paletteFromColors(const QJsonObject& colors) {
  LookPalette p;
  p.shellHi = groupColor(colors, QStringLiteral("shell"), QStringLiteral("highlight"));
  p.shell = groupColor(colors, QStringLiteral("shell"), QStringLiteral("base"));
  p.shellMid = groupColor(colors, QStringLiteral("shell"), QStringLiteral("mid"));
  p.shellLo = groupColor(colors, QStringLiteral("shell"), QStringLiteral("low"));
  p.shellDeep = groupColor(colors, QStringLiteral("shell"), QStringLiteral("deep"));
  p.ink = groupColor(colors, QStringLiteral("ink"), QStringLiteral("default"));
  p.inkDim = groupColor(colors, QStringLiteral("ink"), QStringLiteral("dim"));
  p.inkFaint = groupColor(colors, QStringLiteral("ink"), QStringLiteral("faint"));
  p.phos = groupColor(colors, QStringLiteral("phosphor"), QStringLiteral("default"));
  p.phosHot = groupColor(colors, QStringLiteral("phosphor"), QStringLiteral("hot"));
  p.phosDim = groupColor(colors, QStringLiteral("phosphor"), QStringLiteral("dim"));
  p.phosDeep = groupColor(colors, QStringLiteral("phosphor"), QStringLiteral("deep"));
  p.accent = groupColor(colors, QStringLiteral("accent"), QStringLiteral("default"));
  p.accentDim = groupColor(colors, QStringLiteral("accent"), QStringLiteral("dim"));
  p.well = scalarColor(colors, QStringLiteral("well"));
  return p;
}

LookMaterials materialsFromJson(const QJsonObject& materials) {
  LookMaterials m;
  const QJsonObject bevel = materials.value(QStringLiteral("bevel")).toObject();
  if (!bevel.contains(QStringLiteral("lightOpacity")) ||
      !bevel.contains(QStringLiteral("softOpacity"))) {
    fail(QStringLiteral("missing material: bevel.lightOpacity"));
  }
  m.bevelLightOpacity = bevel.value(QStringLiteral("lightOpacity")).toDouble();
  m.bevelSoftOpacity = bevel.value(QStringLiteral("softOpacity")).toDouble();

  auto stops = [&](const QString& group) {
    const QJsonValue g = materials.value(group);
    if (!g.isObject() || !g.toObject().value(QStringLiteral("stops")).isArray()) {
      fail(QStringLiteral("missing material: ") + group + QStringLiteral(".stops"));
    }
    QVector<QColor> out;
    const QJsonArray arr = g.toObject().value(QStringLiteral("stops")).toArray();
    for (const QJsonValue& v : arr) {
      out.push_back(colorFromHex(v.toString(), group));
    }
    return out;
  };
  m.spectrumStops = stops(QStringLiteral("spectrum"));
  m.railStops = stops(QStringLiteral("rail"));
  return m;
}

ResolvedLook resolveLook(const QString& activeId, const QVector<LookManifest>& installed) {
  QVector<LookManifest> chain;
  QStringList visited;
  QString currentId = activeId;
  while (true) {
    if (visited.contains(currentId)) {
      fail(QStringLiteral("look extends cycle detected at ") + currentId);
    }
    if (chain.size() >= kMaxChain) {
      fail(QStringLiteral("look extends chain exceeds %1").arg(kMaxChain));
    }
    visited.push_back(currentId);

    LookManifest manifest;
    if (currentId == QLatin1String("builtin")) {
      manifest = builtinLookManifest();
    } else {
      bool found = false;
      for (const LookManifest& m : installed) {
        if (m.id == currentId) {
          manifest = m;
          found = true;
          break;
        }
      }
      if (!found) fail(QStringLiteral("look pack not found: ") + currentId);
    }
    chain.push_back(manifest);
    if (currentId == QLatin1String("builtin")) break;
    currentId = manifest.extendsId;
  }

  QJsonObject mergedColors;
  QJsonObject mergedMaterials;
  for (int i = chain.size() - 1; i >= 0; --i) {
    mergedColors = deepMerge(mergedColors, chain[i].colors);
    mergedMaterials = deepMerge(mergedMaterials, chain[i].materials);
  }

  const LookManifest& active = chain.front();
  ResolvedLook look;
  look.id = active.id;
  look.name = active.name;
  look.author = active.author;
  look.palette = paletteFromColors(mergedColors);
  look.materials = materialsFromJson(mergedMaterials);
  return look;
}

ChromeTokens ChromeTokens::from(const ResolvedLook& look) {
  ChromeTokens t;
  t.id = look.id;
  t.palette = look.palette;
  t.materials = look.materials;
  t.chromeFamily = look.chromeFamily;
  t.lcdFamily = look.lcdFamily;
  const LookPalette& p = look.palette;
  const LookMaterials& m = look.materials;
  t.shellHi = p.shellHi;
  t.shell = p.shell;
  t.shellMid = p.shellMid;
  t.shellLo = p.shellLo;
  t.shellDeep = p.shellDeep;
  t.ink = p.ink;
  t.inkDim = p.inkDim;
  t.inkFaint = p.inkFaint;
  t.phos = p.phos;
  t.phosHot = p.phosHot;
  t.phosDim = p.phosDim;
  t.phosDeep = p.phosDeep;
  t.accent = p.accent;
  t.accentDim = p.accentDim;
  t.well = p.well;

  t.titleBar0 = lift(p.shellHi, 10, 12, 18);
  t.titleBar26 = lift(p.shell, 6, 7, 9);
  t.titleBar62 = lift(p.shellMid, 3, 5, 6);
  t.titleBar100 = lift(p.shellLo, 0, 1, 2);
  t.wordmark = lift(p.ink, 2, 8, 15);
  t.windowName = withAlpha(lift(p.ink, -32, -20, -5), 0x8C);
  t.coolSheen = lift(p.ink, -6, 2, 15);
  t.logoDisc = lift(p.ink, 1, 2, 4);
  t.wbtn0 = lift(p.shellHi, 19, 22, 28);
  t.wbtn55 = lift(p.shell, 9, 10, 11);
  t.wbtn100 = lift(p.shellMid, 6, 7, 8);
  t.wbtnClose0 = scaleColor(p.accent, 0.6118, 0.6885, 0.6234);
  t.wbtnClose55 = scaleColor(p.accent, 0.4745, 0.5246, 0.4805);
  t.wbtnClose100 = scaleColor(p.accent, 0.2902, 0.2787, 0.2662);
  t.glyphInk = withAlpha(lift(p.ink, -18, -8, 5), 0xD1);
  t.closeGlyph = tintTowardWhite(p.accent, 41.0 / 194.0, 23.0 / 101.0);
  t.bevelLight = withAlpha(t.coolSheen, int(std::round(m.bevelLightOpacity * 255)));
  t.bevelSoft = withAlpha(t.coolSheen, int(std::round(m.bevelSoftOpacity * 255)));
  t.btnIdle0 = lift(p.shellHi, 13, 15, 19);
  t.btnIdle48 = lift(p.shell, 5, 6, 6);
  t.btnIdle100 = lift(p.shellMid, 4, 5, 6);
  t.btnOn0 = lift(p.phosHot, -15, -2, 0);
  t.btnOn1 = p.phos;
  t.btnOn2 = lift(p.phosDim, -8, 21, 32);
  t.btnOnInk = scaleColor(p.phosDeep, 0.3077, 0.5574, 0.6143);
  t.btnOnLip = lift(p.phosHot, 56, 7, 0);
  t.btnOnFoot = lift(p.phosDeep, -8, 9, 18);
  t.btnLabelIdle = withAlpha(lift(p.ink, -36, -24, -8), 0xB8);
  t.sliderFillHi = lift(p.phosHot, 19, 3, 0);
  t.sliderFillLo = lift(p.phosDim, -11, 5, 14);
  t.plateFace = lift(p.shellMid, 4, 5, 6);
  t.metalHi = lift(p.shellHi, 49, 49, 50);
  t.metalMid = lift(p.shell, 4, 5, 4);
  t.metalLo = t.plateFace;
  t.eqThumbHi = lift(p.shellHi, 67, 69, 75);
  t.scrollThumbHi = lift(p.shellHi, 75, 77, 82);
  t.scrollThumbMid = lift(p.shell, 33, 35, 36);
  t.hoverLift = lift(p.ink, 0, 6, 15);
  t.idleLedHi = lift(p.shell, 23, 24, 24);
  t.idleLedLo = lift(p.shellMid, 8, 9, 9);
  t.accentHot = tintTowardWhite(p.accent, 41.0 / 194.0, 21.0 / 101.0);
  t.litLedRim = lift(p.accentDim, -56, -19, -38);
  t.curveStroke = lift(p.phos, 80, 11, 0);
  t.screenWash0 = lift(p.well, 10, 22, 34);
  t.screenWash1 = lift(p.well, 2, 10, 16);
  t.screenWash2 = lift(p.well, -1, 1, 4);
  t.spectrumStops = m.spectrumStops;
  t.railStops = m.railStops;
  return t;
}

const ChromeTokens& ChromeTokens::builtin() {
  static const ChromeTokens cached = ChromeTokens::from(resolveLook(QStringLiteral("builtin"), {}));
  return cached;
}

QLinearGradient ChromeTokens::spectrumGradient(QPointF from, QPointF to) const {
  QLinearGradient g(from, to);
  const QVector<QColor>& stops = spectrumStops;
  if (stops.isEmpty()) {
    g.setColorAt(0, phos);
    return g;
  }
  if (stops.size() == 4) {
    g.setColorAt(0, stops[0]);
    g.setColorAt(0.26, stops[1]);
    g.setColorAt(0.62, stops[2]);
    g.setColorAt(1, stops[3]);
    return g;
  }
  if (stops.size() == 1) {
    g.setColorAt(0, stops[0]);
    g.setColorAt(1, stops[0]);
    return g;
  }
  for (int i = 0; i < stops.size(); ++i) {
    g.setColorAt(double(i) / double(stops.size() - 1), stops[i]);
  }
  return g;
}

QString lookManifestPath(const QString& packDir) {
  const QString skin = QDir(packDir).filePath(QStringLiteral("skin.json"));
  if (QFileInfo::exists(skin)) return skin;
  const QString look = QDir(packDir).filePath(QStringLiteral("look.json"));
  if (QFileInfo::exists(look)) return look;
  return {};
}

QString defaultSkinsDirectory(const QString& supportDir) {
  return QDir(supportDir).filePath(QStringLiteral("skins"));
}

LookCatalogResult scanLookCatalog(const QString& skinsDir) {
  LookCatalogResult result;
  QDir dir(skinsDir);
  if (!dir.exists()) return result;
  const QFileInfoList packs =
      dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const QFileInfo& pack : packs) {
    const QString folderName = pack.fileName();
    const QString manifestPath = lookManifestPath(pack.absoluteFilePath());
    if (manifestPath.isEmpty()) continue;
    try {
      LookManifest manifest = readManifestFile(manifestPath);
      if (manifest.id != folderName) {
        result.warnings.push_back(QStringLiteral("Skin folder \"%1\" id mismatch: manifest id \"%2\"")
                                      .arg(folderName, manifest.id));
        continue;
      }
      result.manifests.push_back(manifest);
    } catch (const LookError& e) {
      result.warnings.push_back(QStringLiteral("Skin \"%1\": %2").arg(folderName, e.message()));
    } catch (...) {
      result.warnings.push_back(QStringLiteral("Skin \"%1\": failed to read manifest").arg(folderName));
    }
  }
  return result;
}

void SkinController::bootstrap(const QString& supportDir, const QString& bundledSkinsDir,
                               TrampSettings& settings) {
  supportDir_ = supportDir;
  bundledDir_ = bundledSkinsDir;
  skinsDir_ = settings.skinsDirectory.isEmpty() ? defaultSkinsDirectory(supportDir)
                                                : settings.skinsDirectory;
  QDir().mkpath(skinsDir_);
  if (settings.skinsDirectory.isEmpty()) seedBundled();
  rescan();
  const QString requested = canonicalActiveSkinId(settings.activeSkinId);
  const bool available =
      requested == QLatin1String("builtin") || findInstalled(requested) != nullptr;
  activateInternal(available ? requested : QStringLiteral("builtin"), settings);
}

void SkinController::rescan() {
  QVector<LookManifest> merged;
  auto addAll = [&](const LookCatalogResult& scan) {
    for (const LookManifest& m : scan.manifests) {
      bool exists = false;
      for (LookManifest& have : merged) {
        if (have.id == m.id) {
          have = m;
          exists = true;
          break;
        }
      }
      if (!exists) merged.push_back(m);
    }
  };
  if (!bundledDir_.isEmpty()) addAll(scanLookCatalog(bundledDir_));
  addAll(scanLookCatalog(skinsDir_));
  installed_ = merged;
}

LookManifest* SkinController::findInstalled(const QString& id) {
  for (LookManifest& m : installed_) {
    if (m.id == id) return &m;
  }
  return nullptr;
}

const LookManifest* SkinController::findInstalled(const QString& id) const {
  for (const LookManifest& m : installed_) {
    if (m.id == id) return &m;
  }
  return nullptr;
}

void SkinController::seedBundled() {
  if (bundledDir_.isEmpty() || bundledDir_ == skinsDir_) return;
  for (const QString& retired : kRetiredBundledIds) {
    QDir(QDir(skinsDir_).filePath(retired)).removeRecursively();
  }
  const LookCatalogResult bundled = scanLookCatalog(bundledDir_);
  for (const LookManifest& m : bundled.manifests) {
    const QString dest = QDir(skinsDir_).filePath(m.id);
    if (QDir(dest).exists()) continue;
    copyDir(m.packRoot, dest);
  }
}

bool SkinController::activate(const QString& id, TrampSettings& settings) {
  return activateInternal(canonicalActiveSkinId(id), settings);
}

bool SkinController::activateInternal(const QString& id, TrampSettings& settings) {
  try {
    resolved_ = resolveLook(id, installed_);
    applyFonts(id);
    tokens_ = ChromeTokens::from(resolved_);
    settings.activeSkinId = id;
    lastError_.clear();
    setLookFamilies(resolved_.chromeFamily, resolved_.lcdFamily);
    return true;
  } catch (const LookError& e) {
    lastError_ = e.message();
    return false;
  } catch (const std::exception& e) {
    lastError_ = QString::fromUtf8(e.what());
    return false;
  }
}

void SkinController::applyFonts(const QString& id) {
  for (int fontId : loadedFontIds_) {
    QFontDatabase::removeApplicationFont(fontId);
  }
  loadedFontIds_.clear();
  resolved_.chromeFamily.clear();
  resolved_.lcdFamily.clear();

  QVector<LookManifest> chain;
  QString current = id;
  QStringList visited;
  while (true) {
    if (visited.contains(current) || chain.size() >= kMaxChain) break;
    visited.push_back(current);
    if (current == QLatin1String("builtin")) {
      chain.push_back(builtinLookManifest());
      break;
    }
    const LookManifest* found = findInstalled(current);
    if (!found) break;
    chain.push_back(*found);
    current = found->extendsId;
  }

  auto nearest = [&](const QString& role) -> const LookManifest* {
    for (const LookManifest& m : chain) {
      if (m.fonts.contains(role) && m.fonts.value(role).isObject()) return &m;
    }
    return nullptr;
  };

  for (const char* roleRaw : {"chrome", "lcd"}) {
    const QString role = QString::fromLatin1(roleRaw);
    const LookManifest* pack = nearest(role);
    if (!pack || pack->id == QLatin1String("builtin") || pack->packRoot.isEmpty()) continue;
    const QJsonObject font = pack->fonts.value(role).toObject();
    const QString rel = font.value(QStringLiteral("file")).toString();
    const QString filePath = QDir::cleanPath(QDir(pack->packRoot).filePath(rel));
    const QString root = QDir(pack->packRoot).canonicalPath();
    const QString canonical = QFileInfo(filePath).canonicalFilePath();
    if (canonical.isEmpty() || !canonical.startsWith(root)) continue;
    if (!QFileInfo::exists(canonical)) continue;
    const int fontId = QFontDatabase::addApplicationFont(canonical);
    if (fontId < 0) continue;
    loadedFontIds_.push_back(fontId);
    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) continue;
    if (role == QLatin1String("chrome")) resolved_.chromeFamily = families.front();
    else resolved_.lcdFamily = families.front();
  }
}

QVector<SkinCatalogEntry> SkinController::catalog() const {
  QVector<SkinCatalogEntry> entries;
  const LookManifest builtin = builtinLookManifest();
  entries.push_back({builtin.id, builtin.name, builtin.author});
  QSet<QString> seen{builtin.id};
  auto add = [&](const LookManifest& m) {
    if (seen.contains(m.id) || kRetiredBundledIds.contains(m.id)) return;
    seen.insert(m.id);
    entries.push_back({m.id, m.name, m.author});
  };
  for (const QString& id : kBundledCatalogOrder) {
    if (const LookManifest* m = findInstalled(id)) add(*m);
  }
  for (const LookManifest& m : installed_) add(m);
  return entries;
}

void SkinController::setSkinsDirectory(const QString& path, TrampSettings& settings) {
  settings.skinsDirectory = path;
  skinsDir_ = path.isEmpty() ? defaultSkinsDirectory(supportDir_) : path;
  QDir().mkpath(skinsDir_);
  if (path.isEmpty()) seedBundled();
  rescan();
  const QString active = canonicalActiveSkinId(settings.activeSkinId);
  if (active != QLatin1String("builtin") && findInstalled(active) == nullptr) {
    activateInternal(QStringLiteral("builtin"), settings);
    return;
  }
  activateInternal(active, settings);
}

bool SkinController::installDirectory(const QString& path, ConflictFn onConflict) {
  try {
    const QString manifestPath = lookManifestPath(path);
    if (manifestPath.isEmpty()) {
      lastError_ = QStringLiteral("skin.json / look.json not found in ") + path;
      return false;
    }
    LookManifest incoming = readManifestFile(manifestPath);
    const QString target = QDir(skinsDir_).filePath(incoming.id);
    if (QDir(target).exists()) {
      SkinConflict conflict;
      conflict.id = incoming.id;
      conflict.incomingName = incoming.name;
      conflict.incomingAuthor = incoming.author;
      conflict.installedName = incoming.id;
      try {
        const LookManifest existing = readManifestFile(lookManifestPath(target));
        conflict.installedName = existing.name;
        conflict.installedAuthor = existing.author;
      } catch (...) {
      }
      if (onConflict && onConflict(conflict) == SkinConflictChoice::cancel) {
        return false;
      }
      QDir(target).removeRecursively();
    }
    QDir().mkpath(skinsDir_);
    if (!copyDir(path, target)) {
      lastError_ = QStringLiteral("failed to copy skin pack");
      return false;
    }
    lastError_.clear();
    rescan();
    return true;
  } catch (const LookError& e) {
    lastError_ = e.message();
    return false;
  }
}

bool SkinController::installZip(const QString& path, ConflictFn onConflict) {
  QTemporaryDir temp(QDir::temp().filePath(QStringLiteral("tramp-skin-XXXXXX")));
  if (!temp.isValid()) {
    lastError_ = QStringLiteral("failed to create temp directory");
    return false;
  }
  QProcess unzip;
  unzip.setProgram(QStringLiteral("unzip"));
  unzip.setArguments({QStringLiteral("-oq"), path, QStringLiteral("-d"), temp.path()});
  unzip.start();
  if (!unzip.waitForFinished(30000) || unzip.exitCode() != 0) {
    lastError_ = QStringLiteral("unzip is required to install zip skins");
    return false;
  }
  QString root = temp.path();
  const QString nestedSkin = lookManifestPath(root);
  if (nestedSkin.isEmpty()) {
    const QFileInfoList dirs =
        QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& d : dirs) {
      if (!lookManifestPath(d.absoluteFilePath()).isEmpty()) {
        root = d.absoluteFilePath();
        break;
      }
    }
  }
  return installDirectory(root, std::move(onConflict));
}

}  // namespace tramp
