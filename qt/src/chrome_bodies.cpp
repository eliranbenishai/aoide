#include "chrome_bodies.h"

#include "mockup_draw.h"
#include "mockup_tokens.h"
#include "tramp_metrics.h"

#include <QFontMetrics>
#include <QPainterPath>
#include <QVector>

namespace tramp {
namespace {

QRectF bodyRect(QSize logical) {
  return QRectF(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
}

QString formatGain(qreal gain) {
  if (qAbs(gain) < 0.05) {
    return QStringLiteral("0.0");
  }
  const QChar sign = gain > 0 ? QChar('+') : QChar(0x2212);
  return sign + QString::number(qAbs(gain), 'f', 1);
}

void drawLogoMark(QPainter& p, const QRectF& box, const QImage* logo, qreal opacity) {
  if (!logo || logo->isNull() || opacity <= 0) {
    return;
  }
  p.save();
  p.setOpacity(opacity);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  QRectF dest = box;
  const qreal srcAspect = qreal(logo->width()) / qMax(1, logo->height());
  if (srcAspect > 1) {
    dest.setHeight(box.width() / srcAspect);
    dest.moveTop(box.center().y() - dest.height() / 2);
  } else if (srcAspect < 1) {
    dest.setWidth(box.height() * srcAspect);
    dest.moveLeft(box.center().x() - dest.width() / 2);
  }
  p.drawImage(dest, *logo);
  p.restore();
}

void paintMain(QPainter& p, const QRectF& body) {
  const QRectF well(body.left() + 96, body.top() + 14, 705, 132);
  drawScreen(p, well);

  drawGlyphBtn(p, QRectF(body.left() + 22, body.top() + 18, 26, 26), MockupIcon::options,
               false, 16);

  const QRectF inner = well.adjusted(16, 12, -16, -12);
  drawGlowText(p, QRectF(inner.left(), inner.top(), 180, 48), QStringLiteral("2:41"),
               monoFont(46, 0.02), kPhos, QColor(61, 231, 255, 115), 4,
               Qt::AlignLeft | Qt::AlignVCenter);
  drawGlowText(p, QRectF(inner.left() + 168, inner.top() + 16, 90, 20),
               QStringLiteral("ELAPSED"), condensedFont(12, 0.22),
               QColor(61, 231, 255, 128), QColor(61, 231, 255, 64), 3,
               Qt::AlignLeft | Qt::AlignVCenter);

  const qreal bars[] = {0.26, 0.52, 0.71, 0.88, 0.64, 0.47, 0.58, 0.39, 0.31,
                        0.44, 0.35, 0.24, 0.29, 0.19, 0.22, 0.14, 0.17, 0.10,
                        0.12, 0.07};
  const qreal peaks[] = {0.44, 0.70, 0.88, 0.96, 0.80, 0.66, 0.74, 0.57, 0.52,
                         0.61, 0.55, 0.42, 0.47, 0.36, 0.40, 0.30, 0.33, 0.24,
                         0.27, 0.19};
  const QRectF viz(inner.left(), inner.bottom() - 42, 248, 42);
  for (int i = 0; i < 20; ++i) {
    const qreal x = viz.left() + i * 12;
    const qreal h = viz.height() * bars[i];
    QRectF bar(x, viz.bottom() - h, 9, h);
    QLinearGradient g(bar.topLeft(), bar.bottomLeft());
    g.setColorAt(0, kSpectrum0);
    g.setColorAt(0.26, kPhos);
    g.setColorAt(0.62, kSpectrum2);
    g.setColorAt(1, kAccent);
    p.fillRect(bar, g);
    const qreal py = viz.bottom() - viz.height() * peaks[i];
    p.fillRect(QRectF(x, py, 9, 2), QColor(0xea, 0xff, 0xff));
  }

  const QRectF meta(inner.left() + 268 + 20, inner.top(),
                    inner.width() - 288, inner.height());
  p.save();
  p.setClipRect(meta);
  drawGlowText(p, QRectF(meta.left(), meta.top(), 520, 28),
               QStringLiteral("3. Velvet Static — Neon Boulevard (Extended Mix)"),
               condensedFont(24, 0.03), kPhosHot, QColor(61, 231, 255, 128), 3,
               Qt::AlignLeft | Qt::AlignVCenter);
  p.restore();
  QLinearGradient fade(QPointF(meta.right() - 48, meta.top()), QPointF(meta.right(), meta.top()));
  fade.setColorAt(0, Qt::transparent);
  fade.setColorAt(1, QColor(0x07, 0x10, 0x18));
  p.fillRect(QRectF(meta.right() - 48, meta.top(), 48, 32), fade);

  drawGlowText(p, QRectF(meta.left(), meta.top() + 32, meta.width(), 18),
               QStringLiteral("COPPER RAIN EP · TRACK 3 OF 12"),
               condensedFont(14, 0.14), QColor(61, 231, 255, 128),
               QColor(61, 231, 255, 64), 2, Qt::AlignLeft | Qt::AlignVCenter);

  const qreal metaY = inner.bottom() - 18;
  p.setFont(monoFont(13, 0.04));
  p.setPen(QColor(61, 231, 255, 128));
  p.drawText(QRectF(meta.left(), metaY, 80, 18), Qt::AlignVCenter, QStringLiteral("192 kbps"));
  p.drawText(QRectF(meta.left() + 102, metaY, 80, 18), Qt::AlignVCenter,
             QStringLiteral("44.1 kHz"));
  drawGlowText(p, QRectF(meta.left() + 204, metaY, 70, 18), QStringLiteral("STEREO"),
               condensedFont(12, 0.2), kPhos, QColor(61, 231, 255, 115), 3,
               Qt::AlignVCenter | Qt::AlignLeft);

  const QRectF chip(inner.right() - 132, inner.bottom() - 22, 72, 16);
  p.setPen(kPhos);
  drawReload(p, QRectF(chip.left(), chip.center().y() - 5.5, 11, 11), kPhos);
  p.setFont(condensedFont(12, 0.14));
  p.drawText(QRectF(chip.left() + 15, chip.top(), 60, 16), Qt::AlignVCenter,
             QStringLiteral("PLAYLIST"));

  QRectF fmt(inner.right() - 48, inner.bottom() - 22, 40, 18);
  QLinearGradient badge(fmt.topLeft(), fmt.bottomLeft());
  badge.setColorAt(0, QColor(0xff, 0xb3, 0xd4));
  badge.setColorAt(0.55, kAccent);
  badge.setColorAt(1, QColor(0xb8, 0x22, 0x6a));
  fillRound(p, fmt, 2, badge);
  p.setPen(QColor(0x2b, 0x06, 0x16));
  p.setFont(condensedFont(12, 0.18));
  p.drawText(fmt, Qt::AlignCenter, QStringLiteral("MP3"));

  const QRectF volRow(body.left() + 22, body.top() + 156, body.width() - 44, 40);
  drawGlyphBtn(p, QRectF(volRow.left(), volRow.top(), 40, 40), MockupIcon::mute, false, 21);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(kInkFaint);
  p.drawText(QRectF(volRow.left() + 54, volRow.top(), 34, 40), Qt::AlignVCenter,
             QStringLiteral("VOL"));
  drawSlider(p, QRectF(volRow.left() + 98, volRow.center().y() - 7,
                       volRow.width() - 98 - 14 - 86 - 14 - 74 - 8 - 74, 14),
             0.66);
  drawBtn(p, QRectF(volRow.right() - 86 - 14 - 74 - 8 - 74, volRow.top() + 1, 86, 38), false,
          QStringLiteral("MONO"));
  drawBtn(p, QRectF(volRow.right() - 74 - 8 - 74, volRow.top() + 1, 74, 38), true,
          QStringLiteral("EQ"));
  drawBtn(p, QRectF(volRow.right() - 74, volRow.top() + 1, 74, 38), true,
          QStringLiteral("PL"));

  const QRectF seekRow(body.left() + 22, body.top() + 206, body.width() - 44, 32);
  p.setFont(monoFont(14));
  p.setPen(kInkDim);
  p.drawText(QRectF(seekRow.left(), seekRow.top(), 48, 32), Qt::AlignVCenter,
             QStringLiteral("2:41"));
  drawSlider(p, QRectF(seekRow.left() + 62, seekRow.center().y() - 8,
                       seekRow.width() - 62 - 62, 16),
             161.0 / 347.0, true);
  p.drawText(QRectF(seekRow.right() - 48, seekRow.top(), 48, 32),
             Qt::AlignVCenter | Qt::AlignRight, QStringLiteral("5:47"));

  const QRectF playRow(body.left() + 22, body.top() + 246, body.width() - 44, 50);
  qreal x = playRow.left();
  auto place = [&](qreal w, MockupIcon icon, bool on) {
    drawGlyphBtn(p, QRectF(x, playRow.top(), w, 50), icon, on, 22);
    x += w + 6;
  };
  place(66, MockupIcon::previous, false);
  place(78, MockupIcon::play, false);
  place(66, MockupIcon::pause, true);
  place(66, MockupIcon::stop, false);
  place(66, MockupIcon::next, false);
  x += 10;
  place(66, MockupIcon::eject, false);
  x += 6;
  const qreal railW = playRow.right() - 210 - 12 - x;
  if (railW > 8) {
    drawRail(p, QRectF(x, playRow.center().y() - 11, railW, 22));
  }
  const QRectF shuffle(playRow.right() - 210, playRow.top(), 100, 50);
  const QRectF repeat(playRow.right() - 100, playRow.top(), 100, 50);
  drawBtn(p, shuffle, false, {});
  drawBtn(p, repeat, false, {});
  drawLed(p, QPointF(shuffle.left() + 22, shuffle.center().y()), true);
  drawLed(p, QPointF(repeat.left() + 22, repeat.center().y()), true);
  p.setFont(condensedFont(13, 0.16));
  p.setPen(QColor(196, 210, 232, 184));
  p.drawText(shuffle.adjusted(36, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("SHUFFLE"));
  p.drawText(repeat.adjusted(36, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("REPEAT"));
}

void paintEq(QPainter& p, const QRectF& body, const QImage* logo) {
  drawBtn(p, QRectF(body.left() + 22, body.top() + 16, 56, 38), true, QStringLiteral("ON"));
  drawBtn(p, QRectF(body.left() + 86, body.top() + 16, 64, 38), false, QStringLiteral("AUTO"));
  const QRectF presets(body.left() + 158, body.top() + 16, 108, 38);
  drawBtn(p, presets, false, QStringLiteral("PRESETS"));
  drawMenuCaret(p, presets);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(kInkFaint);
  p.drawText(QRectF(body.left() + 280, body.top() + 16, 140, 38), Qt::AlignVCenter,
             QStringLiteral("CURVE · LATE NIGHT"));

  const QRectF curveWell(body.right() - 22 - 372, body.top() + 16, 372, 62);
  drawScreen(p, curveWell);
  const qreal preamp = 3.8;
  const qreal gains[] = {6.2, 4.6, 1.0, -1.9, -0.5, 2.2, 3.4, 1.4, 0.0, 5.0};
  QVector<QPointF> pts;
  pts.reserve(11);
  auto yFor = [&](qreal g) {
    const qreal t = (12.0 - qBound(-12.0, g, 12.0)) / 24.0;
    return curveWell.top() + t * curveWell.height();
  };
  pts.push_back(QPointF(curveWell.left(), yFor(preamp)));
  for (int i = 0; i < 10; ++i) {
    pts.push_back(QPointF(curveWell.left() + curveWell.width() * (i + 1) / 10.0,
                          yFor(gains[i])));
  }
  p.setPen(QPen(QColor(226, 236, 255, 36), 1));
  p.drawLine(QPointF(curveWell.left(), curveWell.center().y()),
             QPointF(curveWell.right(), curveWell.center().y()));
  QPainterPath curve;
  curve.moveTo(pts.first());
  for (int i = 0; i < pts.size() - 1; ++i) {
    const QPointF p0 = i == 0 ? pts[i] : pts[i - 1];
    const QPointF p1 = pts[i];
    const QPointF p2 = pts[i + 1];
    const QPointF p3 = i + 2 < pts.size() ? pts[i + 2] : p2;
    curve.cubicTo(QPointF(p1.x() + (p2.x() - p0.x()) / 6, p1.y() + (p2.y() - p0.y()) / 6),
                  QPointF(p2.x() - (p3.x() - p1.x()) / 6, p2.y() - (p3.y() - p1.y()) / 6),
                  p2);
  }
  QPainterPath fill = curve;
  fill.lineTo(curveWell.bottomRight());
  fill.lineTo(curveWell.bottomLeft());
  fill.closeSubpath();
  QLinearGradient wash(curveWell.topLeft(), curveWell.bottomLeft());
  wash.setColorAt(0, QColor(61, 231, 255, 102));
  wash.setColorAt(1, QColor(61, 231, 255, 0));
  p.fillPath(fill, wash);
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(61, 231, 255, 153), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.drawPath(curve);
  p.setPen(QPen(kCurveStroke, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.drawPath(curve);

  const char* labels[] = {"PREAMP", "60", "170", "310", "600", "1k",
                          "3k",     "6k", "12k", "14k", "16k"};
  qreal allGains[11] = {preamp};
  for (int i = 0; i < 10; ++i) {
    allGains[i + 1] = gains[i];
  }

  const QRectF bandRow(body.left() + 22, body.top() + 92, body.width() - 44, 196);
  p.setFont(monoFont(11));
  p.setPen(kInkFaint);
  p.drawText(QRectF(bandRow.left(), bandRow.top() + 18, 36, 14),
             Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("+12"));
  p.drawText(QRectF(bandRow.left(), bandRow.top() + 18 + 67, 36, 14),
             Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("0"));
  p.drawText(QRectF(bandRow.left(), bandRow.top() + 18 + 134, 36, 14),
             Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("−12"));

  qreal x = bandRow.left() + 44;
  for (int i = 0; i < 11; ++i) {
    const qreal w = i == 0 ? 62 : 50;
    p.setFont(monoFont(11));
    p.setPen(kInk);
    p.drawText(QRectF(x, bandRow.top(), w, 18), Qt::AlignHCenter | Qt::AlignVCenter,
               formatGain(allGains[i]));
    drawVBand(p, QRectF(x, bandRow.top() + 18, w, 148), allGains[i]);
    p.setFont(condensedFont(11, i == 0 ? 0.18 : 0.1));
    p.setPen(i == 0 ? QColor(61, 231, 255, 140) : kInkFaint);
    p.drawText(QRectF(x, bandRow.top() + 166, w, 26), Qt::AlignHCenter | Qt::AlignVCenter,
               QString::fromLatin1(labels[i]));
    x += w + (i == 0 ? 16 : 0);
  }

  drawLogoMark(p, QRectF(body.right() - 36 - 120, body.top() + 120, 120, 120), logo, 0.14);
}

void paintPlaylist(QPainter& p, const QRectF& body, const QImage* logo) {
  const qreal collectionW = 240;
  const QRectF collection(body.left(), body.top(), collectionW, body.height());
  const QRectF tracks(body.left() + collectionW + 8, body.top(),
                      body.width() - collectionW - 8, body.height());
  p.fillRect(QRectF(collection.right(), collection.top(), 8, collection.height()), kShellDeep);

  const QRectF colInner = collection.adjusted(12, 12, -6, -12);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(kInkFaint);
  p.drawText(QRectF(colInner.left(), colInner.top(), colInner.width() - 30, 20),
             Qt::AlignVCenter, QStringLiteral("PLAYLISTS"));
  drawGlyphBtn(p, QRectF(colInner.right() - 24, colInner.top(), 24, 20), MockupIcon::previous,
               false, 10);

  const QRectF colWell(colInner.left(), colInner.top() + 30, colInner.width(),
                       colInner.height() - 30 - 32);
  drawScreen(p, colWell);
  struct ListRow {
    const char* name;
    const char* count;
    bool selected;
  };
  const ListRow lists[] = {
      {"ANALOGUE GHOSTS", "24", false},
      {"COPPER RAIN EP", "13", true},
      {"NIGHTBUS CHOIR — LIVE", "8", false},
  };
  for (int i = 0; i < 3; ++i) {
    QRectF row(colWell.left(), colWell.top() + 4 + i * 26, colWell.width(), 26);
    if (lists[i].selected) {
      QLinearGradient g(row.topLeft(), row.bottomLeft());
      g.setColorAt(0, QColor(61, 231, 255, 33));
      g.setColorAt(1, QColor(61, 231, 255, 10));
      p.fillRect(row, g);
    }
    p.setFont(condensedFont(11, 0.1));
    p.setPen(lists[i].selected ? kPhosHot : kInkDim);
    p.drawText(row.adjusted(10, 0, -36, 0), Qt::AlignVCenter,
               QString::fromUtf8(lists[i].name));
    p.setFont(monoFont(12));
    p.setPen(lists[i].selected ? kPhos : kPhosDim);
    p.drawText(row.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignRight,
               QString::fromLatin1(lists[i].count));
  }

  qreal cx = colInner.left();
  const qreal cy = colInner.bottom() - 24;
  auto cbtn = [&](MockupIcon icon, bool menu) {
    const QRectF r(cx, cy, 30, 24);
    drawGlyphBtn(p, r, icon, false, 13);
    if (menu) {
      drawMenuCaret(p, r);
    }
    cx += 36;
  };
  cbtn(MockupIcon::add, false);
  cbtn(MockupIcon::add, true);
  cbtn(MockupIcon::options, false);
  cbtn(MockupIcon::remove, false);

  const QRectF trackInner = tracks.adjusted(12, 12, -12, -12);
  const QRectF listWell(trackInner.left(), trackInner.top(), trackInner.width() - 24,
                        trackInner.height() - 110);
  drawScreen(p, listWell);
  struct TrackRow {
    const char* artist;
    const char* title;
    const char* time;
  };
  const TrackRow rows[] = {
      {"Cassette Mirage", "Low Orbit Lullaby", "4:12"},
      {"The Brass Cassini", "Slow Dial", "3:38"},
      {"Velvet Static", "Neon Boulevard (Extended Mix)", "5:47"},
      {"Halogen Youth", "Parking Garage Sunset", "4:03"},
      {"Moth & Marrow", "Analogue Ghosts", "6:21"},
      {"Ruby Transit", "Bakelite Heart", "3:55"},
      {"Slow Signal", "Copper Rain", "4:44"},
      {"Aurora Kiosk", "Departure Lounge B", "5:09"},
      {"Pale Antenna", "Tramp Theme (Demo)", "2:58"},
      {"Nightbus Choir", "Fluorescent Hymn", "6:02"},
      {"Second Cassette", "Static Blonde", "3:27"},
      {"Velvet Static", "Neon Boulevard (Reprise)", "2:02"},
      {"Long Wave Motel", "Untitled Sketch", "3:16"},
  };
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(listWell, 3, 3);
  p.setClipPath(clip);
  for (int i = 0; i < 13; ++i) {
    const QRectF row(listWell.left(), listWell.top() + 6 + i * 37, listWell.width(), 37);
    const bool playing = i == 2;
    const QColor color = playing ? kPhosHot : QColor(0x9a, 0xe2, 0xf0, 115);
    if (playing) {
      QLinearGradient g(row.topLeft(), row.bottomLeft());
      g.setColorAt(0, QColor(61, 231, 255, 33));
      g.setColorAt(1, QColor(61, 231, 255, 10));
      p.fillRect(row, g);
      p.fillRect(QRectF(row.left(), row.top() + 6, 3, row.height() - 12), kAccent);
    }
    p.setFont(monoFont(15));
    p.setPen(playing ? kPhos : QColor(color.red(), color.green(), color.blue(), 179));
    p.drawText(QRectF(row.left() + 16, row.top(), 34, 37), Qt::AlignVCenter | Qt::AlignRight,
               QStringLiteral("%1.").arg(i + 1));
    p.setPen(color);
    p.drawText(QRectF(row.left() + 64, row.top(), row.width() - 130, 37), Qt::AlignVCenter,
               QStringLiteral("%1 — %2")
                   .arg(QString::fromUtf8(rows[i].artist), QString::fromUtf8(rows[i].title)));
    p.setPen(QColor(color.red(), color.green(), color.blue(), 204));
    p.drawText(QRectF(row.right() - 62, row.top(), 48, 37), Qt::AlignVCenter | Qt::AlignRight,
               QString::fromUtf8(rows[i].time));
  }
  drawLogoMark(p, QRectF(listWell.right() - 26 - 178, listWell.bottom() - 8 - 178, 178, 178),
               logo, 0.05);
  p.restore();
  drawRail(p, QRectF(listWell.right() + 10, listWell.top() + 8, 14, listWell.height() * 0.22));

  const QRectF footer(trackInner.left(), trackInner.bottom() - 110, trackInner.width(), 110);
  qreal fx = footer.left();
  auto fbtn = [&](qreal w, MockupIcon icon, bool menu) {
    const QRectF r(fx, footer.top(), w, 52);
    drawGlyphBtn(p, r, icon, false, 21);
    if (menu) {
      drawMenuCaret(p, r);
    }
    fx += w + 8;
  };
  fbtn(52, MockupIcon::add, false);
  fbtn(52, MockupIcon::remove, false);
  fbtn(52, MockupIcon::sort, true);
  fbtn(52, MockupIcon::options, true);
  const qreal transport = 52 * 3 + 16;
  const qreal totalW = 128;
  const qreal railW = footer.right() - totalW - 8 - transport - 8 - fx;
  if (railW > 4) {
    drawRail(p, QRectF(fx, footer.top() + 15, railW, 22));
  }
  fx = footer.right() - totalW - 8 - transport;
  fbtn(52, MockupIcon::previous, false);
  fbtn(52, MockupIcon::play, false);
  fbtn(52, MockupIcon::next, false);
  const QRectF total(footer.right() - totalW, footer.top(), totalW, 52);
  drawScreen(p, total);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(kPhosDim);
  p.drawText(total.adjusted(12, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("TOTAL"));
  drawGlowText(p, total.adjusted(0, 0, -12, 0), QStringLiteral("55:34"), monoFont(18), kPhos,
               QColor(61, 231, 255, 115), 2, Qt::AlignVCenter | Qt::AlignRight);

  p.setFont(condensedFont(11, 0.12));
  p.setPen(kInkFaint);
  const QRectF status(footer.left() + 6, footer.bottom() - 26, footer.width() - 28, 26);
  p.drawText(status, Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("COPPER RAIN — NIGHT SET.M3U8  ·  13 TRACKS  ·  PLAYING 3"));
  p.drawText(status, Qt::AlignVCenter | Qt::AlignRight,
             QStringLiteral("DROP FILES HERE TO ENQUEUE"));
}

void paintSettings(QPainter& p, const QRectF& body) {
  p.fillRect(QRectF(body.left(), body.top(), 108, body.height()), kShellDeep);
  auto tab = [&](qreal y, const QString& label, bool on) {
    const QRectF r(body.left(), y, 108, 42);
    if (on) {
      p.fillRect(r, QColor(kShellHi.red(), kShellHi.green(), kShellHi.blue(), 102));
      p.fillRect(QRectF(r.left(), r.top(), 3, r.height()), kPhos);
    }
    p.setFont(condensedFont(12, 0.1));
    p.setPen(on ? kPhos : kInkDim);
    p.drawText(r.adjusted(12, 0, 0, 0), Qt::AlignVCenter, label);
  };
  tab(body.top(), QStringLiteral("General"), true);
  tab(body.top() + 42, QStringLiteral("Skins"), false);

  const QRectF pane(body.left() + 108, body.top(), body.width() - 108, body.height() - 40);
  struct Toggle {
    const char* label;
    bool on;
  };
  const Toggle rows[] = {
      {"Resume last session", true},
      {"Confirm before quit", true},
      {"Scroll title", true},
      {"Minimize hides secondaries", false},
  };
  for (int i = 0; i < 4; ++i) {
    const QRectF row(pane.left() + 16, pane.top() + 12 + i * 36, pane.width() - 32, 32);
    p.setFont(condensedFont(12, 0.08));
    p.setPen(kInk);
    p.drawText(row, Qt::AlignVCenter, QString::fromLatin1(rows[i].label));
    const QRectF sw(row.right() - 40, row.center().y() - 10, 36, 20);
    fillRound(p, sw, 10, rows[i].on ? kPhosDeep : QColor(0x2b, 0x31, 0x3e));
    p.setBrush(rows[i].on ? kPhos : kInkDim);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(rows[i].on ? sw.right() - 10 : sw.left() + 10, sw.center().y()), 7,
                  7);
  }
  p.setFont(condensedFont(12, 0.08));
  p.setPen(kInk);
  p.drawText(QRectF(pane.left() + 16, pane.top() + 168, pane.width() - 32, 20),
             Qt::AlignVCenter, QStringLiteral("Dock snap strength"));
  const char* segs[] = {"Off", "Normal", "Strong"};
  qreal sx = pane.left() + 16;
  for (int i = 0; i < 3; ++i) {
    drawBtn(p, QRectF(sx, pane.top() + 194, 88, 28), i == 1, QString::fromLatin1(segs[i]));
    sx += 96;
  }
  p.setFont(condensedFont(12, 0.1));
  p.setPen(kAccent);
  p.drawText(QRectF(body.left() + 12, body.bottom() - 36, 160, 24), Qt::AlignVCenter,
             QStringLiteral("Reset Settings"));
}

void paintAbout(QPainter& p, const QRectF& body, const QImage* logo) {
  const QRectF inner = body.adjusted(16, 14, -16, -14);
  const QRectF badge(inner.left(), inner.top(), 58, 58);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 76));
  p.drawEllipse(badge.adjusted(-6, -6, 6, 6));
  p.setBrush(kLogoDisc);
  p.drawEllipse(badge);
  drawLogoMark(p, badge.adjusted(-4, -4, 4, 4), logo, 1.0);
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 166), 1));
  p.drawEllipse(badge);

  drawGlowText(p, QRectF(badge.right() + 15, inner.top(), 220, 32), QStringLiteral("TRAMP"),
               condensedFont(28, 0.22), kWordmark, QColor(61, 231, 255, 87), 4,
               Qt::AlignLeft | Qt::AlignVCenter);

  const QString words = QStringLiteral("THE RIDICULOUSLY ATTRACTIVE MUSIC PLAYER");
  p.setFont(condensedFont(10, 0.19));
  QFontMetrics fm(p.font());
  qreal tx = badge.right() + 15;
  const qreal ty = inner.top() + 36;
  for (const QString& word : words.split(QChar(' '))) {
    p.setPen(kPhos);
    p.drawText(QPointF(tx, ty + 12), word.left(1));
    tx += fm.horizontalAdvance(word.left(1));
    p.setPen(kInkDim);
    p.drawText(QPointF(tx, ty + 12), word.mid(1) + QStringLiteral(" "));
    tx += fm.horizontalAdvance(word.mid(1) + QStringLiteral(" "));
  }

  const QRectF ver(inner.right() - 58, inner.top() + 8, 58, 22);
  fillRound(p, ver, 2, kWell);
  p.setPen(QPen(QColor(0, 0, 0, 179), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(ver.adjusted(0.5, 0.5, -0.5, -0.5), 2, 2);
  p.setFont(monoFont(10, 0.1));
  p.setPen(kPhos);
  p.drawText(ver, Qt::AlignCenter, QStringLiteral("V 0.1.0"));

  p.setFont(monoFont(10));
  p.setPen(QColor(kInkDim.red(), kInkDim.green(), kInkDim.blue(), 214));
  p.drawText(QRectF(inner.left(), inner.top() + 70, inner.width(), 16), Qt::AlignVCenter,
             QStringLiteral("Local files, honest tags, and chrome you can feel."));

  const QRectF well(inner.left(), inner.top() + 96, inner.width(),
                    inner.height() - 96 - 60);
  drawScreen(p, well);
  p.setFont(condensedFont(9, 0.29));
  p.setPen(QColor(61, 231, 255, 153));
  p.drawText(QRectF(well.left(), well.top() + 8, well.width(), 14), Qt::AlignHCenter,
             QStringLiteral("ON THIS MACHINE"));
  struct Stat {
    const char* label;
    const char* value;
  };
  const Stat stats[] = {
      {"PLAYLISTS", "12"},
      {"TRACKS", "1,284"},
      {"TOTAL TIME", "3 d 22 h"},
      {"SPINS", "4,096"},
  };
  for (int i = 0; i < 4; ++i) {
    const QRectF row(well.left() + 18, well.top() + 28 + i * ((well.height() - 40) / 4.0),
                     well.width() - 36, (well.height() - 40) / 4.0);
    p.setFont(condensedFont(10, 0.18));
    p.setPen(kInkDim);
    p.drawText(row, Qt::AlignVCenter | Qt::AlignLeft, QString::fromLatin1(stats[i].label));
    p.setFont(monoFont(11));
    p.setPen(QColor(61, 231, 255, 230));
    p.drawText(row, Qt::AlignVCenter | Qt::AlignRight, QString::fromLatin1(stats[i].value));
    const QFontMetrics lm(condensedFont(10, 0.18));
    const QFontMetrics vm(monoFont(11));
    const qreal left = row.left() + lm.horizontalAdvance(QString::fromLatin1(stats[i].label)) + 9;
    const qreal right = row.right() - vm.horizontalAdvance(QString::fromLatin1(stats[i].value)) - 9;
    p.setPen(QColor(kInkDim.red(), kInkDim.green(), kInkDim.blue(), 102));
    for (qreal x = right; x >= left; x -= 4) {
      p.fillRect(QRectF(x, row.center().y(), 1, 1), QColor(kInkDim.red(), kInkDim.green(),
                                                          kInkDim.blue(), 102));
    }
  }

  const QRectF plate(inner.left(), inner.bottom() - 48, inner.width(), 48);
  QLinearGradient pg(plate.topLeft(), plate.bottomLeft());
  pg.setColorAt(0, kPlateFace);
  pg.setColorAt(1, kShellLo);
  fillRound(p, plate, 4, pg);
  const QImage proxima = loadProximaMark();
  QRectF mark(plate.left() + 13, plate.center().y() - 13.5, 48, 27);
  if (!proxima.isNull()) {
    const qreal aspect = qreal(proxima.width()) / qMax(1, proxima.height());
    mark.setWidth(27 * aspect);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(mark, proxima);
  }
  const qreal textLeft = mark.right() + 12;
  p.setFont(condensedFont(11, 0.21));
  p.setPen(QColor(232, 234, 240, 235));
  p.drawText(QRectF(textLeft, plate.top() + 6, 200, 16), Qt::AlignLeft | Qt::AlignVCenter,
             QStringLiteral("PROXIMA MAGNIFICA"));
  p.setFont(monoFont(9));
  p.setPen(kInkDim);
  p.drawText(QRectF(textLeft, plate.top() + 24, 200, 14), Qt::AlignLeft | Qt::AlignVCenter,
             QStringLiteral("© 2026 Free Forever"));
  const QRectF web(plate.right() - 118, plate.center().y() - 12, 106, 24);
  fillRound(p, web, 3, QColor(kWell.red(), kWell.green(), kWell.blue(), 217));
  p.setPen(QPen(QColor(61, 231, 255, 71), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(web.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
  p.setFont(monoFont(10));
  p.setPen(kPhos);
  p.drawText(web, Qt::AlignCenter, QStringLiteral("tramp.music"));
}

}  // namespace

void paintWindowBody(QPainter& painter, WindowId id, QSize logical, const QImage* logo) {
  const QRectF body = bodyRect(logical);
  switch (id) {
    case WindowId::main:
      paintMain(painter, body);
      break;
    case WindowId::equalizer:
      paintEq(painter, body, logo);
      break;
    case WindowId::playlist:
      paintPlaylist(painter, body, logo);
      break;
    case WindowId::settings:
      paintSettings(painter, body);
      break;
    case WindowId::about:
      paintAbout(painter, body, logo);
      break;
  }
}

}  // namespace tramp
