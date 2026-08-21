#pragma once

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTransform>
#include <QVector>
#include <functional>

namespace tramp {

/// Holds everything a painter carries for a scope and puts it back. Pairing
/// [QPainter::save] with [QPainter::restore] by hand is a footgun in any
/// painter with more than one exit — the settings pane returns early out of the
/// Skins tab — and this cannot be got wrong that way.
class PainterStateScope {
 public:
  explicit PainterStateScope(QPainter& painter) : painter_(painter) { painter_.save(); }
  ~PainterStateScope() { painter_.restore(); }
  PainterStateScope(const PainterStateScope&) = delete;
  PainterStateScope& operator=(const PainterStateScope&) = delete;

 private:
  QPainter& painter_;
};

/// A reading of everything a painter hands to whatever draws next, so the
/// contract below can be checked rather than reviewed.
///
/// The reading is the whole of [QPainter]'s state and not the three fields the
/// bug of the day happened to use. `drawStatusDot` left a `Qt::NoPen` behind
/// and cost the playlist footer three readouts, so pen, brush and font went
/// under a test — and the About plate's stray `SmoothPixmapTransform` sailed
/// straight through it, because a render hint is not any of those three. A
/// narrow check reports all-clear on the leak it was not written for.
struct PainterState {
  QPen pen;
  QBrush brush;
  QPointF brushOrigin;
  /// [QFont::toString] rather than the font, because two faces that paint the
  /// same are unequal if they were resolved through different masks.
  QString font;
  QBrush background;
  Qt::BGMode backgroundMode = Qt::TransparentMode;
  QPainter::CompositionMode composition = QPainter::CompositionMode_SourceOver;
  qreal opacity = 1;
  bool clipping = false;
  QPainterPath clip;
  QTransform transform;
  QPainter::RenderHints hints;
  Qt::LayoutDirection direction = Qt::LeftToRight;

  static PainterState of(const QPainter& painter);
  /// Names every kind of state that moved, so a failure says what leaked.
  QStringList differencesFrom(const PainterState& earlier) const;
};

inline PainterState PainterState::of(const QPainter& painter) {
  PainterState state;
  state.pen = painter.pen();
  state.brush = painter.brush();
  state.brushOrigin = painter.brushOrigin();
  state.font = painter.font().toString();
  state.background = painter.background();
  state.backgroundMode = painter.backgroundMode();
  state.composition = painter.compositionMode();
  state.opacity = painter.opacity();
  state.clipping = painter.hasClipping();
  state.clip = painter.clipPath();
  state.transform = painter.transform();
  state.hints = painter.renderHints();
  state.direction = painter.layoutDirection();
  return state;
}

inline QStringList PainterState::differencesFrom(const PainterState& earlier) const {
  QStringList moved;
  const auto note = [&](bool changed, const char* what) {
    if (changed) moved << QLatin1String(what);
  };
  note(pen != earlier.pen, "pen");
  note(brush != earlier.brush, "brush");
  note(brushOrigin != earlier.brushOrigin, "brush origin");
  note(font != earlier.font, "font");
  note(background != earlier.background, "background");
  note(backgroundMode != earlier.backgroundMode, "background mode");
  note(composition != earlier.composition, "composition mode");
  note(!qFuzzyCompare(opacity + 1, earlier.opacity + 1), "opacity");
  note(clipping != earlier.clipping || clip != earlier.clip, "clip");
  note(transform != earlier.transform, "transform");
  note(hints != earlier.hints, "render hints");
  note(direction != earlier.direction, "layout direction");
  return moved;
}

enum class MockupIcon {
  previous,
  play,
  pause,
  stop,
  next,
  eject,
  mute,
  add,
  remove,
  sort,
  options,
  minimize,
  zoomOut,
  zoomIn,
  close,
};

/// Flutter `Shadow`: sigma = blurRadius * 0.57735.
struct TextShadow {
  QColor color;
  QPointF offset{};
  qreal blurRadius = 0;
};

/// Every painter below leaves the painter as it found it — pen, brush, font,
/// render hints, clip, transform and the rest of [PainterState]. A helper that
/// did not cost the playlist footer three of its four readouts: `drawStatusDot`
/// left `Qt::NoPen` behind, and the strip draws a dot between each pair of
/// labels, so everything after the first dot was drawn with no pen and never
/// appeared. Callers set state once and call these freely.
QFont condensedFont(int px, qreal trackingEm = 0);
QFont monoFont(int px, qreal trackingEm = 0);
qreal textWidth(const QFont& font, const QString& text);

void fillRound(QPainter& p, const QRectF& r, qreal radius, const QBrush& brush);

/// CRT well (wash / rim / phosphor bloom / inner shade). Scanlines sit in
/// [drawScreenOverlay].
void drawScreenWell(QPainter& p, const QRectF& well);
void drawScreenOverlay(QPainter& p, const QRectF& well,
                       QColor scan = QColor(0, 0, 0, 82), bool glass = true);
void drawScreen(QPainter& p, const QRectF& well);
void drawListWell(QPainter& p, const QRectF& well);

/// How far a button face has travelled between its states, each 0..1. A button
/// is never simply lit or idle: `on` is the latched amount, `hover` and `press`
/// are pointer feedback, and each is walked to its target by [ChromePhases] so
/// the face cross-fades instead of snapping. The implicit bool conversion keeps
/// call sites that only care about the latched state reading as they did.
struct BtnFace {
  qreal on = 0;
  qreal hover = 0;
  qreal press = 0;

  BtnFace() = default;
  BtnFace(bool lit) : on(lit ? 1 : 0) {}  // NOLINT(google-explicit-constructor)
  BtnFace(qreal onPhase, qreal hoverPhase, qreal pressPhase)
      : on(onPhase), hover(hoverPhase), press(pressPhase) {}
};

void drawBtn(QPainter& p, const QRectF& r, BtnFace face, const QString& label = {});
qreal labelBtnWidth(const QString& label, qreal padL = 16, qreal padR = 16);
void drawIcon(QPainter& p, const QRectF& box, MockupIcon icon, const QColor& color);
void drawGlyphBtn(QPainter& p, const QRectF& r, MockupIcon icon, BtnFace face,
                  qreal iconSize = 22);
void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle = false,
                bool glow = true);
void drawVBand(QPainter& p, const QRectF& column, qreal gainDb);
/// Where [drawVBand] puts the thumb for a gain. Centred on the value point, so
/// at the ends of the range it stands proud of the well — exported so hit
/// regions can be checked against the paint rather than against a copy of it.
QRectF bandThumbRect(const QRectF& well, qreal gainDb);
void drawLed(QPainter& p, QPointF c, qreal on, qreal size = 8);
qreal toggleBtnWidth(const QString& label);
/// Label plus an indicator lamp. `face.on` lights the lamp, not the chassis:
/// this is the one button kind whose latched state lives beside the face rather
/// than in it.
void drawToggleBtn(QPainter& p, const QRectF& r, const QString& label, BtnFace face);
void drawMenuCaret(QPainter& p, const QRectF& btn);
void drawReload(QPainter& p, const QRectF& box, const QColor& color);
void drawChevron(QPainter& p, const QRectF& box, bool pointsLeft, const QColor& color);
void drawCreateMark(QPainter& p, const QRectF& box, const QColor& color);
void drawRenameMark(QPainter& p, const QRectF& box, const QColor& color);
void drawFooterSep(QPainter& p, const QRectF& r);
void drawStatusDot(QPainter& p, QPointF c);
void drawScrollbar(QPainter& p, const QRectF& track, qreal thumbTop, qreal thumbH);
void drawDiscLogo(QPainter& p, const QRectF& disc, const QImage* logo,
                  bool insets = true);
void drawNoiseOverlay(QPainter& p, const QRectF& rect, qreal radius);
void drawStyledText(QPainter& p, const QRectF& box, const QString& text,
                    const QFont& font, const QColor& fill, int flags,
                    const QVector<TextShadow>& shadows);
void drawGlowText(QPainter& p, const QRectF& box, const QString& text,
                  const QFont& font, const QColor& fill, const QColor& glow,
                  qreal blurRadius, int flags);
void paintBlurred(QPainter& p, const QRectF& bounds, qreal sigma,
                  const std::function<void(QPainter&)>& paint);
QImage loadTrampLogo();
QImage loadProximaMark();

/// Paint accounting for the benches. `TRAMP_BENCH_NO_BLUR` short-circuits every
/// blur so a run measures the rest of the chrome.
///
/// `layerNanos` is the whole `paintBlurred` round trip — buffer, offscreen
/// painter, the caller's drawing, the blur, the composite — so `layerNanos`
/// minus `nanos` is what the blurred-layer machinery costs around the kernel.
struct BlurCost {
  qint64 calls = 0;
  qint64 nanos = 0;
  qint64 pixels = 0;
  qint64 layers = 0;
  qint64 layerNanos = 0;
  qint64 fonts = 0;
  qint64 fontNanos = 0;
};
BlurCost blurCost();
void resetBlurCost();

}  // namespace tramp
