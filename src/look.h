#pragma once

#include "settings.h"
#include "tramp_metrics.h"

#include <QColor>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>
#include <stdexcept>

namespace tramp {

class LookError : public std::runtime_error {
 public:
  explicit LookError(const QString& message)
      : std::runtime_error(message.toStdString()), message_(message) {}
  QString message() const { return message_; }

 private:
  QString message_;
};

struct LookManifest {
  int formatVersion = 1;
  QString id;
  QString name;
  QString author;
  QString extendsId;
  QJsonObject colors;
  QJsonObject materials;
  QJsonObject fonts;
  QJsonObject radii;
  QString packRoot;
};

struct LookPalette {
  QColor shellHi;
  QColor shell;
  QColor shellMid;
  QColor shellLo;
  QColor shellDeep;
  QColor ink;
  QColor inkDim;
  QColor inkFaint;
  QColor phos;
  QColor phosHot;
  QColor phosDim;
  QColor phosDeep;
  QColor accent;
  QColor accentDim;
  QColor well;
};

struct LookMaterials {
  qreal bevelLightOpacity = 0.15;
  qreal bevelSoftOpacity = 0.06;
  QVector<QColor> spectrumStops;
  QVector<QColor> railStops;
};

struct LookRadii {
  qreal window = kShellRadius;
  qreal surface = kWellRadius;
  qreal button = kButtonRadius;
};

struct ResolvedLook {
  QString id;
  QString name;
  QString author;
  LookPalette palette;
  LookMaterials materials;
  LookRadii radii;
  QString chromeFamily;
  QString lcdFamily;
};

struct SkinCatalogEntry {
  QString id;
  QString name;
  QString author;
  QString previewPath;
  bool canRemove = false;

  SkinCatalogEntry() = default;
  SkinCatalogEntry(QString id_, QString name_, QString author_ = {})
      : id(id_), name(name_), author(author_) {}
};

/// Bump when the golden main shot or its paint path changes, so cached thumbs
/// rebuild instead of showing a player Tramp no longer draws.
inline constexpr int kSkinPreviewGeneration = 6;

using SkinPreviewWriter = std::function<bool(const QString& id, const QString& path,
                                             const QVector<LookManifest>& installed,
                                             QString* error)>;

struct LoadedSkinFonts {
  QVector<int> ids;
  QString chromeFamily;
  QString lcdFamily;
  void unload();
};

LoadedSkinFonts loadSkinFonts(const QString& id, const QVector<LookManifest>& installed);
bool isBundledHomageId(const QString& id);

struct ChromeTokens {
  QString id = QStringLiteral("builtin");
  LookPalette palette{};
  LookMaterials materials{};
  LookRadii radii{};
  QString chromeFamily;
  QString lcdFamily;

  qreal windowRadius(const QRectF& r) const;
  qreal surfaceRadius(const QRectF& r) const;
  qreal buttonRadius(const QRectF& r) const;

  QColor shellHi;
  QColor shell;
  QColor shellMid;
  QColor shellLo;
  QColor shellDeep;
  QColor ink;
  QColor inkDim;
  QColor inkFaint;
  QColor phos;
  QColor phosHot;
  QColor phosDim;
  QColor phosDeep;
  QColor accent;
  QColor accentDim;
  QColor well;
  QColor titleBar0;
  QColor titleBar26;
  QColor titleBar62;
  QColor titleBar100;
  QColor wordmark;
  QColor windowName;
  QColor coolSheen;
  QColor logoDisc;
  QColor wbtn0;
  QColor wbtn55;
  QColor wbtn100;
  QColor wbtnClose0;
  QColor wbtnClose55;
  QColor wbtnClose100;
  QColor glyphInk;
  QColor closeGlyph;
  QColor bevelLight;
  QColor bevelSoft;
  QColor btnIdle0;
  QColor btnIdle48;
  QColor btnIdle100;
  QColor btnOn0;
  QColor btnOn1;
  QColor btnOn2;
  QColor btnOnInk;
  QColor btnOnLip;
  QColor btnOnFoot;
  QColor btnLabelIdle;
  QColor sliderFillHi;
  QColor sliderFillLo;
  QColor plateFace;
  QColor metalHi;
  QColor metalMid;
  QColor metalLo;
  QColor eqThumbHi;
  QColor scrollThumbHi;
  QColor scrollThumbMid;
  QColor hoverLift;
  QColor idleLedHi;
  QColor idleLedLo;
  QColor accentHot;
  QColor litLedRim;
  QColor curveStroke;
  QColor screenWash0;
  QColor screenWash1;
  QColor screenWash2;
  QColor listWash0;
  QColor listWash1;
  QColor listWash2;
  QColor listSheen;
  QVector<QColor> spectrumStops;
  QVector<QColor> railStops;

  static ChromeTokens from(const ResolvedLook& look);
  static const ChromeTokens& builtin();

  QLinearGradient spectrumGradient(QPointF from, QPointF to) const;
};

inline QColor withAlpha(const QColor& c, int alpha) {
  return QColor(c.red(), c.green(), c.blue(), alpha);
}

/// Straight channel blend, [t] 0 = [a], 1 = [b]. Chrome that transitions between
/// two token colours mixes them here rather than fading one over the other, so a
/// half-lit button is one opaque face and not two stacked translucent ones.
inline QColor mix(const QColor& a, const QColor& b, qreal t) {
  const qreal k = t < 0 ? 0 : (t > 1 ? 1 : t);
  auto lerp = [k](int from, int to) { return int(qRound(from + (to - from) * k)); };
  return QColor(lerp(a.red(), b.red()), lerp(a.green(), b.green()), lerp(a.blue(), b.blue()),
                lerp(a.alpha(), b.alpha()));
}

/// Multiply a colour's brightness, keeping alpha. Used for the small lift and
/// sink that mark hover and press without introducing new tokens per skin.
inline QColor scaled(const QColor& c, qreal factor) {
  auto ch = [factor](int v) { return int(qBound(0, qRound(v * factor), 255)); };
  return QColor(ch(c.red()), ch(c.green()), ch(c.blue()), c.alpha());
}

const ChromeTokens& currentLook();

class LookPaintScope {
 public:
  explicit LookPaintScope(const ChromeTokens& tokens);
  ~LookPaintScope();
  LookPaintScope(const LookPaintScope&) = delete;
  LookPaintScope& operator=(const LookPaintScope&) = delete;

 private:
  const ChromeTokens* prev_;
};

LookManifest parseLookManifest(const QJsonObject& json, bool allowBuiltin = false);
LookManifest builtinLookManifest();
ResolvedLook resolveLook(const QString& activeId,
                         const QVector<LookManifest>& installed);
LookPalette paletteFromColors(const QJsonObject& colors);
LookMaterials materialsFromJson(const QJsonObject& materials);
LookRadii radiiFromJson(const QJsonObject& radii);

struct LookCatalogResult {
  QVector<LookManifest> manifests;
  QStringList warnings;
};

LookCatalogResult scanLookCatalog(const QString& skinsDir);
QString defaultSkinsDirectory(const QString& supportDir);
QString lookManifestPath(const QString& packDir);

struct SkinConflict {
  QString id;
  QString installedName;
  QString installedAuthor;
  QString incomingName;
  QString incomingAuthor;
};

enum class SkinConflictChoice { replace, cancel };

class SkinController {
 public:
  using ConflictFn = std::function<SkinConflictChoice(const SkinConflict&)>;

  void bootstrap(const QString& supportDir, const QString& bundledSkinsDir,
                 TrampSettings& settings);
  bool activate(const QString& id, TrampSettings& settings);
  bool installDirectory(const QString& path, ConflictFn onConflict);
  bool installZip(const QString& path, ConflictFn onConflict);
  void setSkinsDirectory(const QString& path, TrampSettings& settings);
  void rescan();
  bool remove(const QString& id, const TrampSettings& settings);
  void ensurePreviews(const SkinPreviewWriter& write);
  QString previewPath(const QString& id) const;

  const ChromeTokens& tokens() const { return tokens_; }
  const ResolvedLook& resolved() const { return resolved_; }
  QVector<SkinCatalogEntry> catalog() const;
  QString lastError() const { return lastError_; }
  QString skinsDirectory() const { return skinsDir_; }

 private:
  bool activateInternal(const QString& id, TrampSettings& settings);
  void applyFonts(const QString& id);
  void seedBundled();
  LookManifest* findInstalled(const QString& id);
  const LookManifest* findInstalled(const QString& id) const;
  bool canRemoveId(const QString& id, const QString& activeId) const;
  QString previewDir() const;
  int readPreviewGeneration() const;
  void writePreviewGeneration() const;
  bool previewFileStale(const QString& id) const;

  QString supportDir_;
  QString bundledDir_;
  QString skinsDir_;
  QVector<LookManifest> installed_;
  ResolvedLook resolved_;
  ChromeTokens tokens_ = ChromeTokens::builtin();
  QString lastError_;
  QVector<int> loadedFontIds_;
};

}  // namespace tramp
