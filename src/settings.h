#pragma once

#include "equalizer.h"
#include "window_spec.h"

#include <QJsonObject>
#include <QString>
#include <QVector>
#include <optional>

namespace tramp {

enum class DockSide { left, right, top, bottom };
enum class DockSnapStrength { off, normal, strong };

inline double snapPixels(DockSnapStrength s) {
  switch (s) {
    case DockSnapStrength::off:
      return 0;
    case DockSnapStrength::normal:
      return 20;
    case DockSnapStrength::strong:
      return 40;
  }
  return 20;
}

struct WindowFrame {
  bool visible = true;
  bool shaded = false;
  double left = 0;
  double top = 0;
  std::optional<double> width;
  std::optional<double> height;

  static WindowFrame mainDefault() { return {true, false, 0, 0, {}, {}}; }
  static WindowFrame equalizerDefault() { return {true, false, 0, 348, {}, {}}; }
  static WindowFrame playlistDefault() { return {true, false, 0, 696, {}, {}}; }
  static WindowFrame settingsDefault() { return {false, false, 860, 40, {}, {}}; }
  static WindowFrame aboutDefault() { return {false, false, 860, 480, {}, {}}; }
  static WindowFrame skinsDefault() { return {false, false, 1340, 40, {}, {}}; }
};

struct DockEdge {
  WindowId a = WindowId::main;
  WindowId b = WindowId::equalizer;
  DockSide side = DockSide::bottom;
};

struct TrampSettings {
  int zoomPercent = 75;
  bool alwaysOnTop = false;
  bool forceMono = false;
  WindowFrame main = WindowFrame::mainDefault();
  WindowFrame equalizer = WindowFrame::equalizerDefault();
  WindowFrame playlist = WindowFrame::playlistDefault();
  WindowFrame settings = WindowFrame::settingsDefault();
  WindowFrame about = WindowFrame::aboutDefault();
  WindowFrame skins = WindowFrame::skinsDefault();
  QVector<DockEdge> dockEdges;
  EqualizerSettings equalizerCurve;
  QString activeSkinId = QStringLiteral("builtin");
  QString skinsDirectory;
  bool resumeLastSession = true;
  bool confirmBeforeQuit = false;
  bool scrollTitle = true;
  bool showElapsed = true;
  bool minimizeHidesSecondaries = true;
  DockSnapStrength dockSnapStrength = DockSnapStrength::normal;
  double playlistCollectionWidth = 240;
  bool playlistCollectionCollapsed = false;
  QString audioDevice;
  bool audioExclusive = false;

  QJsonObject toJson() const;
  static TrampSettings fromJson(const QJsonObject& json);
};

/// Factory settings, keeping the active skin. The skins directory goes back
/// with the rest; the session re-seeds the bundled packs when it applies this.
inline void resetSettingsExceptSkins(TrampSettings& s) {
  const QString skin = s.activeSkinId;
  s = TrampSettings{};
  s.activeSkinId = skin;
}

WindowFrame* frameFor(TrampSettings& s, WindowId id);
const WindowFrame* frameFor(const TrampSettings& s, WindowId id);

}  // namespace tramp
