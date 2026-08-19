#pragma once

#include "settings.h"

#include <QColor>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPointF>
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

struct ResolvedLook {
  QString id;
  QString name;
  QString author;
  LookPalette palette;
  LookMaterials materials;
  QString chromeFamily;
  QString lcdFamily;
};

struct SkinCatalogEntry {
  QString id;
  QString name;
  QString author;
};

struct ChromeTokens {
  QString id = QStringLiteral("builtin");
  LookPalette palette{};
  LookMaterials materials{};
  QString chromeFamily;
  QString lcdFamily;

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
