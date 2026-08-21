#pragma once

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <functional>

namespace tramp {

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

void drawBtn(QPainter& p, const QRectF& r, bool on, const QString& label = {});
qreal labelBtnWidth(const QString& label, qreal padL = 16, qreal padR = 16);
void drawIcon(QPainter& p, const QRectF& box, MockupIcon icon, const QColor& color);
void drawGlyphBtn(QPainter& p, const QRectF& r, MockupIcon icon, bool on,
                  qreal iconSize = 22);
void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle = false,
                bool glow = true);
void drawVBand(QPainter& p, const QRectF& column, qreal gainDb);
void drawLed(QPainter& p, QPointF c, bool on, qreal size = 8);
qreal toggleBtnWidth(const QString& label);
void drawToggleBtn(QPainter& p, const QRectF& r, const QString& label, bool lit);
void drawPlate(QPainter& p, const QRectF& r);
void drawRail(QPainter& p, const QRectF& r);
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

/// Gaussian-blur accounting for the paint benches. `TRAMP_BENCH_NO_BLUR`
/// short-circuits every blur so a run measures the rest of the chrome.
struct BlurCost {
  qint64 calls = 0;
  qint64 nanos = 0;
  qint64 pixels = 0;
};
BlurCost blurCost();
void resetBlurCost();

}  // namespace tramp
