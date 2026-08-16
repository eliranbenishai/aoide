#pragma once

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>

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

QFont condensedFont(int px, qreal trackingEm = 0);
QFont monoFont(int px, qreal trackingEm = 0);

void fillRound(QPainter& p, const QRectF& r, qreal radius, const QBrush& brush);
void drawScreen(QPainter& p, const QRectF& well);
void drawBtn(QPainter& p, const QRectF& r, bool on, const QString& label = {});
void drawIcon(QPainter& p, const QRectF& box, MockupIcon icon, const QColor& color);
void drawGlyphBtn(QPainter& p, const QRectF& r, MockupIcon icon, bool on,
                  qreal iconSize = 22);
void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle = false);
void drawVBand(QPainter& p, const QRectF& column, qreal gainDb);
void drawLed(QPainter& p, QPointF c, bool on, qreal size = 8);
void drawPlate(QPainter& p, const QRectF& r);
void drawRail(QPainter& p, const QRectF& r);
void drawMenuCaret(QPainter& p, const QRectF& btn);
void drawReload(QPainter& p, const QRectF& box, const QColor& color);
void drawNoiseOverlay(QPainter& p, const QRectF& rect, qreal radius);
void drawGlowText(QPainter& p, const QRectF& box, const QString& text,
                  const QFont& font, const QColor& fill, const QColor& glow,
                  qreal blur, int flags);
QImage loadTrampLogo();
QImage loadProximaMark();

}  // namespace tramp
