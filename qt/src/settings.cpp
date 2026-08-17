#include "settings.h"

#include "tramp_metrics.h"

#include <QJsonArray>

namespace tramp {
namespace {

QString windowName(WindowId id) {
  switch (id) {
    case WindowId::main:
      return QStringLiteral("main");
    case WindowId::equalizer:
      return QStringLiteral("equalizer");
    case WindowId::playlist:
      return QStringLiteral("playlist");
    case WindowId::settings:
      return QStringLiteral("settings");
    case WindowId::about:
      return QStringLiteral("about");
  }
  return QStringLiteral("main");
}

std::optional<WindowId> parseWindowId(const QString& name) {
  if (name == QLatin1String("main")) return WindowId::main;
  if (name == QLatin1String("equalizer")) return WindowId::equalizer;
  if (name == QLatin1String("playlist")) return WindowId::playlist;
  if (name == QLatin1String("settings")) return WindowId::settings;
  if (name == QLatin1String("about")) return WindowId::about;
  return std::nullopt;
}

QString sideName(DockSide side) {
  switch (side) {
    case DockSide::left:
      return QStringLiteral("left");
    case DockSide::right:
      return QStringLiteral("right");
    case DockSide::top:
      return QStringLiteral("top");
    case DockSide::bottom:
      return QStringLiteral("bottom");
  }
  return QStringLiteral("bottom");
}

std::optional<DockSide> parseSide(const QString& name) {
  if (name == QLatin1String("left")) return DockSide::left;
  if (name == QLatin1String("right")) return DockSide::right;
  if (name == QLatin1String("top")) return DockSide::top;
  if (name == QLatin1String("bottom")) return DockSide::bottom;
  return std::nullopt;
}

QJsonObject frameToJson(const WindowFrame& f) {
  QJsonObject o;
  o.insert(QStringLiteral("visible"), f.visible);
  o.insert(QStringLiteral("shaded"), f.shaded);
  o.insert(QStringLiteral("left"), f.left);
  o.insert(QStringLiteral("top"), f.top);
  if (f.width) o.insert(QStringLiteral("width"), *f.width);
  if (f.height) o.insert(QStringLiteral("height"), *f.height);
  return o;
}

WindowFrame frameFromJson(const QJsonObject& o, const WindowFrame& fallback) {
  WindowFrame f = fallback;
  if (o.contains(QStringLiteral("visible")) && o.value(QStringLiteral("visible")).isBool()) {
    f.visible = o.value(QStringLiteral("visible")).toBool();
  }
  if (o.contains(QStringLiteral("shaded")) && o.value(QStringLiteral("shaded")).isBool()) {
    f.shaded = o.value(QStringLiteral("shaded")).toBool();
  }
  if (o.value(QStringLiteral("left")).isDouble()) {
    f.left = o.value(QStringLiteral("left")).toDouble();
  }
  if (o.value(QStringLiteral("top")).isDouble()) {
    f.top = o.value(QStringLiteral("top")).toDouble();
  }
  if (o.value(QStringLiteral("width")).isDouble()) {
    const double w = o.value(QStringLiteral("width")).toDouble();
    if (w > 0) f.width = w;
  }
  if (o.value(QStringLiteral("height")).isDouble()) {
    const double h = o.value(QStringLiteral("height")).toDouble();
    if (h > 0) f.height = h;
  }
  return f;
}

bool looksLikeFrame(const QJsonObject& o) {
  return o.contains(QStringLiteral("visible")) || o.contains(QStringLiteral("shaded")) ||
         o.contains(QStringLiteral("left")) || o.contains(QStringLiteral("top"));
}

bool looksLikeCurve(const QJsonObject& o) {
  return o.contains(QStringLiteral("gains")) || o.contains(QStringLiteral("enabled")) ||
         o.contains(QStringLiteral("preamp"));
}

EqualizerSettings curveFromJson(const QJsonObject& o) {
  const QJsonArray raw = o.value(QStringLiteral("gains")).toArray();
  if (raw.size() != EqualizerSettings::kBandCount) {
    return EqualizerSettings::flat();
  }
  EqualizerSettings s;
  s.enabled = o.value(QStringLiteral("enabled")).toBool();
  s.auto_ = o.value(QStringLiteral("auto")).toBool();
  s.preamp = EqualizerSettings::clampGain(o.value(QStringLiteral("preamp")).toDouble());
  for (int i = 0; i < EqualizerSettings::kBandCount; ++i) {
    if (!raw.at(i).isDouble()) return EqualizerSettings::flat();
    s.gains[size_t(i)] = EqualizerSettings::clampGain(raw.at(i).toDouble());
  }
  const QString preset = o.value(QStringLiteral("presetName")).toString();
  if (!preset.isEmpty()) s.presetName = preset;
  return s;
}

QJsonObject curveToJson(const EqualizerSettings& s) {
  QJsonObject o;
  o.insert(QStringLiteral("enabled"), s.enabled);
  o.insert(QStringLiteral("auto"), s.auto_);
  o.insert(QStringLiteral("preamp"), s.preamp);
  QJsonArray gains;
  for (double g : s.gains) {
    gains.append(g);
  }
  o.insert(QStringLiteral("gains"), gains);
  if (!s.presetName.isEmpty()) {
    o.insert(QStringLiteral("presetName"), s.presetName);
  }
  return o;
}

}  // namespace

WindowFrame* frameFor(TrampSettings& s, WindowId id) {
  switch (id) {
    case WindowId::main:
      return &s.main;
    case WindowId::equalizer:
      return &s.equalizer;
    case WindowId::playlist:
      return &s.playlist;
    case WindowId::settings:
      return &s.settings;
    case WindowId::about:
      return &s.about;
  }
  return &s.main;
}

const WindowFrame* frameFor(const TrampSettings& s, WindowId id) {
  return frameFor(const_cast<TrampSettings&>(s), id);
}

QJsonObject TrampSettings::toJson() const {
  QJsonObject o;
  o.insert(QStringLiteral("zoomPercent"), zoomPercent);
  o.insert(QStringLiteral("alwaysOnTop"), alwaysOnTop);
  o.insert(QStringLiteral("forceMono"), forceMono);
  o.insert(QStringLiteral("main"), frameToJson(main));
  o.insert(QStringLiteral("equalizer"), frameToJson(equalizer));
  o.insert(QStringLiteral("playlist"), frameToJson(playlist));
  o.insert(QStringLiteral("settings"), frameToJson(settings));
  o.insert(QStringLiteral("about"), frameToJson(about));
  QJsonArray edges;
  for (const DockEdge& e : dockEdges) {
    QJsonObject edge;
    edge.insert(QStringLiteral("a"), windowName(e.a));
    edge.insert(QStringLiteral("b"), windowName(e.b));
    edge.insert(QStringLiteral("side"), sideName(e.side));
    edges.append(edge);
  }
  o.insert(QStringLiteral("dockEdges"), edges);
  o.insert(QStringLiteral("equalizerCurve"), curveToJson(equalizerCurve));
  o.insert(QStringLiteral("activeSkinId"), activeSkinId);
  if (!skinsDirectory.isEmpty()) {
    o.insert(QStringLiteral("skinsDirectory"), skinsDirectory);
  }
  o.insert(QStringLiteral("resumeLastSession"), resumeLastSession);
  o.insert(QStringLiteral("confirmBeforeQuit"), confirmBeforeQuit);
  o.insert(QStringLiteral("scrollTitle"), scrollTitle);
  o.insert(QStringLiteral("minimizeHidesSecondaries"), minimizeHidesSecondaries);
  QString snap = QStringLiteral("normal");
  if (dockSnapStrength == DockSnapStrength::off) snap = QStringLiteral("off");
  if (dockSnapStrength == DockSnapStrength::strong) snap = QStringLiteral("strong");
  o.insert(QStringLiteral("dockSnapStrength"), snap);
  o.insert(QStringLiteral("playlistCollectionWidth"), playlistCollectionWidth);
  o.insert(QStringLiteral("playlistCollectionCollapsed"), playlistCollectionCollapsed);
  return o;
}

TrampSettings TrampSettings::fromJson(const QJsonObject& json) {
  TrampSettings s;
  const int zoom = json.value(QStringLiteral("zoomPercent")).toInt(s.zoomPercent);
  bool zoomOk = false;
  for (int step : kZoomSteps) {
    if (step == zoom) zoomOk = true;
  }
  if (zoomOk) s.zoomPercent = zoom;
  if (json.value(QStringLiteral("alwaysOnTop")).isBool()) {
    s.alwaysOnTop = json.value(QStringLiteral("alwaysOnTop")).toBool();
  }
  if (json.value(QStringLiteral("forceMono")).isBool()) {
    s.forceMono = json.value(QStringLiteral("forceMono")).toBool();
  }
  if (json.value(QStringLiteral("main")).isObject()) {
    s.main = frameFromJson(json.value(QStringLiteral("main")).toObject(), s.main);
  }
  const QJsonObject eq = json.value(QStringLiteral("equalizer")).toObject();
  if (!eq.isEmpty() && looksLikeFrame(eq)) {
    s.equalizer = frameFromJson(eq, s.equalizer);
  }
  if (json.value(QStringLiteral("playlist")).isObject()) {
    s.playlist = frameFromJson(json.value(QStringLiteral("playlist")).toObject(), s.playlist);
  }
  if (json.value(QStringLiteral("settings")).isObject()) {
    s.settings = frameFromJson(json.value(QStringLiteral("settings")).toObject(), s.settings);
  }
  if (json.value(QStringLiteral("about")).isObject()) {
    s.about = frameFromJson(json.value(QStringLiteral("about")).toObject(), s.about);
  }
  const QJsonObject namedCurve = json.value(QStringLiteral("equalizerCurve")).toObject();
  if (!namedCurve.isEmpty()) {
    s.equalizerCurve = curveFromJson(namedCurve);
  } else if (!eq.isEmpty() && looksLikeCurve(eq)) {
    s.equalizerCurve = curveFromJson(eq);
  }
  if (json.value(QStringLiteral("resumeLastSession")).isBool()) {
    s.resumeLastSession = json.value(QStringLiteral("resumeLastSession")).toBool();
  }
  if (json.value(QStringLiteral("confirmBeforeQuit")).isBool()) {
    s.confirmBeforeQuit = json.value(QStringLiteral("confirmBeforeQuit")).toBool();
  }
  if (json.value(QStringLiteral("scrollTitle")).isBool()) {
    s.scrollTitle = json.value(QStringLiteral("scrollTitle")).toBool();
  }
  if (json.value(QStringLiteral("minimizeHidesSecondaries")).isBool()) {
    s.minimizeHidesSecondaries = json.value(QStringLiteral("minimizeHidesSecondaries")).toBool();
  }
  const QString snap = json.value(QStringLiteral("dockSnapStrength")).toString();
  if (snap == QLatin1String("off")) s.dockSnapStrength = DockSnapStrength::off;
  if (snap == QLatin1String("strong")) s.dockSnapStrength = DockSnapStrength::strong;
  if (snap == QLatin1String("normal")) s.dockSnapStrength = DockSnapStrength::normal;
  if (json.value(QStringLiteral("playlistCollectionWidth")).isDouble()) {
    const double w = json.value(QStringLiteral("playlistCollectionWidth")).toDouble();
    if (w > 0) s.playlistCollectionWidth = w;
  }
  if (json.value(QStringLiteral("playlistCollectionCollapsed")).isBool()) {
    s.playlistCollectionCollapsed = json.value(QStringLiteral("playlistCollectionCollapsed")).toBool();
  }
  const QString skin = json.value(QStringLiteral("activeSkinId")).toString();
  if (!skin.isEmpty()) s.activeSkinId = skin;
  s.skinsDirectory = json.value(QStringLiteral("skinsDirectory")).toString();
  if (s.skinsDirectory.isEmpty()) {
    s.skinsDirectory = json.value(QStringLiteral("looksDirectory")).toString();
  }
  const QJsonArray edges = json.value(QStringLiteral("dockEdges")).toArray();
  for (const QJsonValue& v : edges) {
    if (!v.isObject()) continue;
    const QJsonObject o = v.toObject();
    const auto a = parseWindowId(o.value(QStringLiteral("a")).toString());
    const auto b = parseWindowId(o.value(QStringLiteral("b")).toString());
    const auto side = parseSide(o.value(QStringLiteral("side")).toString());
    if (!a || !b || !side) continue;
    s.dockEdges.push_back({*a, *b, *side});
  }
  return s;
}

}  // namespace tramp
