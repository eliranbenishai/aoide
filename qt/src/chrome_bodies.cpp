#include "chrome_bodies.h"

#include "mockup_tokens.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>

namespace tramp {
namespace {

QRectF bodyRect(QSize logical) {
  return QRectF(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
}

QFont condensed(int px, qreal trackingEm = 0) {
  QFont f(chromeFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Bold);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

QFont mono(int px) {
  QFont f(lcdFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Medium);
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
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
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
  p.drawLine(QPointF(well.left() + 1, well.top()),
             QPointF(well.right() - 1, well.top()));
}

void drawBtn(QPainter& p, const QRectF& r, bool on, const QString& label) {
  QLinearGradient face(r.topLeft(), r.bottomLeft());
  if (on) {
    face.setColorAt(0, QColor(0xa3, 0xf4, 0xff));
    face.setColorAt(0.45, kPhos);
    face.setColorAt(1, QColor(0x12, 0x95, 0xa8));
  } else {
    face.setColorAt(0, QColor(0x3f, 0x46, 0x57));
    face.setColorAt(0.48, QColor(0x2b, 0x31, 0x3e));
    face.setColorAt(1, QColor(0x1e, 0x22, 0x2c));
  }
  fillRound(p, r, 4, face);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(r, 4, 4);
  p.setClipPath(clip);
  p.fillRect(QRectF(r.left() + 1, r.top() + 1, r.width() - 2, r.height() * 0.45),
             QColor(232, 240, 255, on ? 40 : 31));
  p.fillRect(QRectF(r.left(), r.bottom() - 2, r.width(), 2), QColor(0, 0, 0, 120));
  p.restore();
  if (!label.isEmpty()) {
    p.setFont(condensed(13, 0.18));
    p.setPen(on ? QColor(0x04, 0x22, 0x2b) : QColor(196, 210, 232, 184));
    p.drawText(r, Qt::AlignCenter, label);
  }
}

void drawGlyphBtn(QPainter& p, const QRectF& r, const QPainterPath& glyph, bool on) {
  drawBtn(p, r, on, {});
  p.save();
  p.translate(r.center());
  const QRectF gb = glyph.boundingRect();
  p.translate(-gb.center());
  p.setPen(Qt::NoPen);
  p.setBrush(on ? QColor(0x04, 0x22, 0x2b) : QColor(214, 226, 245, 216));
  p.drawPath(glyph);
  p.restore();
}

QPainterPath playGlyph() {
  QPainterPath path;
  path.moveTo(0, 0);
  path.lineTo(10, 7);
  path.lineTo(0, 14);
  path.closeSubpath();
  return path;
}

QPainterPath pauseGlyph() {
  QPainterPath path;
  path.addRect(0, 0, 3.5, 14);
  path.addRect(7.5, 0, 3.5, 14);
  return path;
}

QPainterPath stopGlyph() {
  QPainterPath path;
  path.addRect(0, 0, 12, 12);
  return path;
}

QPainterPath prevGlyph() {
  QPainterPath path;
  path.addRect(0, 0, 2.4, 14);
  path.moveTo(12, 0);
  path.lineTo(3, 7);
  path.lineTo(12, 14);
  path.closeSubpath();
  return path;
}

QPainterPath nextGlyph() {
  QPainterPath path;
  path.moveTo(0, 0);
  path.lineTo(9, 7);
  path.lineTo(0, 14);
  path.closeSubpath();
  path.addRect(9.6, 0, 2.4, 14);
  return path;
}

QPainterPath ejectGlyph() {
  QPainterPath path;
  path.moveTo(6, 0);
  path.lineTo(12, 8);
  path.lineTo(0, 8);
  path.closeSubpath();
  path.addRect(0, 10, 12, 2.5);
  return path;
}

void drawSlider(QPainter& p, const QRectF& track, qreal t) {
  QPainterPath trough;
  trough.addRoundedRect(track, track.height() / 2, track.height() / 2);
  p.fillPath(trough, QColor(0x06, 0x07, 0x0a));
  QRectF fill = track;
  fill.setWidth(qMax(8.0, track.width() * t));
  QLinearGradient g(fill.topLeft(), fill.bottomLeft());
  g.setColorAt(0, QColor(0xcb, 0xf9, 0xff));
  g.setColorAt(1, QColor(0x0f, 0x7f, 0x96));
  QPainterPath fillPath;
  fillPath.addRoundedRect(fill, fill.height() / 2, fill.height() / 2);
  p.fillPath(fillPath, g);
  const qreal x = track.left() + track.width() * t;
  const QRectF thumb(x - 10, track.center().y() - 15, 20, 30);
  QLinearGradient tg(thumb.topLeft(), thumb.bottomLeft());
  tg.setColorAt(0, QColor(0x6f, 0x76, 0x88));
  tg.setColorAt(0.4, QColor(0x3d, 0x43, 0x50));
  tg.setColorAt(1, QColor(0x22, 0x26, 0x2f));
  fillRound(p, thumb, 4, tg);
}

void drawPlate(QPainter& p, const QRectF& r) {
  fillRound(p, r, 4, QColor(0x1e, 0x22, 0x2c));
  p.setPen(QPen(QColor(226, 236, 255, 26), 1));
  p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
  for (qreal y = r.top() + 2; y < r.bottom() - 2; y += 3) {
    p.fillRect(QRectF(r.left() + 2, y, r.width() - 4, 1), QColor(226, 236, 255, 10));
  }
}

void drawLed(QPainter& p, QPointF c, bool on) {
  if (on) {
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 90));
    p.drawEllipse(c, 6, 6);
    p.setBrush(kAccent);
  } else {
    p.setBrush(QColor(0x3a, 0x40, 0x4c));
  }
  p.setPen(QPen(QColor(0, 0, 0, 160), 1));
  p.drawEllipse(c, 3.2, 3.2);
}

void paintMain(QPainter& p, const QRectF& body) {
  const QRectF well(body.left() + 96, body.top() + 14, 705, 132);
  drawScreen(p, well);

  drawBtn(p, QRectF(body.left() + 22, body.top() + 18, 26, 26), false, {});
  p.setPen(QColor(214, 226, 245, 200));
  p.setFont(condensed(14));
  p.drawText(QRectF(body.left() + 22, body.top() + 18, 26, 26), Qt::AlignCenter,
             QStringLiteral("⋯"));

  p.setFont(mono(46));
  p.setPen(kPhos);
  p.drawText(QRectF(well.left() + 16, well.top() + 10, 180, 52),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("2:41"));
  p.setFont(condensed(12, 0.22));
  p.setPen(QColor(61, 231, 255, 128));
  p.drawText(QRectF(well.left() + 168, well.top() + 28, 90, 20),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("ELAPSED"));

  const qreal bars[] = {0.26, 0.52, 0.71, 0.88, 0.64, 0.47, 0.58, 0.39, 0.31,
                        0.44, 0.35, 0.24, 0.29, 0.19, 0.22, 0.14, 0.17, 0.10,
                        0.12, 0.07};
  const qreal peaks[] = {0.44, 0.70, 0.88, 0.96, 0.80, 0.66, 0.74, 0.57, 0.52,
                         0.61, 0.55, 0.42, 0.47, 0.36, 0.40, 0.30, 0.33, 0.24,
                         0.27, 0.19};
  const QRectF viz(well.left() + 16, well.bottom() - 54, 248, 42);
  for (int i = 0; i < 20; ++i) {
    const qreal x = viz.left() + i * 12;
    const qreal h = viz.height() * bars[i];
    QRectF bar(x, viz.bottom() - h, 9, h);
    QLinearGradient g(bar.topLeft(), bar.bottomLeft());
    g.setColorAt(0, QColor(0xcb, 0xf9, 0xff));
    g.setColorAt(0.26, kPhos);
    g.setColorAt(0.62, QColor(0x1b, 0x9e, 0xc4));
    g.setColorAt(1, kAccent);
    p.fillRect(bar, g);
    const qreal py = viz.bottom() - viz.height() * peaks[i];
    p.fillRect(QRectF(x, py - 2, 9, 2), QColor(0xea, 0xff, 0xff));
  }

  p.setFont(condensed(24, 0.03));
  p.setPen(kPhosHot);
  p.drawText(QRectF(well.left() + 288, well.top() + 12, 400, 32),
             Qt::AlignLeft | Qt::AlignVCenter,
             QStringLiteral("3. Velvet Static — Neon Boulevard (Extended Mix)"));
  p.setFont(condensed(14, 0.14));
  p.setPen(QColor(61, 231, 255, 128));
  p.drawText(QRectF(well.left() + 288, well.top() + 44, 400, 20),
             Qt::AlignLeft | Qt::AlignVCenter,
             QStringLiteral("COPPER RAIN EP · TRACK 3 OF 12"));

  p.setFont(mono(13));
  p.setPen(QColor(61, 231, 255, 128));
  p.drawText(QRectF(well.left() + 288, well.bottom() - 28, 80, 18),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("192 kbps"));
  p.drawText(QRectF(well.left() + 378, well.bottom() - 28, 80, 18),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("44.1 kHz"));
  p.setFont(condensed(12, 0.2));
  p.setPen(kPhos);
  p.drawText(QRectF(well.left() + 470, well.bottom() - 28, 70, 18),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("STEREO"));
  QRectF fmt(well.right() - 52, well.bottom() - 30, 40, 18);
  QLinearGradient badge(fmt.topLeft(), fmt.bottomLeft());
  badge.setColorAt(0, QColor(0xff, 0xb3, 0xd4));
  badge.setColorAt(0.55, kAccent);
  badge.setColorAt(1, QColor(0xb8, 0x22, 0x6a));
  fillRound(p, fmt, 2, badge);
  p.setPen(QColor(0x2b, 0x06, 0x16));
  p.drawText(fmt, Qt::AlignCenter, QStringLiteral("MP3"));

  const QRectF volRow(body.left() + 22, body.top() + 156, body.width() - 44, 40);
  drawGlyphBtn(p, QRectF(volRow.left(), volRow.top(), 40, 40), playGlyph(), false);
  p.setFont(condensed(11, 0.2));
  p.setPen(kInkFaint);
  p.drawText(QRectF(volRow.left() + 50, volRow.top(), 34, 40), Qt::AlignVCenter,
             QStringLiteral("VOL"));
  drawSlider(p, QRectF(volRow.left() + 90, volRow.center().y() - 7, 360, 14), 0.66);
  drawBtn(p, QRectF(volRow.right() - 248, volRow.top() + 1, 86, 38), false,
          QStringLiteral("MONO"));
  drawBtn(p, QRectF(volRow.right() - 154, volRow.top() + 1, 74, 38), true,
          QStringLiteral("EQ"));
  drawBtn(p, QRectF(volRow.right() - 72, volRow.top() + 1, 74, 38), true,
          QStringLiteral("PL"));

  const QRectF seekRow(body.left() + 22, body.top() + 206, body.width() - 44, 32);
  p.setFont(mono(14));
  p.setPen(kInkDim);
  p.drawText(QRectF(seekRow.left(), seekRow.top(), 48, 32), Qt::AlignVCenter,
             QStringLiteral("2:41"));
  drawSlider(p, QRectF(seekRow.left() + 56, seekRow.center().y() - 8, seekRow.width() - 120, 16),
             0.46);
  p.drawText(QRectF(seekRow.right() - 56, seekRow.top(), 56, 32),
             Qt::AlignVCenter | Qt::AlignRight, QStringLiteral("5:47"));

  const QRectF playRow(body.left() + 22, body.top() + 246, body.width() - 44, 50);
  qreal x = playRow.left();
  auto place = [&](qreal w, const QPainterPath& g, bool on) {
    drawGlyphBtn(p, QRectF(x, playRow.top(), w, 50), g, on);
    x += w + 6;
  };
  place(66, prevGlyph(), false);
  place(78, pauseGlyph(), true);
  place(66, stopGlyph(), false);
  place(66, nextGlyph(), false);
  x += 10;
  place(66, ejectGlyph(), false);
  const QRectF rail(x + 6, playRow.center().y() - 11, 210, 22);
  drawPlate(p, rail);
  QRectF shuffle(playRow.right() - 210, playRow.top(), 100, 50);
  QRectF repeat(playRow.right() - 100, playRow.top(), 100, 50);
  drawBtn(p, shuffle, false, {});
  drawBtn(p, repeat, false, {});
  drawLed(p, QPointF(shuffle.left() + 16, shuffle.center().y()), false);
  drawLed(p, QPointF(repeat.left() + 16, repeat.center().y()), true);
  p.setFont(condensed(13, 0.16));
  p.setPen(QColor(196, 210, 232, 184));
  p.drawText(shuffle.adjusted(28, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("SHUFFLE"));
  p.drawText(repeat.adjusted(28, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("REPEAT"));
}

void paintEq(QPainter& p, const QRectF& body) {
  drawBtn(p, QRectF(body.left() + 22, body.top() + 16, 64, 38), true, QStringLiteral("ON"));
  drawBtn(p, QRectF(body.left() + 94, body.top() + 16, 72, 38), false, QStringLiteral("AUTO"));
  drawBtn(p, QRectF(body.left() + 174, body.top() + 16, 96, 38), false,
          QStringLiteral("PRESETS"));
  p.setFont(condensed(12, 0.18));
  p.setPen(kInkFaint);
  p.drawText(QRectF(body.left() + 280, body.top() + 16, 140, 38), Qt::AlignVCenter,
             QStringLiteral("CURVE · LATE NIGHT"));
  drawScreen(p, QRectF(body.right() - 394, body.top() + 16, 372, 62));
  p.setPen(QPen(kPhos, 1.6));
  QPainterPath curve;
  curve.moveTo(body.right() - 380, body.top() + 48);
  curve.cubicTo(body.right() - 280, body.top() + 28, body.right() - 200, body.top() + 70,
                body.right() - 36, body.top() + 40);
  p.setBrush(Qt::NoBrush);
  p.drawPath(curve);

  const char* labels[] = {"PREAMP", "60", "170", "310", "600", "1k",
                          "3k",     "6k", "12k", "14k", "16k"};
  const qreal gains[] = {0.2, 0.65, 0.78, 0.42, 0.5, 0.72, 0.38, 0.55, 0.48, 0.6, 0.44};
  p.setFont(mono(11));
  p.setPen(kInkFaint);
  p.drawText(QRectF(body.left() + 22, body.top() + 96, 44, 20), Qt::AlignRight,
             QStringLiteral("+12"));
  p.drawText(QRectF(body.left() + 22, body.top() + 170, 44, 20), Qt::AlignRight,
             QStringLiteral("0"));
  p.drawText(QRectF(body.left() + 22, body.top() + 244, 44, 20), Qt::AlignRight,
             QStringLiteral("-12"));

  qreal x = body.left() + 72;
  for (int i = 0; i < 11; ++i) {
    const qreal w = i == 0 ? 62 : 50;
    const QRectF track(x + w / 2 - 6, body.top() + 100, 12, 150);
    QLinearGradient trough(track.topLeft(), track.topRight());
    trough.setColorAt(0, QColor(0x06, 0x07, 0x0a));
    trough.setColorAt(1, QColor(0x1e, 0x22, 0x2c));
    fillRound(p, track, 6, trough);
    const qreal t = gains[i];
    QRectF fill(track.left(), track.bottom() - track.height() * t, 12,
                track.height() * t);
    QLinearGradient g(fill.topLeft(), fill.bottomLeft());
    g.setColorAt(0, QColor(0xcb, 0xf9, 0xff));
    g.setColorAt(1, kPhos);
    p.fillRect(fill, g);
    const QRectF thumb(track.center().x() - 16, fill.top() - 6, 32, 14);
    QLinearGradient tg(thumb.topLeft(), thumb.bottomLeft());
    tg.setColorAt(0, QColor(0x6f, 0x76, 0x88));
    tg.setColorAt(1, QColor(0x22, 0x26, 0x2f));
    fillRound(p, thumb, 3, tg);
    p.fillRect(QRectF(thumb.center().x() - 8, thumb.center().y() - 1, 16, 2), kPhos);
    p.setFont(condensed(10, 0.12));
    p.setPen(i == 0 ? kPhos : kInkFaint);
    p.drawText(QRectF(x, body.top() + 256, w, 20), Qt::AlignHCenter,
               QString::fromLatin1(labels[i]));
    x += w + (i == 0 ? 16 : 0);
  }
}

void paintPlaylist(QPainter& p, const QRectF& body) {
  const QRectF inner = body.adjusted(12, 12, -12, -12);
  const QRectF collection(inner.left(), inner.top(), 240, inner.height() - 110);
  const QRectF tracks(inner.left() + 248, inner.top(), inner.width() - 248,
                      inner.height() - 110);
  const QRectF footer(inner.left(), inner.bottom() - 98, inner.width(), 98);
  drawPlate(p, collection);
  p.setFont(condensed(12, 0.18));
  p.setPen(kInkFaint);
  p.drawText(collection.adjusted(12, 10, -12, 0), Qt::AlignTop | Qt::AlignLeft,
             QStringLiteral("COLLECTION"));
  const QString lists[] = {QStringLiteral("Copper Rain EP"),
                           QStringLiteral("Night Drive"),
                           QStringLiteral("Studio Mix 03")};
  for (int i = 0; i < 3; ++i) {
    QRectF row(collection.left() + 8, collection.top() + 36 + i * 36, collection.width() - 16,
               32);
    if (i == 0) {
      fillRound(p, row, 3, QColor(61, 231, 255, 28));
    }
    p.setPen(i == 0 ? kPhos : kInk);
    p.setFont(condensed(13, 0.06));
    p.drawText(row.adjusted(8, 0, -8, 0), Qt::AlignVCenter, lists[i]);
  }

  drawScreen(p, tracks);
  p.setFont(condensed(13, 0.04));
  const QString rows[] = {
      QStringLiteral("1  Velvet Static — Neon Boulevard"),
      QStringLiteral("2  Copper Rain — Glass Harbor"),
      QStringLiteral("3  Night Orchid — Static Bloom"),
      QStringLiteral("4  Ivory Circuit — Afterimage"),
  };
  for (int i = 0; i < 4; ++i) {
    p.setPen(i == 0 ? kPhosHot : QColor(232, 234, 240, 180));
    p.drawText(QRectF(tracks.left() + 16, tracks.top() + 16 + i * 28, tracks.width() - 32, 24),
               Qt::AlignVCenter, rows[i]);
  }

  drawPlate(p, footer);
  drawBtn(p, QRectF(footer.left() + 12, footer.center().y() - 19, 86, 38), false,
          QStringLiteral("ADD"));
  drawBtn(p, QRectF(footer.left() + 106, footer.center().y() - 19, 96, 38), false,
          QStringLiteral("REMOVE"));
  p.setFont(condensed(12, 0.2));
  p.setPen(kInkFaint);
  p.drawText(footer.adjusted(0, 0, -16, 0), Qt::AlignVCenter | Qt::AlignRight,
             QStringLiteral("TOTAL  54:12"));
}

void paintSettings(QPainter& p, const QRectF& body) {
  const QRectF inner = body.adjusted(16, 16, -16, -16);
  drawBtn(p, QRectF(inner.left(), inner.top(), 96, 32), true, QStringLiteral("GENERAL"));
  drawBtn(p, QRectF(inner.left() + 104, inner.top(), 80, 32), false, QStringLiteral("SKINS"));
  drawScreen(p, inner.adjusted(0, 48, 0, 0));
  p.setFont(condensed(13, 0.14));
  p.setPen(QColor(61, 231, 255, 160));
  const QString rows[] = {QStringLiteral("Always on top"), QStringLiteral("Force mono"),
                          QStringLiteral("Resume last session"),
                          QStringLiteral("Global zoom        75%")};
  for (int i = 0; i < 4; ++i) {
    p.drawText(QRectF(inner.left() + 20, inner.top() + 72 + i * 36, inner.width() - 40, 28),
               Qt::AlignVCenter, rows[i]);
  }
}

void paintAbout(QPainter& p, const QRectF& body) {
  p.setFont(condensed(28, 0.12));
  p.setPen(kWordmark);
  p.drawText(QRectF(body.left() + 24, body.top() + 16, 280, 36), Qt::AlignVCenter,
             QStringLiteral("TRAMP"));
  p.setFont(condensed(11, 0.16));
  p.setPen(kPhos);
  p.drawText(QRectF(body.left() + 24, body.top() + 52, 400, 20), Qt::AlignVCenter,
             QStringLiteral("THE RIDICULOUSLY ATTRACTIVE MUSIC PLAYER"));
  p.setFont(condensed(12, 0.14));
  p.setPen(kInkDim);
  p.drawText(QRectF(body.left() + 24, body.top() + 74, 400, 18), Qt::AlignVCenter,
             QStringLiteral("Local files, honest tags, and chrome you can feel."));

  const QRectF well = QRectF(body.left() + 20, body.top() + 108, body.width() - 40, 140);
  drawScreen(p, well);
  p.setFont(condensed(11, 0.2));
  p.setPen(kPhos);
  p.drawText(well.adjusted(0, 10, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
             QStringLiteral("ON THIS MACHINE"));
  p.setFont(mono(14));
  p.setPen(kPhosHot);
  const QString stats[] = {QStringLiteral("PLAYLISTS ………… 12"),
                           QStringLiteral("TRACKS …………… 1,284"),
                           QStringLiteral("TOTAL TIME …… 3 d 22 h"),
                           QStringLiteral("SPINS …………… 4,096")};
  for (int i = 0; i < 4; ++i) {
    p.drawText(QRectF(well.left() + 24, well.top() + 36 + i * 24, well.width() - 48, 22),
               Qt::AlignVCenter, stats[i]);
  }
}

}  // namespace

void paintWindowBody(QPainter& painter, WindowId id, QSize logical) {
  const QRectF body = bodyRect(logical);
  switch (id) {
    case WindowId::main:
      paintMain(painter, body);
      break;
    case WindowId::equalizer:
      paintEq(painter, body);
      break;
    case WindowId::playlist:
      paintPlaylist(painter, body);
      break;
    case WindowId::settings:
      paintSettings(painter, body);
      break;
    case WindowId::about:
      paintAbout(painter, body);
      break;
  }
}

}  // namespace tramp
