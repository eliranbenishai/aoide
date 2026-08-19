#include "mockup_draw.h"

#include "look.h"
#include "tramp_fonts.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>
#include <cmath>
#include <functional>
#include <vector>

namespace tramp {
namespace {

const ChromeTokens& T() { return currentLook(); }

QImage g_noise;

// Separable Gaussian. `sigma` is Skia/Flutter MaskFilter sigma.
QImage gaussianBlur(QImage src, qreal sigma) {
  if (src.isNull() || sigma < 0.12) {
    return src;
  }
  src = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  const int radius = qMax(1, int(std::ceil(sigma * 3.0)));
  const int kSize = radius * 2 + 1;
  std::vector<qreal> kernel(static_cast<size_t>(kSize));
  qreal sum = 0;
  const qreal s2 = 2 * sigma * sigma;
  for (int i = -radius; i <= radius; ++i) {
    const qreal w = std::exp(-(qreal(i) * qreal(i)) / s2);
    kernel[static_cast<size_t>(i + radius)] = w;
    sum += w;
  }
  for (qreal& w : kernel) {
    w /= sum;
  }

  auto pass = [&](bool horiz) {
    QImage out(src.size(), src.format());
    out.fill(Qt::transparent);
    const int w = src.width();
    const int h = src.height();
    for (int y = 0; y < h; ++y) {
      const QRgb* inLine = horiz ? reinterpret_cast<const QRgb*>(src.constScanLine(y))
                                 : nullptr;
      QRgb* outLine = reinterpret_cast<QRgb*>(out.scanLine(y));
      for (int x = 0; x < w; ++x) {
        qreal r = 0, g = 0, b = 0, a = 0;
        for (int k = -radius; k <= radius; ++k) {
          const int xx = horiz ? qBound(0, x + k, w - 1) : x;
          const int yy = horiz ? y : qBound(0, y + k, h - 1);
          const QRgb p = horiz ? inLine[xx]
                               : reinterpret_cast<const QRgb*>(src.constScanLine(yy))[xx];
          const qreal wk = kernel[static_cast<size_t>(k + radius)];
          r += qRed(p) * wk;
          g += qGreen(p) * wk;
          b += qBlue(p) * wk;
          a += qAlpha(p) * wk;
        }
        outLine[x] = qRgba(int(r + 0.5), int(g + 0.5), int(b + 0.5), int(a + 0.5));
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
  p.scale(box.width() / 24.0, box.height() / 24.0);
  QPainterPath body;
  body.moveTo(4, 9.4);
  body.lineTo(7.3, 9.4);
  body.lineTo(12, 5);
  body.lineTo(12, 19);
  body.lineTo(7.3, 14.6);
  body.lineTo(4, 14.6);
  body.closeSubpath();
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawPath(body);
  QPen wave(color, 1.7, Qt::SolidLine, Qt::RoundCap);
  p.setPen(wave);
  p.setBrush(Qt::NoBrush);
  p.drawArc(QRectF(11.2, 7.8, 8.4, 8.4), 90 * 16, -180 * 16);
  p.drawArc(QRectF(10.2, 4.2, 15.6, 15.6), 90 * 16, -180 * 16);
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

void paintBlurred(QPainter& p, const QRectF& bounds, qreal sigma,
                  const std::function<void(QPainter&)>& paint) {
  if (sigma < 0.12) {
    p.save();
    paint(p);
    p.restore();
    return;
  }
  const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
  QImage buf(int(std::ceil(bounds.width())) + pad * 2,
             int(std::ceil(bounds.height())) + pad * 2,
             QImage::Format_ARGB32_Premultiplied);
  buf.fill(Qt::transparent);
  QPainter bp(&buf);
  bp.setRenderHint(QPainter::Antialiasing);
  bp.setRenderHint(QPainter::TextAntialiasing);
  bp.translate(pad - bounds.left(), pad - bounds.top());
  paint(bp);
  bp.end();
  p.drawImage(bounds.topLeft() - QPointF(pad, pad), gaussianBlur(buf, sigma));
}

QFont condensedFont(int px, qreal trackingEm) {
  QFont f(chromeFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Bold);
  f.setHintingPreference(QFont::PreferNoHinting);
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
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

qreal textWidth(const QFont& font, const QString& text) {
  return QFontMetricsF(font).horizontalAdvance(text);
}

void fillRound(QPainter& p, const QRectF& r, qreal radius, const QBrush& brush) {
  QPainterPath path;
  path.addRoundedRect(r, radius, radius);
  p.fillPath(path, brush);
}

namespace {

struct CachedWell {
  int w = 0;
  int h = 0;
  QString lookId;
  QImage bloom;
  int bloomPad = 0;
  QImage inner;
  int innerPad = 0;
};

const CachedWell& cachedWell(int w, int h) {
  static std::vector<CachedWell> cache;
  const QString lookId = T().id;
  for (const CachedWell& c : cache) {
    if (c.w == w && c.h == h && c.lookId == lookId) return c;
  }
  CachedWell c;
  c.w = w;
  c.h = h;
  c.lookId = lookId;
  const QRectF well(0, 0, w, h);
  {
    constexpr qreal sigma = 12;
    const QRectF bounds = well.adjusted(-36, -36, 36, 36);
    const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
    c.bloomPad = pad;
    QImage buf(int(std::ceil(bounds.width())) + pad * 2,
               int(std::ceil(bounds.height())) + pad * 2,
               QImage::Format_ARGB32_Premultiplied);
    buf.fill(Qt::transparent);
    QPainter bp(&buf);
    bp.setRenderHint(QPainter::Antialiasing);
    bp.translate(pad - bounds.left(), pad - bounds.top());
    bp.setPen(Qt::NoPen);
    bp.setBrush(withAlpha(T().phos, 13));
    bp.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), 2.5, 2.5);
    bp.end();
    c.bloom = gaussianBlur(buf, sigma);
  }
  {
    constexpr qreal sigma = 3;
    const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
    c.innerPad = pad;
    QImage buf(w + pad * 2, h + pad * 2, QImage::Format_ARGB32_Premultiplied);
    buf.fill(Qt::transparent);
    QPainter bp(&buf);
    bp.setRenderHint(QPainter::Antialiasing);
    bp.setPen(Qt::NoPen);
    bp.setBrush(QColor(0, 0, 0, 0xE6));
    bp.drawRoundedRect(QRectF(pad + 1, pad + 1, w - 2, h - 2), 2, 2);
    bp.end();
    QImage blurred = gaussianBlur(buf, sigma);
    QImage mask(buf.size(), QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    QPainter mp(&mask);
    mp.setRenderHint(QPainter::Antialiasing);
    mp.setPen(Qt::NoPen);
    mp.setBrush(Qt::white);
    mp.drawRoundedRect(QRectF(pad, pad, w, h), 3, 3);
    mp.end();
    QPainter mix(&blurred);
    mix.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    mix.drawImage(0, 0, mask);
    mix.end();
    c.inner = std::move(blurred);
  }
  cache.push_back(std::move(c));
  return cache.back();
}

}  // namespace

void drawScreenWell(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, 3, 3);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.18,
                               well.top() - well.height() * 0.20),
                       well.width() * 0.9);
  wash.setColorAt(0, T().screenWash0);
  wash.setColorAt(0.48, T().screenWash1);
  wash.setColorAt(1, T().screenWash2);
  p.fillRect(well, wash);
  p.setPen(QPen(QColor(T().coolSheen.red(), T().coolSheen.green(), T().coolSheen.blue(), 31), 1));
  p.drawLine(QPointF(well.left(), well.top() + 0.5),
             QPointF(well.right(), well.top() + 0.5));
  p.setPen(QPen(withAlpha(T().phos, 26), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), 2.5, 2.5);
  p.restore();

  const CachedWell& fx =
      cachedWell(int(std::lround(well.width())), int(std::lround(well.height())));
  p.drawImage(well.topLeft() - QPointF(36 + fx.bloomPad, 36 + fx.bloomPad), fx.bloom);
  p.drawImage(well.topLeft() - QPointF(fx.innerPad, fx.innerPad), fx.inner);
}

void drawScreenOverlay(QPainter& p, const QRectF& well, QColor scan, bool glass) {
  QPainterPath path;
  path.addRoundedRect(well, 3, 3);
  p.save();
  p.setClipPath(path);
  for (qreal y = well.top(); y < well.bottom(); y += 3) {
    p.fillRect(QRectF(well.left(), y, well.width(), 1), scan);
  }
  if (glass) {
    QLinearGradient wash(well.topLeft(),
                         QPointF(well.left(), well.top() + well.height() * 0.38));
    wash.setColorAt(0, QColor(255, 255, 255, 13));
    wash.setColorAt(1, QColor(255, 255, 255, 0));
    p.fillRect(well, wash);
  }
  p.restore();
}

void drawScreen(QPainter& p, const QRectF& well) {
  drawScreenWell(p, well);
  drawScreenOverlay(p, well);
}

void drawListWell(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, 3, 3);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.2,
                               well.top() - well.height() * 0.10),
                       well.width() * 1.05);
  wash.setColorAt(0, T().listWash0);
  wash.setColorAt(0.7, T().listWash1);
  wash.setColorAt(1, T().listWash2);
  p.fillRect(well, wash);
  constexpr qreal row = 37;
  for (qreal y = well.top(); y < well.bottom(); y += row * 2) {
    p.fillRect(QRectF(well.left(), y, well.width(), row), withAlpha(T().ink, 4));
    p.fillRect(QRectF(well.left(), y + row, well.width(), row), QColor(0, 0, 0, 31));
  }
  p.restore();
  p.setPen(QPen(withAlpha(T().phos, 20), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRect(well.adjusted(0.5, 0.5, -0.5, -0.5));
}

void drawBtn(QPainter& p, const QRectF& r, bool on, const QString& label) {
  QPainterPath path;
  path.addRoundedRect(r, 4, 4);
  if (on) {
    paintBlurred(p, r.adjusted(-18, -18, 18, 18), 4, [&](QPainter& bp) {
      bp.setPen(Qt::NoPen);
      bp.setBrush(withAlpha(T().phos, 77));
      bp.drawRoundedRect(r.adjusted(-3, -3, 3, 3), 7, 7);
    });
  }
  QLinearGradient face(r.topLeft(), r.bottomLeft());
  if (on) {
    face.setColorAt(0, T().btnOn0);
    face.setColorAt(0.45, T().btnOn1);
    face.setColorAt(1, T().btnOn2);
  } else {
    face.setColorAt(0, T().btnIdle0);
    face.setColorAt(0.48, T().btnIdle48);
    face.setColorAt(1, T().btnIdle100);
  }
  p.fillPath(path, face);
  p.save();
  p.setClipPath(path);
  if (on) {
    p.setPen(QPen(QColor(T().btnOnLip.red(), T().btnOnLip.green(), T().btnOnLip.blue(), 179), 1));
    p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
    p.fillRect(QRectF(r.left() + 1, r.bottom() - 4, r.width() - 2, 3),
               QColor(T().btnOnFoot.red(), T().btnOnFoot.green(), T().btnOnFoot.blue(), 140));
  } else {
    QLinearGradient rim(r.topLeft(), r.bottomLeft());
    rim.setColorAt(0, withAlpha(T().hoverLift, 51));
    rim.setColorAt(0.5, Qt::transparent);
    rim.setColorAt(1, QColor(0, 0, 0, 128));
    p.setPen(QPen(QBrush(rim), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  }
  QLinearGradient gloss(r.topLeft(), QPointF(r.left(), r.top() + r.height() * 0.55));
  gloss.setColorAt(0, withAlpha(T().hoverLift, on ? 71 : 31));
  gloss.setColorAt(1, withAlpha(T().hoverLift, 0));
  p.fillRect(QRectF(r.left() + 1, r.top() + 1, r.width() - 2, r.height() * 0.5), gloss);
  p.restore();
  paintBlurred(p, r.adjusted(-4, -2, 4, 6), 1.2, [&](QPainter& bp) {
    bp.setPen(QPen(QColor(0, 0, 0, 153), 0.5));
    bp.setBrush(Qt::NoBrush);
    bp.drawRoundedRect(r.translated(0, 1), 4, 4);
  });
  if (!label.isEmpty()) {
    p.setFont(condensedFont(13, 0.18));
    p.setPen(on ? T().btnOnInk : T().btnLabelIdle);
    p.drawText(r, Qt::AlignCenter, label.toUpper());
  }
}

qreal labelBtnWidth(const QString& label, qreal padL, qreal padR) {
  return padL + textWidth(condensedFont(13, 0.18), label.toUpper()) + padR;
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
  drawIcon(p, box, icon, on ? T().btnOnInk : T().glyphInk);
}

void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle, bool glow) {
  t = qBound(0.0, t, 1.0);
  QPainterPath trough;
  trough.addRoundedRect(track, track.height() / 2, track.height() / 2);
  QLinearGradient tg(track.topLeft(), track.bottomLeft());
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.6, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);
  p.setPen(QPen(withAlpha(T().coolSheen, 20), 1));
  p.setBrush(Qt::NoBrush);
  p.drawPath(trough);

  const qreal fillW = qMax(0.0, (track.width() - 4) * t);
  if (fillW > 0) {
    QRectF fill(track.left() + 2, track.top() + 2, fillW, track.height() - 4);
    QPainterPath fillPath;
    if (seekStyle) {
      const qreal rl = fill.height() / 2;
      const qreal rr = 3;
      fillPath.moveTo(fill.left() + rl, fill.top());
      fillPath.lineTo(fill.right() - rr, fill.top());
      fillPath.quadTo(fill.right(), fill.top(), fill.right(), fill.top() + rr);
      fillPath.lineTo(fill.right(), fill.bottom() - rr);
      fillPath.quadTo(fill.right(), fill.bottom(), fill.right() - rr, fill.bottom());
      fillPath.lineTo(fill.left() + rl, fill.bottom());
      fillPath.quadTo(fill.left(), fill.bottom(), fill.left(), fill.bottom() - rl);
      fillPath.lineTo(fill.left(), fill.top() + rl);
      fillPath.quadTo(fill.left(), fill.top(), fill.left() + rl, fill.top());
      fillPath.closeSubpath();
    } else {
      fillPath.addRoundedRect(fill, fill.height() / 2, fill.height() / 2);
    }
    if (glow) {
      paintBlurred(p, fill.adjusted(-8, -8, 8, 8), 4, [&](QPainter& bp) {
        bp.setPen(Qt::NoPen);
        bp.setBrush(withAlpha(T().phos, 102));
        bp.drawPath(fillPath);
      });
    }
    QLinearGradient g(fill.topLeft(), fill.bottomLeft());
    g.setColorAt(0, T().sliderFillHi);
    g.setColorAt(0.4, T().phos);
    g.setColorAt(1, T().sliderFillLo);
    p.fillPath(fillPath, g);
    p.setPen(QPen(QColor(T().btnOnLip.red(), T().btnOnLip.green(), T().btnOnLip.blue(), 153), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(fillPath);
  }

  const QSizeF thumb = seekStyle ? QSizeF(22, 32) : QSizeF(20, 30);
  const qreal x = qBound(track.left() + thumb.width() / 2,
                         track.left() + track.width() * t,
                         track.right() - thumb.width() / 2);
  const QRectF thumbR(x - thumb.width() / 2, track.center().y() - thumb.height() / 2,
                      thumb.width(), thumb.height());
  if (glow) {
    paintBlurred(p, thumbR.adjusted(-4, -2, 4, 6), 2, [&](QPainter& bp) {
      bp.setPen(Qt::NoPen);
      bp.setBrush(QColor(0, 0, 0, 166));
      bp.drawRoundedRect(thumbR.translated(0, 1), 4, 4);
    });
  }
  QLinearGradient face(thumbR.topLeft(), thumbR.bottomLeft());
  face.setColorAt(0, T().metalHi);
  face.setColorAt(0.55, T().metalMid);
  face.setColorAt(1, T().metalLo);
  fillRound(p, thumbR, 4, face);
  QLinearGradient rim(thumbR.topLeft(), thumbR.bottomRight());
  rim.setColorAt(0, withAlpha(T().btnLabelIdle, 140));
  rim.setColorAt(1, T().idleLedLo);
  p.setPen(QPen(QBrush(rim), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(thumbR.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  const QRectF grip(thumbR.left() + 5, thumbR.top() + 8, thumbR.width() - 10,
                    thumbR.height() - 16);
  p.setPen(QPen(withAlpha(T().hoverLift, 56), 1));
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
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.55, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);

  const qreal frac = qBound(0.0, (gainDb + 12.0) / 24.0, 1.0);
  const qreal thumbY = track.top() + (1.0 - frac) * track.height();
  if (track.bottom() - thumbY > 0.5) {
    p.save();
    p.setClipPath(trough);
    p.setClipRect(QRectF(track.left(), thumbY, trackW, track.bottom() - thumbY),
                  Qt::IntersectClip);
    p.fillRect(track, T().spectrumGradient(track.bottomLeft(), track.topLeft()));
    p.restore();
  }

  p.fillRect(QRectF(track.left() - 13, track.center().y() - 0.5, trackW + 26, 1),
             withAlpha(T().coolSheen, 36));

  const QRectF thumb(column.center().x() - 17, thumbY - 9, 34, 18);
  QLinearGradient face(thumb.topLeft(), thumb.bottomLeft());
  face.setColorAt(0, T().eqThumbHi);
  face.setColorAt(0.42, T().idleLedHi);
  face.setColorAt(1, T().metalLo);
  fillRound(p, thumb, 3, face);
  p.setPen(QPen(withAlpha(T().hoverLift, 89), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
  const QRectF line(thumb.center().x() - 11, thumb.center().y() - 1, 22, 2);
  QLinearGradient lg(line.topLeft(), line.topRight());
  lg.setColorAt(0, T().spectrumStops.value(0, T().phosHot));
  lg.setColorAt(1, T().phos);
  fillRound(p, line, 1, lg);
}

void drawLed(QPainter& p, QPointF c, bool on, qreal size) {
  const qreal r = size / 2;
  if (on) {
    paintBlurred(p, QRectF(c.x() - r - 12, c.y() - r - 12, (r + 12) * 2, (r + 12) * 2), 6,
                 [&](QPainter& bp) {
                   bp.setPen(Qt::NoPen);
                   bp.setBrush(QColor(T().accent.red(), T().accent.green(), T().accent.blue(), 89));
                   bp.drawEllipse(c, r + 5, r + 5);
                 });
    paintBlurred(p, QRectF(c.x() - r - 8, c.y() - r - 8, (r + 8) * 2, (r + 8) * 2), 3,
                 [&](QPainter& bp) {
                   bp.setPen(Qt::NoPen);
                   bp.setBrush(QColor(T().accent.red(), T().accent.green(), T().accent.blue(), 179));
                   bp.drawEllipse(c, r + 1.5, r + 1.5);
                 });
    QRadialGradient g(c + QPointF(-r * 0.2, -r * 0.3), r);
    g.setColorAt(0, T().accentHot);
    g.setColorAt(0.45, T().accent);
    g.setColorAt(1, T().accentDim);
    p.setBrush(g);
    p.setPen(QPen(QColor(T().litLedRim.red(), T().litLedRim.green(), T().litLedRim.blue(), 153), 1));
    p.drawEllipse(c, r, r);
  } else {
    QRadialGradient g(c + QPointF(-r * 0.2, -r * 0.3), r);
    g.setColorAt(0, T().idleLedHi);
    g.setColorAt(1, T().idleLedLo);
    p.setBrush(g);
    p.setPen(QPen(QColor(0, 0, 0, 204), 1));
    p.drawEllipse(c, r, r);
  }
}

void drawPlate(QPainter& p, const QRectF& r) {
  fillRound(p, r, 4, T().plateFace);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(r, 4, 4);
  p.setClipPath(clip);
  for (qreal y = r.top(); y < r.bottom(); y += 3) {
    p.fillRect(QRectF(r.left(), y, r.width(), 1), withAlpha(T().coolSheen, 11));
    p.fillRect(QRectF(r.left(), y + 1, r.width(), 1), QColor(0, 0, 0, 26));
  }
  p.setPen(QPen(withAlpha(T().coolSheen, 26), 1));
  p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
  p.setPen(QPen(QColor(0, 0, 0, 179), 1));
  p.drawLine(QPointF(r.left() + 2, r.bottom() - 1), QPointF(r.right() - 2, r.bottom() - 1));
  p.restore();
}

void drawRail(QPainter& p, const QRectF& r) {
  p.save();
  p.setOpacity(0.9);
  fillRound(p, r, 3, T().plateFace);
  QPainterPath clip;
  clip.addRoundedRect(r, 3, 3);
  p.setClipPath(clip);
  for (qreal y = r.top(); y < r.bottom(); y += 3) {
    p.fillRect(QRectF(r.left(), y, r.width(), 1), withAlpha(T().coolSheen, 11));
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
  p.fillPath(path, withAlpha(T().glyphInk, 115));
}

void drawReload(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  const qreal s = box.width();
  const QPointF c = box.center();
  const qreal radius = s * 0.34;
  const qreal width = qMax(1.2, s * 0.14);
  QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  auto arrow = [&](qreal start, qreal sweep, qreal tipAngle) {
    // Flutter Canvas.drawArc is clockwise; Qt is counter-clockwise.
    const int start16 = int(-start * 180.0 / M_PI * 16);
    const int sweep16 = int(-sweep * 180.0 / M_PI * 16);
    p.drawArc(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2), start16,
              sweep16);
    const QPointF tip(c.x() + radius * std::cos(tipAngle),
                      c.y() + radius * std::sin(tipAngle));
    const qreal tangent = tipAngle + M_PI / 2;
    const qreal head = s * 0.22;
    const qreal wing = s * 0.16;
    QPainterPath path;
    path.moveTo(tip.x() + head * std::cos(tangent), tip.y() + head * std::sin(tangent));
    path.lineTo(tip);
    path.lineTo(tip.x() - wing * std::cos(tipAngle), tip.y() - wing * std::sin(tipAngle));
    p.drawPath(path);
  };
  arrow(-M_PI * 0.85, M_PI * 0.95, M_PI * 0.12);
  arrow(M_PI * 0.15, M_PI * 0.95, M_PI * 1.12);
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

void drawStyledText(QPainter& p, const QRectF& box, const QString& text,
                    const QFont& font, const QColor& fill, int flags,
                    const QVector<TextShadow>& shadows) {
  for (const TextShadow& shadow : shadows) {
    const QRectF dest = box.translated(shadow.offset);
    if (shadow.blurRadius > 0.2) {
      const qreal sigma = shadow.blurRadius * 0.57735;
      const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
      QImage buf(int(std::ceil(box.width())) + pad * 2,
                 int(std::ceil(box.height())) + pad * 2,
                 QImage::Format_ARGB32_Premultiplied);
      buf.fill(Qt::transparent);
      QPainter bp(&buf);
      bp.setRenderHint(QPainter::TextAntialiasing);
      bp.setFont(font);
      bp.setPen(shadow.color);
      bp.drawText(QRectF(pad, pad, box.width(), box.height()), flags, text);
      bp.end();
      p.drawImage(dest.topLeft() - QPointF(pad, pad), gaussianBlur(buf, sigma));
    } else {
      p.setFont(font);
      p.setPen(shadow.color);
      p.drawText(dest, flags, text);
    }
  }
  p.setFont(font);
  p.setPen(fill);
  p.drawText(box, flags, text);
}

void drawGlowText(QPainter& p, const QRectF& box, const QString& text, const QFont& font,
                  const QColor& fill, const QColor& glow, qreal blurRadius, int flags) {
  drawStyledText(p, box, text, font, fill, flags, {{glow, {}, blurRadius}});
}

qreal toggleBtnWidth(const QString& label) {
  return 15 + 8 + 9 + textWidth(condensedFont(13, 0.16), label.toUpper()) + 15;
}

void drawToggleBtn(QPainter& p, const QRectF& r, const QString& label, bool lit) {
  drawBtn(p, r, false, {});
  drawLed(p, QPointF(r.left() + 15 + 4, r.center().y()), lit);
  p.setFont(condensedFont(13, 0.16));
  p.setPen(T().btnLabelIdle);
  p.drawText(r.adjusted(15 + 8 + 9, 0, -15, 0), Qt::AlignVCenter | Qt::AlignLeft,
             label.toUpper());
}

void drawChevron(QPainter& p, const QRectF& box, bool pointsLeft, const QColor& color) {
  QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  const qreal tipX = pointsLeft ? box.left() + 0.5 : box.right() - 0.5;
  const qreal backX = pointsLeft ? box.right() - 0.5 : box.left() + 0.5;
  QPainterPath path;
  path.moveTo(backX, box.top() + 1);
  path.lineTo(tipX, box.center().y());
  path.lineTo(backX, box.bottom() - 1);
  p.drawPath(path);
}

void drawCreateMark(QPainter& p, const QRectF& box, const QColor& color) {
  QPen pen(color, 1.4, Qt::SolidLine, Qt::RoundCap);
  p.setPen(pen);
  const qreal w = box.width();
  const qreal h = box.height();
  const qreal ruleWidth = w * 0.62;
  for (int i = 0; i < 3; ++i) {
    const qreal y = box.top() + h * (0.18 + i * 0.32);
    p.drawLine(QPointF(box.left() + 0.7, y), QPointF(box.left() + ruleWidth, y));
  }
  const qreal cx = box.left() + w - w * 0.16;
  const qreal cy = box.top() + h - h * 0.16;
  const qreal reach = w * 0.2;
  p.drawLine(QPointF(cx - reach, cy), QPointF(cx + reach, cy));
  p.drawLine(QPointF(cx, cy - reach), QPointF(cx, cy + reach));
}

void drawRenameMark(QPainter& p, const QRectF& box, const QColor& color) {
  QPen pen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  const qreal w = box.width();
  const qreal h = box.height();
  const qreal l = box.left();
  const qreal t = box.top();
  p.drawLine(QPointF(l + w * 0.2, t + h * 0.68), QPointF(l + w * 0.84, t + h * 0.06));
  p.drawLine(QPointF(l + w * 0.2, t + h * 0.68), QPointF(l + w * 0.36, t + h * 0.8));
  p.drawLine(QPointF(l + w * 0.36, t + h * 0.8), QPointF(l + w * 0.86, t + h * 0.28));
  p.drawLine(QPointF(l + w * 0.06, t + h * 0.96), QPointF(l + w * 0.94, t + h * 0.96));
}

void drawFooterSep(QPainter& p, const QRectF& r) {
  QLinearGradient g(r.topLeft(), r.bottomLeft());
  g.setColorAt(0, QColor(0, 0, 0, 179));
  g.setColorAt(0.5, withAlpha(T().coolSheen, 31));
  g.setColorAt(1, QColor(0, 0, 0, 179));
  p.fillRect(r, g);
}

void drawStatusDot(QPainter& p, QPointF c) {
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(T().inkFaint.red(), T().inkFaint.green(), T().inkFaint.blue(), 160));
  p.drawEllipse(c, 1.5, 1.5);
}

void drawScrollbar(QPainter& p, const QRectF& track, qreal thumbTop, qreal thumbH) {
  QPainterPath trough;
  trough.addRoundedRect(track, track.width() / 2, track.width() / 2);
  QLinearGradient tg(track.topLeft(), track.topRight());
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.6, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);
  QLinearGradient inset(track.topLeft(), track.topRight());
  inset.setColorAt(0, QColor(0, 0, 0, 242));
  inset.setColorAt(0.55, Qt::transparent);
  p.fillPath(trough, inset);
  p.setPen(QPen(withAlpha(T().coolSheen, 26), 1));
  p.drawLine(QPointF(track.right() - 0.5, track.top() + 1),
             QPointF(track.right() - 0.5, track.bottom() - 1));
  const QRectF thumb(track.left() + 1, track.top() + thumbTop, track.width() - 2, thumbH);
  QLinearGradient face(thumb.topLeft(), thumb.topRight());
  face.setColorAt(0, T().scrollThumbHi);
  face.setColorAt(0.52, T().scrollThumbMid);
  face.setColorAt(1, T().idleLedLo);
  fillRound(p, thumb, thumb.width() / 2, face);
  QLinearGradient gloss(thumb.topLeft(), thumb.bottomLeft());
  gloss.setColorAt(0, withAlpha(T().hoverLift, 128));
  gloss.setColorAt(0.35, withAlpha(T().hoverLift, 0));
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(thumb, thumb.width() / 2, thumb.width() / 2);
  p.setClipPath(clip);
  p.fillRect(thumb, gloss);
  const QRectF ridge(thumb.left() + 3, thumb.center().y() - 4, thumb.width() - 6, 8);
  for (qreal y = ridge.top(); y < ridge.bottom(); y += 2) {
    p.fillRect(QRectF(ridge.left(), y, ridge.width(), 1), QColor(0, 0, 0, 128));
    p.fillRect(QRectF(ridge.left(), y + 1, ridge.width(), 1), withAlpha(T().coolSheen, 61));
  }
  p.restore();
}

void drawDiscLogo(QPainter& p, const QRectF& disc, const QImage* logo, bool insets) {
  // Title bar: BoxShadow blurRadius 12 alpha 0x47. About badge: size*0.42 @ 0.3.
  const qreal bloomRadius = disc.width() <= 32 ? 12.0 : disc.width() * 0.42;
  const qreal bloomSigma = bloomRadius * 0.57735;
  const int bloomA = disc.width() <= 32 ? 0x47 : int(0.3 * 255);
  paintBlurred(p, disc.adjusted(-bloomRadius, -bloomRadius, bloomRadius, bloomRadius),
               bloomSigma, [&](QPainter& bp) {
                 bp.setPen(Qt::NoPen);
                 bp.setBrush(QColor(T().accent.red(), T().accent.green(), T().accent.blue(), bloomA));
                 bp.drawEllipse(disc);
               });
  const qreal dropY = disc.width() <= 32 ? 2.0 : 3.0;
  const qreal dropBlur = disc.width() <= 32 ? 4.0 : 7.0;
  paintBlurred(p, disc.adjusted(-dropBlur, -2, dropBlur, dropY + dropBlur),
               dropBlur * 0.57735, [&](QPainter& bp) {
                 bp.setPen(Qt::NoPen);
                 bp.setBrush(QColor(0, 0, 0, disc.width() <= 32 ? 0x8C : 0x99));
                 bp.drawEllipse(disc.translated(0, dropY));
               });
  p.setPen(Qt::NoPen);
  p.setBrush(T().logoDisc);
  p.drawEllipse(disc);
  if (logo && !logo->isNull()) {
    p.save();
    QPainterPath clip;
    clip.addEllipse(disc);
    p.setClipPath(clip);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF dest = disc.adjusted(-disc.width() * 0.06, -disc.height() * 0.06,
                                      disc.width() * 0.06, disc.height() * 0.06);
    p.drawImage(dest, *logo);
    p.restore();
  }
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 166), 1));
  p.drawEllipse(disc);
  if (!insets) {
    return;
  }
  p.save();
  QPainterPath clip;
  clip.addEllipse(disc);
  p.setClipPath(clip);
  QLinearGradient sheen(disc.topLeft(), QPointF(disc.left(), disc.top() + disc.height() * 0.55));
  sheen.setColorAt(0, QColor(255, 255, 255, 128));
  sheen.setColorAt(1, QColor(255, 255, 255, 0));
  p.fillRect(QRectF(disc.left(), disc.top(), disc.width(), disc.height() * 0.55), sheen);
  QLinearGradient shade(QPointF(disc.left(), disc.top() + disc.height() * 0.45),
                        disc.bottomLeft());
  shade.setColorAt(0, withAlpha(T().phosDeep, 0));
  shade.setColorAt(1, withAlpha(T().phosDeep, 89));
  p.fillRect(QRectF(disc.left(), disc.top() + disc.height() * 0.45, disc.width(),
                    disc.height() * 0.55),
             shade);
  QLinearGradient gloss(disc.topLeft() + QPointF(disc.width() * 0.2, 0),
                        disc.bottomRight() - QPointF(disc.width() * 0.1, disc.height() * 0.2));
  gloss.setColorAt(0, QColor(255, 255, 255, 115));
  gloss.setColorAt(0.46, QColor(255, 255, 255, 0));
  p.fillRect(disc, gloss);
  p.restore();
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
