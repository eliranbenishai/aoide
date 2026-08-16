#include "mockup_draw.h"

#include "mockup_tokens.h"
#include "tramp_fonts.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>
#include <cmath>

namespace tramp {
namespace {

QImage g_noise;

QImage boxBlur(QImage src, int radius) {
  if (radius <= 0 || src.isNull()) {
    return src;
  }
  src = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  const int w = src.width();
  const int h = src.height();
  const int diam = radius * 2 + 1;
  auto pass = [&](bool horiz) {
    QImage out(w, h, src.format());
    out.fill(Qt::transparent);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int r = 0, g = 0, b = 0, a = 0;
        for (int k = -radius; k <= radius; ++k) {
          const int xx = horiz ? qBound(0, x + k, w - 1) : x;
          const int yy = horiz ? y : qBound(0, y + k, h - 1);
          const QRgb p = src.pixel(xx, yy);
          r += qRed(p);
          g += qGreen(p);
          b += qBlue(p);
          a += qAlpha(p);
        }
        out.setPixel(x, y, qRgba(r / diam, g / diam, b / diam, a / diam));
      }
    }
    src = out;
  };
  pass(true);
  pass(false);
  return src;
}

QImage noiseTile() {
  if (!g_noise.isNull()) {
    return g_noise;
  }
  constexpr int n = 140;
  g_noise = QImage(n, n, QImage::Format_ARGB32);
  auto hash = [](int x, int y) -> double {
    qint64 v = (qint64(x) * 374761393) ^ (qint64(y) * 668265263) ^
               (qint64(x) * y * 1274126177);
    v = (v ^ (v >> 13)) * 1274126177;
    v = (v ^ (v >> 16)) & 0x7fffffff;
    return double(v) / double(0x7fffffff);
  };
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      double sum = 0;
      double amp = 1;
      double norm = 0;
      int xo = x;
      int yo = y;
      for (int o = 0; o < 3; ++o) {
        sum += amp * hash(xo, yo);
        norm += amp;
        amp *= 0.5;
        xo = xo * 2 + 17;
        yo = yo * 2 + 31;
      }
      const int v = int(std::round((sum / norm) * 255.0));
      g_noise.setPixel(x, y, qRgba(v, v, v, 255));
    }
  }
  return g_noise;
}

void paintIconPath(QPainter& p, const QRectF& box, qreal view, const QPainterPath& path,
                   const QColor& color) {
  p.save();
  p.translate(box.topLeft());
  p.scale(box.width() / view, box.height() / view);
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawPath(path);
  p.restore();
}

QPainterPath pathPrev() {
  QPainterPath path;
  path.addRect(6, 5, 2.4, 14);
  path.moveTo(20, 5);
  path.lineTo(20, 19);
  path.lineTo(10.4, 12);
  path.closeSubpath();
  return path;
}

QPainterPath pathPlay() {
  QPainterPath path;
  path.moveTo(7, 4.5);
  path.lineTo(20, 12);
  path.lineTo(7, 19.5);
  path.closeSubpath();
  return path;
}

QPainterPath pathPause() {
  QPainterPath path;
  path.addRect(7, 5, 3.6, 14);
  path.addRect(13.4, 5, 3.6, 14);
  return path;
}

QPainterPath pathStop() {
  QPainterPath path;
  path.addRect(6, 6, 12, 12);
  return path;
}

QPainterPath pathNext() {
  QPainterPath path;
  path.addRect(15.6, 5, 2.4, 14);
  path.moveTo(4, 5);
  path.lineTo(13.6, 12);
  path.lineTo(4, 19);
  path.closeSubpath();
  return path;
}

QPainterPath pathEject() {
  QPainterPath path;
  path.moveTo(12, 4.5);
  path.lineTo(19.5, 13);
  path.lineTo(4.5, 13);
  path.closeSubpath();
  path.addRect(4.5, 15.5, 15, 3.5);
  return path;
}

QPainterPath pathAdd() {
  QPainterPath path;
  path.addRect(10.9, 4, 2.2, 6.9);
  path.addRect(4, 10.9, 16, 2.2);
  path.addRect(10.9, 13.1, 2.2, 6.9);
  return path;
}

QPainterPath pathRemove() {
  QPainterPath path;
  path.addRect(4, 10.9, 16, 2.2);
  return path;
}

QPainterPath pathSort() {
  QPainterPath path;
  path.addRoundedRect(2.6, 5.4, 11, 2.2, 1.1, 1.1);
  path.addRoundedRect(2.6, 10.9, 8, 2.2, 1.1, 1.1);
  path.addRoundedRect(2.6, 16.4, 5, 2.2, 1.1, 1.1);
  path.addRoundedRect(17.9, 5.4, 2.2, 9.4, 0.6, 0.6);
  path.moveTo(19, 19.3);
  path.lineTo(14.9, 14.2);
  path.lineTo(23.1, 14.2);
  path.closeSubpath();
  return path;
}

QPainterPath pathMinimize() {
  QPainterPath path;
  path.addRect(3, 10, 10, 2);
  return path;
}

QPainterPath pathZoomOut() {
  QPainterPath path;
  path.addRect(3, 7, 10, 2);
  return path;
}

QPainterPath pathZoomIn() {
  QPainterPath path;
  path.addRect(7, 3, 2, 4);
  path.addRect(9, 7, 4, 2);
  path.addRect(7, 9, 2, 4);
  path.addRect(3, 7, 4, 2);
  return path;
}

QPainterPath pathClose() {
  QPainterPath path;
  path.moveTo(4.4, 3);
  path.lineTo(8, 6.6);
  path.lineTo(11.6, 3);
  path.lineTo(13, 4.4);
  path.lineTo(9.4, 8);
  path.lineTo(13, 11.6);
  path.lineTo(11.6, 13);
  path.lineTo(8, 9.4);
  path.lineTo(4.4, 13);
  path.lineTo(3, 11.6);
  path.lineTo(6.6, 8);
  path.lineTo(3, 4.4);
  path.closeSubpath();
  return path;
}

void drawMute(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  p.translate(box.topLeft());
  const qreal s = box.width();
  QPainterPath body;
  body.moveTo(s * 0.08, s * 0.35);
  body.lineTo(s * 0.32, s * 0.35);
  body.lineTo(s * 0.58, s * 0.12);
  body.lineTo(s * 0.58, s * 0.88);
  body.lineTo(s * 0.32, s * 0.65);
  body.lineTo(s * 0.08, s * 0.65);
  body.closeSubpath();
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawPath(body);
  QPen wave(color, qMax(1.2, s * 0.07), Qt::SolidLine, Qt::RoundCap);
  p.setPen(wave);
  p.setBrush(Qt::NoBrush);
  p.drawArc(QRectF(s * 0.55 - s * 0.275, s * 0.5 - s * 0.275, s * 0.55, s * 0.55),
            60 * 16, -120 * 16);
  p.drawArc(QRectF(s * 0.58 - s * 0.42, s * 0.5 - s * 0.42, s * 0.84, s * 0.84),
            50 * 16, -100 * 16);
  p.restore();
}

void drawCog(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  p.translate(box.center());
  const qreal s = box.width() / 24.0;
  p.scale(s, s);
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  for (int i = 0; i < 8; ++i) {
    p.save();
    p.rotate(i * 45.0);
    p.drawRoundedRect(QRectF(-1.1, -8.8, 2.2, 4.0), 0.5, 0.5);
    p.restore();
  }
  QPainterPath ring;
  ring.setFillRule(Qt::OddEvenFill);
  ring.addEllipse(QRectF(-6.3, -6.3, 12.6, 12.6));
  ring.addEllipse(QRectF(-2.6, -2.6, 5.2, 5.2));
  p.drawPath(ring);
  p.restore();
}

}  // namespace

QFont condensedFont(int px, qreal trackingEm) {
  QFont f(chromeFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Bold);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

QFont monoFont(int px, qreal trackingEm) {
  QFont f(lcdFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Medium);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

void fillRound(QPainter& p, const QRectF& r, qreal radius, const QBrush& brush) {
  QPainterPath path;
  path.addRoundedRect(r, radius, radius);
  p.fillPath(path, brush);
}

void drawScreen(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, 3, 3);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.18,
                               well.top() - well.height() * 0.20),
                       well.width() * 0.9);
  wash.setColorAt(0, QColor(0x0f, 0x1c, 0x2a));
  wash.setColorAt(0.48, QColor(0x07, 0x10, 0x18));
  wash.setColorAt(1, QColor(0x04, 0x07, 0x0c));
  p.fillRect(well, wash);
  p.setPen(QPen(QColor(61, 231, 255, 26), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), 2.5, 2.5);
  for (qreal y = well.top(); y < well.bottom(); y += 3) {
    p.fillRect(QRectF(well.left(), y, well.width(), 1), QColor(0, 0, 0, 82));
  }
  QLinearGradient glass(well.topLeft(),
                        QPointF(well.left(), well.top() + well.height() * 0.38));
  glass.setColorAt(0, QColor(255, 255, 255, 13));
  glass.setColorAt(1, QColor(255, 255, 255, 0));
  p.fillRect(well, glass);
  p.restore();
  p.setPen(QPen(QColor(226, 236, 255, 31), 1));
  p.drawLine(QPointF(well.left() + 1, well.top() + 0.5),
             QPointF(well.right() - 1, well.top() + 0.5));
}

void drawBtn(QPainter& p, const QRectF& r, bool on, const QString& label) {
  QPainterPath path;
  path.addRoundedRect(r, 4, 4);
  if (on) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(61, 231, 255, 77));
    p.drawRoundedRect(r.adjusted(-6, -6, 6, 6), 8, 8);
    p.restore();
  }
  QLinearGradient face(r.topLeft(), r.bottomLeft());
  if (on) {
    face.setColorAt(0, kBtnOn0);
    face.setColorAt(0.45, kPhos);
    face.setColorAt(1, QColor(0x12, 0x95, 0xa8));
  } else {
    face.setColorAt(0, kBtnIdle0);
    face.setColorAt(0.48, kBtnIdle48);
    face.setColorAt(1, kBtnIdle100);
  }
  p.fillPath(path, face);
  p.save();
  p.setClipPath(path);
  if (on) {
    p.setPen(QPen(QColor(kBtnOnLip.red(), kBtnOnLip.green(), kBtnOnLip.blue(), 179), 1));
    p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
    p.fillRect(QRectF(r.left() + 1, r.bottom() - 4, r.width() - 2, 3),
               QColor(kBtnOnFoot.red(), kBtnOnFoot.green(), kBtnOnFoot.blue(), 140));
  } else {
    QLinearGradient rim(r.topLeft(), r.bottomLeft());
    rim.setColorAt(0, QColor(232, 240, 255, 51));
    rim.setColorAt(0.5, Qt::transparent);
    rim.setColorAt(1, QColor(0, 0, 0, 128));
    p.setPen(QPen(QBrush(rim), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  }
  QLinearGradient gloss(r.topLeft(), QPointF(r.left(), r.top() + r.height() * 0.55));
  gloss.setColorAt(0, QColor(232, 240, 255, on ? 71 : 31));
  gloss.setColorAt(1, QColor(232, 240, 255, 0));
  p.fillRect(QRectF(r.left() + 1, r.top() + 1, r.width() - 2, r.height() * 0.5), gloss);
  p.restore();
  if (!label.isEmpty()) {
    p.setFont(condensedFont(13, 0.18));
    p.setPen(on ? kBtnOnInk : kBtnLabelIdle);
    p.drawText(r, Qt::AlignCenter, label.toUpper());
  }
}

void drawIcon(QPainter& p, const QRectF& box, MockupIcon icon, const QColor& color) {
  switch (icon) {
    case MockupIcon::mute:
      drawMute(p, box, color);
      return;
    case MockupIcon::options:
      drawCog(p, box, color);
      return;
    case MockupIcon::previous:
      paintIconPath(p, box, 24, pathPrev(), color);
      return;
    case MockupIcon::play:
      paintIconPath(p, box, 24, pathPlay(), color);
      return;
    case MockupIcon::pause:
      paintIconPath(p, box, 24, pathPause(), color);
      return;
    case MockupIcon::stop:
      paintIconPath(p, box, 24, pathStop(), color);
      return;
    case MockupIcon::next:
      paintIconPath(p, box, 24, pathNext(), color);
      return;
    case MockupIcon::eject:
      paintIconPath(p, box, 24, pathEject(), color);
      return;
    case MockupIcon::add:
      paintIconPath(p, box, 24, pathAdd(), color);
      return;
    case MockupIcon::remove:
      paintIconPath(p, box, 24, pathRemove(), color);
      return;
    case MockupIcon::sort:
      paintIconPath(p, box, 24, pathSort(), color);
      return;
    case MockupIcon::minimize:
      paintIconPath(p, box, 16, pathMinimize(), color);
      return;
    case MockupIcon::zoomOut:
      paintIconPath(p, box, 16, pathZoomOut(), color);
      return;
    case MockupIcon::zoomIn:
      paintIconPath(p, box, 16, pathZoomIn(), color);
      return;
    case MockupIcon::close:
      paintIconPath(p, box, 16, pathClose(), color);
      return;
  }
}

void drawGlyphBtn(QPainter& p, const QRectF& r, MockupIcon icon, bool on, qreal iconSize) {
  drawBtn(p, r, on, {});
  const QRectF box(r.center().x() - iconSize / 2, r.center().y() - iconSize / 2, iconSize,
                   iconSize);
  drawIcon(p, box, icon, on ? kBtnOnInk : QColor(214, 226, 245, 217));
}

void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle) {
  t = qBound(0.0, t, 1.0);
  QPainterPath trough;
  trough.addRoundedRect(track, track.height() / 2, track.height() / 2);
  QLinearGradient tg(track.topLeft(), track.bottomLeft());
  tg.setColorAt(0, QColor(0x06, 0x07, 0x0a));
  tg.setColorAt(0.6, QColor(0x14, 0x18, 0x21));
  tg.setColorAt(1, QColor(0x1e, 0x22, 0x2c));
  p.fillPath(trough, tg);
  p.setPen(QPen(QColor(226, 236, 255, 20), 1));
  p.setBrush(Qt::NoBrush);
  p.drawPath(trough);

  const qreal fillW = qMax(0.0, (track.width() - 4) * t);
  if (fillW > 0) {
    QRectF fill(track.left() + 2, track.top() + 2, fillW, track.height() - 4);
    QPainterPath fillPath;
    if (seekStyle) {
      fillPath.addRoundedRect(fill, 3, 3);
    } else {
      fillPath.addRoundedRect(fill, fill.height() / 2, fill.height() / 2);
    }
    p.fillPath(fillPath, QColor(61, 231, 255, 102));
    QLinearGradient g(fill.topLeft(), fill.bottomLeft());
    g.setColorAt(0, kSliderFillHi);
    g.setColorAt(0.4, kPhos);
    g.setColorAt(1, kSliderFillLo);
    p.fillPath(fillPath, g);
  }

  const QSizeF thumb = seekStyle ? QSizeF(22, 32) : QSizeF(20, 30);
  const qreal x = qBound(track.left() + thumb.width() / 2,
                         track.left() + track.width() * t,
                         track.right() - thumb.width() / 2);
  const QRectF thumbR(x - thumb.width() / 2, track.center().y() - thumb.height() / 2,
                      thumb.width(), thumb.height());
  fillRound(p, thumbR.translated(0, 1), 4, QColor(0, 0, 0, 166));
  QLinearGradient face(thumbR.topLeft(), thumbR.bottomLeft());
  face.setColorAt(0, QColor(0x6f, 0x76, 0x88));
  face.setColorAt(0.55, QColor(0x3d, 0x43, 0x50));
  face.setColorAt(1, QColor(0x22, 0x26, 0x2f));
  fillRound(p, thumbR, 4, face);
  const QRectF grip(thumbR.left() + 5, thumbR.top() + 8, thumbR.width() - 10,
                    thumbR.height() - 16);
  p.setPen(QPen(QColor(232, 240, 255, 56), 1));
  for (qreal y = grip.top(); y < grip.bottom(); y += 2.2) {
    p.drawLine(QPointF(grip.left(), y), QPointF(grip.right(), y));
  }
}

void drawVBand(QPainter& p, const QRectF& column, qreal gainDb) {
  constexpr qreal trackW = 12;
  const QRectF track(column.center().x() - trackW / 2, column.top(), trackW,
                     column.height());
  QPainterPath trough;
  trough.addRoundedRect(track, 6, 6);
  QLinearGradient tg(track.topLeft(), track.topRight());
  tg.setColorAt(0, QColor(0x06, 0x07, 0x0a));
  tg.setColorAt(0.55, QColor(0x16, 0x1a, 0x22));
  tg.setColorAt(1, QColor(0x1e, 0x22, 0x2c));
  p.fillPath(trough, tg);

  const qreal frac = qBound(0.0, (gainDb + 12.0) / 24.0, 1.0);
  const qreal thumbY = track.top() + (1.0 - frac) * track.height();
  if (track.bottom() - thumbY > 0.5) {
    p.save();
    p.setClipPath(trough);
    p.setClipRect(QRectF(track.left(), thumbY, trackW, track.bottom() - thumbY),
                  Qt::IntersectClip);
    QLinearGradient spec(track.bottomLeft(), track.topLeft());
    spec.setColorAt(0, kSpectrum0);
    spec.setColorAt(0.26, kPhos);
    spec.setColorAt(0.62, kSpectrum2);
    spec.setColorAt(1, kAccent);
    p.fillRect(track, spec);
    p.restore();
  }

  p.fillRect(QRectF(track.left() - 13, track.center().y() - 0.5, trackW + 26, 1),
             QColor(226, 236, 255, 36));

  const QRectF thumb(column.center().x() - 17, thumbY - 9, 34, 18);
  QLinearGradient face(thumb.topLeft(), thumb.bottomLeft());
  face.setColorAt(0, QColor(0x75, 0x7c, 0x8f));
  face.setColorAt(0.42, QColor(0x3d, 0x43, 0x50));
  face.setColorAt(1, QColor(0x1e, 0x22, 0x2c));
  fillRound(p, thumb, 3, face);
  p.setPen(QPen(QColor(236, 244, 255, 89), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
  const QRectF line(thumb.center().x() - 11, thumb.center().y() - 1, 22, 2);
  QLinearGradient lg(line.topLeft(), line.topRight());
  lg.setColorAt(0, kSpectrum0);
  lg.setColorAt(1, kPhos);
  fillRound(p, line, 1, lg);
}

void drawLed(QPainter& p, QPointF c, bool on, qreal size) {
  const qreal r = size / 2;
  if (on) {
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 89));
    p.drawEllipse(c, r + 5, r + 5);
    p.setBrush(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 179));
    p.drawEllipse(c, r + 1.5, r + 1.5);
    QRadialGradient g(c + QPointF(-r * 0.2, -r * 0.3), r);
    g.setColorAt(0, kAccentHot);
    g.setColorAt(0.45, kAccent);
    g.setColorAt(1, kAccentDim);
    p.setBrush(g);
    p.setPen(QPen(QColor(kLitLedRim.red(), kLitLedRim.green(), kLitLedRim.blue(), 153), 1));
    p.drawEllipse(c, r, r);
  } else {
    QRadialGradient g(c + QPointF(-r * 0.2, -r * 0.3), r);
    g.setColorAt(0, kIdleLedHi);
    g.setColorAt(1, kIdleLedLo);
    p.setBrush(g);
    p.setPen(QPen(QColor(0, 0, 0, 204), 1));
    p.drawEllipse(c, r, r);
  }
}

void drawPlate(QPainter& p, const QRectF& r) {
  fillRound(p, r, 4, kPlateFace);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(r, 4, 4);
  p.setClipPath(clip);
  for (qreal y = r.top(); y < r.bottom(); y += 3) {
    p.fillRect(QRectF(r.left(), y, r.width(), 1), QColor(226, 236, 255, 11));
    p.fillRect(QRectF(r.left(), y + 1, r.width(), 1), QColor(0, 0, 0, 26));
  }
  p.setPen(QPen(QColor(226, 236, 255, 26), 1));
  p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
  p.setPen(QPen(QColor(0, 0, 0, 179), 1));
  p.drawLine(QPointF(r.left() + 2, r.bottom() - 1), QPointF(r.right() - 2, r.bottom() - 1));
  p.restore();
}

void drawRail(QPainter& p, const QRectF& r) {
  p.save();
  p.setOpacity(0.9);
  fillRound(p, r, 3, kPlateFace);
  QPainterPath clip;
  clip.addRoundedRect(r, 3, 3);
  p.setClipPath(clip);
  for (qreal y = r.top(); y < r.bottom(); y += 3) {
    p.fillRect(QRectF(r.left(), y, r.width(), 1), QColor(226, 236, 255, 11));
    p.fillRect(QRectF(r.left(), y + 1, r.width(), 1), QColor(0, 0, 0, 26));
  }
  p.restore();
}

void drawMenuCaret(QPainter& p, const QRectF& btn) {
  const QRectF c(btn.right() - 11, btn.bottom() - 11, 6, 6);
  QPainterPath path;
  path.moveTo(c.left(), c.bottom());
  path.lineTo(c.right(), c.bottom());
  path.lineTo(c.right(), c.top());
  path.closeSubpath();
  p.fillPath(path, QColor(214, 226, 245, 115));
}

void drawReload(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  const qreal s = box.width();
  const QPointF c = box.center();
  const qreal radius = s * 0.34;
  QPen pen(color, qMax(1.2, s * 0.14), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  p.drawArc(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2),
            int(-0.85 * 180 * 16), int(0.95 * 180 * 16));
  p.drawArc(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2),
            int(0.15 * 180 * 16), int(0.95 * 180 * 16));
  p.restore();
}

void drawNoiseOverlay(QPainter& p, const QRectF& rect, qreal radius) {
  const QRectF inner = rect.adjusted(1, 1, -1, -1);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(inner, qMax(0.0, radius - 1), qMax(0.0, radius - 1));
  p.setClipPath(clip);
  p.setOpacity(0.05);
  p.setCompositionMode(QPainter::CompositionMode_Overlay);
  p.drawTiledPixmap(inner, QPixmap::fromImage(noiseTile()));
  p.restore();
}

void drawGlowText(QPainter& p, const QRectF& box, const QString& text, const QFont& font,
                  const QColor& fill, const QColor& glow, qreal blur, int flags) {
  if (blur > 0.4) {
    const int pad = int(std::ceil(blur * 3));
    QImage buf(int(box.width()) + pad * 2, int(box.height()) + pad * 2,
               QImage::Format_ARGB32_Premultiplied);
    buf.fill(Qt::transparent);
    QPainter bp(&buf);
    bp.setRenderHint(QPainter::TextAntialiasing);
    bp.setFont(font);
    bp.setPen(glow);
    bp.drawText(QRectF(pad, pad, box.width(), box.height()), flags, text);
    bp.end();
    p.drawImage(box.topLeft() - QPointF(pad, pad), boxBlur(buf, qMax(1, int(blur / 2))));
  }
  p.setFont(font);
  p.setPen(fill);
  p.drawText(box, flags, text);
}

QImage loadTrampLogo() {
  QImage img(assetPath("branding/logo.png"));
  if (img.isNull()) {
    img.load(assetPath("branding/app_icon.png"));
  }
  return img;
}

QImage loadProximaMark() {
  return QImage(assetPath("branding/proxima_mark.png"));
}

}  // namespace tramp
