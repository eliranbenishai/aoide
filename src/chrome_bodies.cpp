#include "chrome_bodies.h"

#include "chrome_layout.h"
#include "look.h"
#include "mockup_draw.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
#include "tramp_version.h"

#include <QFontMetrics>
#include <QImage>
#include <QPainterPath>
#include <QVector>
#include <array>
#include <cmath>

namespace tramp {
namespace {

const ChromeTokens& T() { return currentLook(); }

/// A control's face. Panels paint from the phase store so a state change
/// cross-fades instead of snapping; a golden dump or a test paints without a
/// live store and takes plain session state.
BtnFace faceOf(const ChromePhases& phases, ChromeHit::Kind kind, bool latched, int index = -1) {
  return phases.live() ? phases.face(kind, index) : BtnFace(latched);
}

QString formatGain(qreal gain) {
  if (qAbs(gain) < 0.05) {
    return QStringLiteral("0.0");
  }
  const QChar sign = gain > 0 ? QChar('+') : QChar(0x2212);
  return sign + QString::number(qAbs(gain), 'f', 1);
}

void paintWellMarquee(QPainter& p, const QRectF& clip, const QString& text, const QFont& font,
                      const QColor& fill, const QVector<TextShadow>& shadows, qreal offset,
                      bool glow) {
  const qreal textW = textWidth(font, text);
  const qreal loop = marqueeLoopWidth(textW, clip.width());
  if (!glow) {
    p.save();
    p.setClipRect(clip);
    p.setFont(font);
    p.setPen(fill);
    const QRectF box(clip.left() - offset, clip.top(), qMax(textW + 16, clip.width()),
                     clip.height());
    p.drawText(box, Qt::AlignLeft | Qt::AlignVCenter, text);
    if (loop > 0) {
      p.drawText(box.translated(loop, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    p.restore();
    return;
  }
  const int bw = qMax(1, int(std::ceil(clip.width())));
  const int bh = qMax(1, int(std::ceil(clip.height())));
  QImage buf(bw, bh, QImage::Format_ARGB32_Premultiplied);
  buf.fill(Qt::transparent);
  QPainter tp(&buf);
  tp.setRenderHint(QPainter::TextAntialiasing);
  auto drawCopy = [&](qreal x) {
    drawStyledText(tp, QRectF(x, 0, qMax(textW + 16, clip.width()), clip.height()), text, font,
                   fill, Qt::AlignLeft | Qt::AlignVCenter, shadows);
  };
  drawCopy(-offset);
  if (loop > 0) drawCopy(-offset + loop);
  QLinearGradient fade(QPointF(clip.width() * 0.84, 0), QPointF(clip.width(), 0));
  fade.setColorAt(0, QColor(255, 255, 255, 255));
  fade.setColorAt(1, QColor(255, 255, 255, 0));
  tp.setCompositionMode(QPainter::CompositionMode_DestinationIn);
  tp.fillRect(QRectF(0, 0, clip.width(), clip.height()), fade);
  tp.end();
  p.drawImage(clip.topLeft(), buf);
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

void paintMain(QPainter& p, const QRectF& body, const SessionView& view, BodyPaint pass,
               const ChromePhases& phases) {
  using K = ChromeHit::Kind;
  QString time = formatClock(view.showElapsed
                                 ? view.positionMs
                                 : qMax<qint64>(0, view.durationMs - view.positionMs));
  QString timeLabel = view.showElapsed ? QStringLiteral("ELAPSED") : QStringLiteral("REMAIN");
  QString title = view.title;
  QString subtitle = view.subtitle.toUpper();
  QString bitrate = view.bitrate;
  QString rate = view.sampleRate;
  QString channels = view.channels;
  QString fmtLabel = view.formatChip;
  qreal volume = view.muted ? 0 : view.volume;
  qreal seek = view.durationMs > 0 ? qreal(view.positionMs) / qreal(view.durationMs) : 0;
  QString pos = formatClock(view.positionMs);
  QString dur = formatClock(view.durationMs);
  bool playOn = view.playing;
  bool pauseOn = view.paused;
  bool shuffleOn = view.shuffle;
  bool repeatOn = view.repeat != RepeatMode::off;
  std::array<qreal, 20> bars = view.spectrum;
  std::array<qreal, 20> peaks = view.spectrumPeaks;
  if (view.goldenDemo) {
    time = QStringLiteral("2:41");
    timeLabel = QStringLiteral("ELAPSED");
    title = QStringLiteral("3. Velvet Static — Neon Boulevard (Extended Mix)");
    subtitle = QStringLiteral("COPPER RAIN EP · TRACK 3 OF 12");
    bitrate = QStringLiteral("192 kbps");
    rate = QStringLiteral("44.1 kHz");
    channels = QStringLiteral("STEREO");
    fmtLabel = QStringLiteral("MP3");
    volume = 0.66;
    seek = 161.0 / 347.0;
    pos = QStringLiteral("2:41");
    dur = QStringLiteral("5:47");
    playOn = true;
    pauseOn = false;
    shuffleOn = true;
    repeatOn = true;
    bars = {0.26, 0.52, 0.71, 0.88, 0.64, 0.47, 0.58, 0.39, 0.31, 0.44,
            0.35, 0.24, 0.29, 0.19, 0.22, 0.14, 0.17, 0.10, 0.12, 0.07};
    peaks = {0.44, 0.70, 0.88, 0.96, 0.80, 0.66, 0.74, 0.57, 0.52, 0.61,
             0.55, 0.42, 0.47, 0.36, 0.40, 0.30, 0.33, 0.24, 0.27, 0.19};
  }

  const bool chassis = pass != BodyPaint::live;
  const bool live = pass != BodyPaint::chassis;
  const bool glow = pass == BodyPaint::full;

  const QRectF well(body.left() + 96, body.top() + 14, 705, 132);
  const QRectF inner = well.adjusted(16, 12, -16, -12);

  if (chassis) {
    drawScreenWell(p, well);
    drawGlyphBtn(p, QRectF(body.left() + 22, body.top() + 18, 26, 26), MockupIcon::options,
                 faceOf(phases, K::options, false), 16);
  }

  if (live) {
    const int timePx =
        pixelSizeFittingLineHeight(monoFont(kElapsedTimePx), kElapsedTimePx, kElapsedTimeBoxH);
    const QFont timeFont = monoFont(timePx, 0.02);
    const QFontMetricsF tm(timeFont);
    const qreal timeW = tm.horizontalAdvance(time);
    const QRectF timeBox(inner.left(), inner.top() - 9, timeW + 8, kElapsedTimeBoxH);
    if (glow) {
      drawStyledText(p, timeBox, time, timeFont, T().phos, Qt::AlignLeft | Qt::AlignTop,
                     {
                         {withAlpha(T().phos, 0xd9), QPointF(), 1},
                         {withAlpha(T().phos, 0x73), QPointF(), 8},
                     });
    } else {
      p.setFont(timeFont);
      p.setPen(T().phos);
      p.drawText(timeBox, Qt::AlignLeft | Qt::AlignTop, time);
    }
    const QFont elFont = condensedFont(12, 0.22);
    const QFontMetricsF em(elFont);
    const qreal baseline = inner.top() - 9 + tm.ascent();
    const QRectF elBox(inner.left() + timeW + 10, baseline - em.ascent(), 90, em.height());
    if (glow) {
      drawStyledText(p, elBox, timeLabel, elFont, withAlpha(T().phos, 128),
                     Qt::AlignLeft | Qt::AlignTop,
                     {{withAlpha(T().phos, 0x40), QPointF(), 8}});
    } else {
      p.setFont(elFont);
      p.setPen(withAlpha(T().phos, 128));
      p.drawText(elBox, Qt::AlignLeft | Qt::AlignTop, timeLabel);
    }

    const QRectF viz(inner.left(), inner.bottom() - 42, 248, 42);
    for (int i = 0; i < 20; ++i) {
      const qreal x = viz.left() + i * 12;
      const qreal h = viz.height() * bars[size_t(i)];
      QRectF bar(x, viz.bottom() - h, 9, h);
      p.fillRect(bar, T().spectrumGradient(bar.topLeft(), bar.bottomLeft()));
      if (glow) {
        paintBlurred(p, bar.adjusted(-4, -4, 4, 4), 2.5, [&](QPainter& bp) {
          bp.setPen(Qt::NoPen);
          bp.setBrush(withAlpha(T().phos, 77));
          bp.fillRect(bar, withAlpha(T().phos, 77));
        });
      }
      const qreal py = viz.bottom() - viz.height() * peaks[size_t(i)];
      p.fillRect(QRectF(x, py, 9, 2), T().phosHot);
    }
  }

  const QRectF meta(inner.left() + 268 + 20, inner.top(), inner.width() - 288, inner.height());
  const QFont titleFont = condensedFont(24, 0.03);
  const QFont subFont = condensedFont(14, 0.14);
  const bool scroll = view.scrollTitle && !view.goldenDemo;
  const QRectF titleClip(meta.left(), meta.top(), meta.width(), 32);
  const QRectF subClip(meta.left(), meta.top() + 32, meta.width(), 18);
  const qreal titleOff =
      marqueeOffset(textWidth(titleFont, title), titleClip.width(), view.titleScrollMs, scroll);
  const qreal subOff =
      marqueeOffset(textWidth(subFont, subtitle), subClip.width(), view.titleScrollMs, scroll);
  const bool titleLive = displayTitleOnLivePass(titleOff);
  const bool subLive = displayTitleOnLivePass(subOff);
  const QVector<TextShadow> titleGlow = {
      {withAlpha(T().phos, 0xe6), QPointF(), 1.5},
      {withAlpha(T().phos, 0x80), QPointF(), 10},
  };
  const QVector<TextShadow> subGlow = {{withAlpha(T().phos, 0x40), QPointF(), 8}};

  if ((chassis && !titleLive) || (live && titleLive)) {
    paintWellMarquee(p, titleClip, title, titleFont, T().phosHot, titleGlow, titleOff,
                     chassis && !titleLive);
  }
  if (!subtitle.isEmpty() && ((chassis && !subLive) || (live && subLive))) {
    paintWellMarquee(p, subClip, subtitle, subFont, withAlpha(T().phos, 128), subGlow, subOff,
                     chassis && !subLive);
  }

  if (chassis) {
    const qreal metaY = inner.bottom() - 18;
    const QFont metaFont = monoFont(13, 0.04);
    const QFont channelsFont = condensedFont(12, 0.2);
    const QFont fmtFont = condensedFont(12, 0.18);
    const QFont chipFont = condensedFont(12, 0.14);
    const QString chipLabel = QStringLiteral("PLAYLIST");
    const qreal reload = 11;
    const qreal fmtW = 9 + textWidth(fmtFont, fmtLabel) + 9;
    const qreal chipW = reload + 4 + textWidth(chipFont, chipLabel);
    const auto metaRow = layoutDisplayMetaRow(
        QRectF(meta.left(), metaY, meta.width(), kDisplayMetaH), metaY,
        textWidth(metaFont, bitrate), textWidth(metaFont, rate),
        textWidth(channelsFont, channels), chipW, fmtW);
    drawStyledText(p, metaRow.bitrate, bitrate, metaFont, withAlpha(T().phos, 128),
                   Qt::AlignVCenter | Qt::AlignLeft, {{withAlpha(T().phos, 0x40), QPointF(), 8}});
    drawStyledText(p, metaRow.rate, rate, metaFont, withAlpha(T().phos, 128),
                   Qt::AlignVCenter | Qt::AlignLeft, {{withAlpha(T().phos, 0x40), QPointF(), 8}});
    drawStyledText(p, metaRow.channels, channels, channelsFont, T().phos,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   {
                       {withAlpha(T().phos, 0xd9), QPointF(), 1},
                       {withAlpha(T().phos, 0x73), QPointF(), 12},
                   });

    const QRectF fmt = metaRow.format;
    paintBlurred(p, fmt.adjusted(-18, -18, 18, 18), 12 * 0.57735, [&](QPainter& bp) {
      bp.setPen(Qt::NoPen);
      bp.setBrush(withAlpha(T().accent, 102));
      bp.drawRoundedRect(fmt, 2, 2);
    });
    QLinearGradient badge(fmt.topLeft(), fmt.bottomLeft());
    badge.setColorAt(0, T().accentHot);
    badge.setColorAt(0.55, T().accent);
    badge.setColorAt(1, T().accentDim);
    fillRound(p, fmt, 2, badge);
    p.setPen(T().litLedRim);
    p.setFont(fmtFont);
    p.drawText(fmt, Qt::AlignCenter, fmtLabel);

    const QRectF chip = metaRow.playlist;
    drawReload(p, QRectF(chip.left(), chip.center().y() - reload / 2, reload, reload), T().phos);
    drawStyledText(p, QRectF(chip.left() + reload + 4, chip.top(), chip.width() - reload - 4, 16),
                   chipLabel, chipFont, T().phos, Qt::AlignVCenter | Qt::AlignLeft,
                   {{withAlpha(T().phos, 217), QPointF(), 8}});
  }

  if (live) {
    drawScreenOverlay(p, well);
  }

  if (chassis) {
    const MainVolumeRow vol = layoutMainVolumeRow(body);
    drawGlyphBtn(p, vol.mute, MockupIcon::mute, faceOf(phases, K::mute, view.muted), 21);
    p.setFont(condensedFont(11, 0.2));
    p.setPen(T().inkFaint);
    p.drawText(vol.label, Qt::AlignVCenter, QStringLiteral("VOL"));
    drawSlider(p, vol.track, volume);
    drawBtn(p, vol.mono, faceOf(phases, K::mono, view.forceMono), QStringLiteral("MONO"));
    drawBtn(p, vol.eq, faceOf(phases, K::eqToggle, view.eqOn), QStringLiteral("EQ"));
    drawBtn(p, vol.pl, faceOf(phases, K::plToggle, view.plOn), QStringLiteral("PL"));
  }

  if (live) {
    const QFont stamp = monoFont(14);
    const QFontMetricsF sm(stamp);
    const MainSeekRow row =
        layoutMainSeekRow(body, sm.horizontalAdvance(pos), sm.horizontalAdvance(dur));
    p.setFont(stamp);
    p.setPen(T().inkDim);
    p.drawText(row.elapsed, Qt::AlignVCenter, pos);
    p.drawText(row.duration, Qt::AlignVCenter, dur);
    drawSlider(p, row.track, seek, true, glow);
  }

  if (chassis) {
    const MainTransportRow row =
        layoutMainTransportRow(body, toggleBtnWidth(QStringLiteral("SHUFFLE")),
                               toggleBtnWidth(QStringLiteral("REPEAT")));
    auto place = [&](const QRectF& r, MockupIcon icon, ChromeHit::Kind kind, bool on) {
      drawGlyphBtn(p, r, icon, faceOf(phases, kind, on), 22);
    };
    place(row.prev, MockupIcon::previous, K::prev, false);
    place(row.play, MockupIcon::play, K::play, playOn);
    place(row.pause, MockupIcon::pause, K::pause, pauseOn);
    place(row.stop, MockupIcon::stop, K::stop, false);
    place(row.next, MockupIcon::next, K::next, false);
    place(row.eject, MockupIcon::eject, K::eject, false);
    drawToggleBtn(p, row.shuffle, QStringLiteral("SHUFFLE"), faceOf(phases, K::shuffle, shuffleOn));
    drawToggleBtn(p, row.repeat, QStringLiteral("REPEAT"), faceOf(phases, K::repeat, repeatOn));
  }
}

void paintEq(QPainter& p, const QRectF& body, const QImage* logo, const SessionView& view,
            BodyPaint pass, const ChromePhases& phases) {
  using K = ChromeHit::Kind;
  bool on = view.eq.enabled;
  bool autoOn = view.eq.auto_;
  QString curveName = view.eq.presetName.isEmpty() ? QStringLiteral("CUSTOM") : view.eq.presetName.toUpper();
  qreal preamp = view.eq.preamp;
  qreal gains[10];
  for (int i = 0; i < 10; ++i) gains[i] = view.eq.gains[size_t(i)];
  if (view.goldenDemo) {
    on = true;
    autoOn = false;
    curveName = QStringLiteral("LATE NIGHT");
    preamp = 3.8;
    const qreal demo[] = {6.2, 4.6, 1.0, -1.9, -0.5, 2.2, 3.4, 1.4, 0.0, 5.0};
    for (int i = 0; i < 10; ++i) gains[i] = demo[i];
  }

  const bool chassis = pass != BodyPaint::live;
  const bool live = pass != BodyPaint::chassis;
  const bool glow = pass == BodyPaint::full;

  const EqHeaderRow header = layoutEqHeader(body, labelBtnWidth(QStringLiteral("ON")),
                                            labelBtnWidth(QStringLiteral("AUTO")),
                                            labelBtnWidth(QStringLiteral("PRESETS"), 16, 22));
  const QRectF& curveWell = header.curveWell;
  const QRectF bandRow = eqBandRow(body);
  const char* labels[] = {"PREAMP", "60", "170", "310", "600", "1k",
                          "3k",     "6k", "12k", "14k", "16k"};
  qreal allGains[11] = {preamp};
  for (int i = 0; i < 10; ++i) allGains[i + 1] = gains[i];

  if (chassis) {
    drawBtn(p, header.on, faceOf(phases, K::eqOn, on), QStringLiteral("ON"));
    drawBtn(p, header.autoBtn, faceOf(phases, K::eqAuto, autoOn), QStringLiteral("AUTO"));
    drawBtn(p, header.presets, faceOf(phases, K::eqPresets, false), QStringLiteral("PRESETS"));
    drawMenuCaret(p, header.presets);
    p.setFont(condensedFont(11, 0.2));
    p.setPen(T().inkFaint);
    p.drawText(header.curveLabel, Qt::AlignVCenter,
               QStringLiteral("CURVE · %1").arg(curveName));
    drawScreenWell(p, curveWell);
    p.setFont(monoFont(11));
    p.setPen(T().inkFaint);
    const QString scale[] = {QStringLiteral("+12"), QStringLiteral("0"), QStringLiteral("−12")};
    for (int i = 0; i < 3; ++i) {
      p.drawText(eqScaleMark(bandRow, i), Qt::AlignRight | Qt::AlignVCenter, scale[i]);
    }
    for (int i = 0; i < kEqBandCount; ++i) {
      p.setFont(condensedFont(11, i == 0 ? 0.18 : 0.1));
      p.setPen(i == 0 ? withAlpha(T().phos, 140) : T().inkFaint);
      p.drawText(eqBandColumn(bandRow, i).label, Qt::AlignHCenter | Qt::AlignVCenter,
                 QString::fromLatin1(labels[i]));
    }
  }

  if (live) {
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
    p.setPen(QPen(withAlpha(T().coolSheen, 36), 1));
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
    wash.setColorAt(0, withAlpha(T().phos, 102));
    wash.setColorAt(1, withAlpha(T().phos, 0));
    p.save();
    QPainterPath wellClip;
    wellClip.addRoundedRect(curveWell, kWellRadius, kWellRadius);
    p.setClipPath(wellClip);
    p.fillPath(fill, wash);
    if (glow) {
      paintBlurred(p, curveWell.adjusted(-8, -8, 8, 8), 2.5, [&](QPainter& bp) {
        bp.setBrush(Qt::NoBrush);
        bp.setPen(QPen(withAlpha(T().phos, 153), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        bp.drawPath(curve);
      });
    }
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(T().curveStroke, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(curve);
    p.restore();
    drawScreenOverlay(p, curveWell);

    for (int i = 0; i < kEqBandCount; ++i) {
      const EqBandColumn column = eqBandColumn(bandRow, i);
      p.setFont(monoFont(11));
      p.setPen(T().ink);
      p.drawText(column.gain, Qt::AlignHCenter | Qt::AlignVCenter, formatGain(allGains[i]));
      drawVBand(p, column.well, allGains[i]);
    }
    drawLogoMark(p, QRectF(body.right() - 36 - 120, body.top() + 120, 120, 120), logo, 0.14);
  }
}

void paintPlaylist(QPainter& p, const QRectF& body, const QImage* logo, const SessionView& view,
                   const ChromePhases& phases) {
  using K = ChromeHit::Kind;
  QVector<CollectionRowView> lists = view.collection;
  QVector<TrackRowView> rows = view.tracks;
  QString totalText = formatClock(view.playlistTotalMs);
  QString statusName = view.playlistName.toUpper();
  if (statusName.isEmpty()) statusName = QStringLiteral("UNTITLED");
  if (view.playlistAltered) statusName += QStringLiteral(" *");
  int playingN = view.playingIndex ? *view.playingIndex + 1 : 0;
  qreal collectionW = view.collectionCollapsed ? 0 : view.collectionWidth;
  if (view.goldenDemo) {
    collectionW = 240;
    lists = {
        {QStringLiteral("ANALOGUE GHOSTS"), 24, false, false},
        {QStringLiteral("COPPER RAIN EP"), 13, true, false},
        {QStringLiteral("NIGHTBUS CHOIR — LIVE"), 8, false, false},
    };
    rows = {
        {QStringLiteral("Cassette Mirage"), QStringLiteral("Low Orbit Lullaby"), QStringLiteral("4:12"), false, false},
        {QStringLiteral("The Brass Cassini"), QStringLiteral("Slow Dial"), QStringLiteral("3:38"), false, false},
        {QStringLiteral("Velvet Static"), QStringLiteral("Neon Boulevard (Extended Mix)"), QStringLiteral("5:47"), true, true},
        {QStringLiteral("Halogen Youth"), QStringLiteral("Parking Garage Sunset"), QStringLiteral("4:03"), false, false},
        {QStringLiteral("Moth & Marrow"), QStringLiteral("Analogue Ghosts"), QStringLiteral("6:21"), false, false},
        {QStringLiteral("Ruby Transit"), QStringLiteral("Bakelite Heart"), QStringLiteral("3:55"), false, false},
        {QStringLiteral("Slow Signal"), QStringLiteral("Copper Rain"), QStringLiteral("4:44"), false, false},
        {QStringLiteral("Aurora Kiosk"), QStringLiteral("Departure Lounge B"), QStringLiteral("5:09"), false, false},
        {QStringLiteral("Pale Antenna"), QStringLiteral("Tramp Theme (Demo)"), QStringLiteral("2:58"), false, false},
        {QStringLiteral("Nightbus Choir"), QStringLiteral("Fluorescent Hymn"), QStringLiteral("6:02"), false, false},
        {QStringLiteral("Second Cassette"), QStringLiteral("Static Blonde"), QStringLiteral("3:27"), false, false},
        {QStringLiteral("Velvet Static"), QStringLiteral("Neon Boulevard (Reprise)"), QStringLiteral("2:02"), false, false},
        {QStringLiteral("Long Wave Motel"), QStringLiteral("Untitled Sketch"), QStringLiteral("13:16"), false, false},
    };
    totalText = QStringLiteral("65:34");
    statusName = QStringLiteral("COPPER RAIN — NIGHT SET.M3U8");
    playingN = 3;
  }

  const QRectF collection(body.left(), body.top(), collectionW, body.height());
  const QRectF divider(collection.right(), collection.top(), collectionW > 0 ? 8 : 0,
                       collection.height());
  const QRectF tracksPane = playlistTracksPane(body, collectionW);
  if (collectionW > 0) {
    p.fillRect(divider, T().shellDeep);
    drawFooterSep(p, QRectF(divider.center().x() - 1, divider.top() + 24, 1,
                            divider.height() - 48));

  const QRectF colInner = collection.adjusted(12, 12, -6, -12);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(T().inkFaint);
  p.drawText(QRectF(colInner.left(), colInner.top(), colInner.width() - 30, 20),
             Qt::AlignVCenter, QStringLiteral("PLAYLISTS"));
  const QRectF collapse(colInner.right() - 24, colInner.top(), 24, 20);
  drawBtn(p, collapse, faceOf(phases, K::plCollapse, false), {});
  drawChevron(p, QRectF(collapse.center().x() - 2.8, collapse.center().y() - 4, 5.6, 8), true,
              T().glyphInk);

  const QRectF colWell(colInner.left(), colInner.top() + 30, colInner.width(),
                       colInner.height() - 30 - 8 - 24);
  drawScreenWell(p, colWell);
  p.save();
  QPainterPath colClip;
  colClip.addRoundedRect(colWell, kWellRadius, kWellRadius);
  p.setClipPath(colClip);
  for (int i = 0; i < lists.size(); ++i) {
    QRectF row(colWell.left(), colWell.top() + 4 + i * 26, colWell.width(), 26);
    if (lists[i].selected) {
      QLinearGradient g(row.topLeft(), row.bottomLeft());
      g.setColorAt(0, withAlpha(T().phos, 33));
      g.setColorAt(1, withAlpha(T().phos, 10));
      p.fillRect(row, g);
    }
    p.setFont(condensedFont(11, 0.1));
    p.setPen(lists[i].disabled ? T().inkFaint : (lists[i].selected ? T().phosHot : T().inkDim));
    p.drawText(row.adjusted(10, 0, -36, 0), Qt::AlignVCenter, lists[i].name.toUpper());
    p.setFont(monoFont(12));
    p.setPen(lists[i].selected ? T().phos : T().phosDim);
    p.drawText(row.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignRight,
               QString::number(lists[i].count));
  }
  p.restore();
  drawScreenOverlay(p, colWell);

  qreal cx = colInner.left();
  const qreal cy = colInner.bottom() - 24;
  auto cbtn = [&](ChromeHit::Kind kind, auto paintFace, bool menu) {
    const QRectF r(cx, cy, 30, 24);
    drawBtn(p, r, faceOf(phases, kind, false), {});
    paintFace(r);
    if (menu) {
      drawMenuCaret(p, r);
    }
    cx += 36;
  };
  cbtn(K::plAddCollection, [&](const QRectF& r) {
    drawIcon(p, QRectF(r.center().x() - 6.5, r.center().y() - 6.5, 13, 13), MockupIcon::add,
             T().glyphInk);
  }, false);
  cbtn(K::plCreate, [&](const QRectF& r) {
    drawCreateMark(p, QRectF(r.center().x() - 6, r.center().y() - 6, 12, 12),
                   T().glyphInk);
  }, true);
  cbtn(K::plRename, [&](const QRectF& r) {
    drawRenameMark(p, QRectF(r.center().x() - 6, r.center().y() - 6, 12, 12),
                   T().glyphInk);
  }, false);
  cbtn(K::plRemoveCollection, [&](const QRectF& r) {
    drawIcon(p, QRectF(r.center().x() - 6.5, r.center().y() - 6.5, 13, 13), MockupIcon::remove,
             T().glyphInk);
  }, false);
  } else if (!view.goldenDemo) {
    const QRectF tab(tracksPane.left() + 4, tracksPane.top() + 12, 14, 56);
    drawBtn(p, tab, faceOf(phases, K::plCollapse, false), {});
    drawChevron(p, QRectF(tab.center().x() - 2.8, tab.center().y() - 4, 5.6, 8), false,
                T().glyphInk);
  }

  const QRectF trackInner = playlistTrackInner(tracksPane);
  const QRectF listRow = playlistListRowRect(trackInner);
  const int rowCount = rows.size();
  const QRectF listWell = playlistListWell(listRow, rowCount);
  drawListWell(p, listWell);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(listWell, kWellRadius, kWellRadius);
  p.setClipPath(clip);
  const int scrollRows = view.goldenDemo ? 0 : view.trackScroll;
  const int visible = playlistVisibleRows(listWell.height()) + 1;
  const QFont lcd = monoFont(15);
  const QFont lcdTrack = monoFont(15, 0.15 / 15.0);
  // Measure against the whole list, not the visible page, so the column keeps
  // its width as the list scrolls. The longest string is the widest one, so this
  // costs one text measurement rather than one per track.
  const QString* widestTime = nullptr;
  for (const TrackRowView& r : rows) {
    if (!widestTime || r.time.size() > widestTime->size()) widestTime = &r.time;
  }
  const qreal timeTextW = widestTime ? textWidth(lcd, *widestTime) : 0;
  for (int vis = 0; vis < visible && scrollRows + vis < rows.size(); ++vis) {
    const int i = scrollRows + vis;
    const QRectF row(listWell.left(), listWell.top() + kPlaylistRowPadTop + vis * kPlaylistRowStride,
                     listWell.width(), kPlaylistRowStride);
    const bool playing = rows[i].playing;
    const bool selected = rows[i].selected;
    const bool disabled = rows[i].disabled;
    const QColor color = disabled ? T().inkFaint : (playing ? T().phosHot : withAlpha(T().phos, 115));
    if (selected) {
      QLinearGradient g(row.topLeft(), row.bottomLeft());
      g.setColorAt(0, withAlpha(T().phos, 33));
      g.setColorAt(1, withAlpha(T().phos, 10));
      p.fillRect(row, g);
    }
    if (playing) {
      const QRectF bar(row.left(), row.top() + 6, 3, row.height() - 12);
      paintBlurred(p, bar.adjusted(-18, -18, 18, 18), 12 * 0.57735, [&](QPainter& bp) {
        bp.setPen(Qt::NoPen);
        bp.setBrush(withAlpha(T().accent, 230));
        bp.drawRoundedRect(bar, 0, 2);
      });
      QPainterPath barPath;
      barPath.addRoundedRect(bar, 0, 2);
      p.fillPath(barPath, T().accent);
    }
    const PlaylistRowColumns col = playlistRowColumns(row, timeTextW);
    const QString label = rows[i].artist.isEmpty()
                              ? rows[i].title
                              : QStringLiteral("%1 — %2").arg(rows[i].artist, rows[i].title);
    if (playing) {
      drawStyledText(p, col.index, QStringLiteral("%1.").arg(i + 1), lcd, T().phos,
                     Qt::AlignVCenter | Qt::AlignRight,
                     {{withAlpha(T().phos, 0x80), QPointF(), 10}});
      drawStyledText(p, col.title, label, lcdTrack, color, Qt::AlignVCenter | Qt::AlignLeft,
                     {{withAlpha(T().phos, 0x80), QPointF(), 10}});
    } else {
      p.setFont(lcd);
      p.setPen(QColor(color.red(), color.green(), color.blue(), 179));
      p.drawText(col.index, Qt::AlignVCenter | Qt::AlignRight,
                 QStringLiteral("%1.").arg(i + 1));
      p.setFont(lcdTrack);
      p.setPen(color);
      p.drawText(col.title, Qt::AlignVCenter, label);
    }
    p.setFont(lcd);
    p.setPen(QColor(color.red(), color.green(), color.blue(), 204));
    p.drawText(col.time, Qt::AlignVCenter | Qt::AlignRight, rows[i].time);
  }
  drawLogoMark(p, QRectF(listWell.right() - 26 - 178, listWell.bottom() - 8 - 178, 178, 178),
               logo, 0.05);
  p.restore();
  drawScreenOverlay(p, listWell, QColor(0, 0, 0, 56), false);
  const int maxScroll = playlistListMaxScroll(rowCount, listWell.height());
  if (maxScroll > 0) {
    const QRectF scroll = playlistListScrollTrack(listWell);
    const QRectF thumb = playlistListThumb(scroll, rowCount, scrollRows, listWell.height());
    drawScrollbar(p, scroll, thumb.top() - scroll.top(), thumb.height());
  }

  const QRectF footer(trackInner.left(), trackInner.bottom() - kPlaylistFooterH, trackInner.width(),
                      kPlaylistFooterH);
  const QRectF deck(footer.left(), footer.top(), footer.width(), 74);
  const QRectF deckInner = deck.adjusted(12, 10, -12, -10);
  const QFont totalLabel = condensedFont(11, 0.2);
  const QFont totalValue = monoFont(18);
  const qreal totalW = playlistStripTotalWidth(textWidth(totalLabel, QStringLiteral("TOTAL")),
                                              textWidth(totalValue, totalText));
  const auto strip = layoutPlaylistStrip(deckInner, totalW);
  auto paintBtn = [&](const QRectF& r, MockupIcon icon, ChromeHit::Kind kind, bool menu,
                      qreal iconSize, bool on = false) {
    drawGlyphBtn(p, r, icon, faceOf(phases, kind, on), iconSize);
    if (menu) {
      drawMenuCaret(p, r);
    }
  };
  paintBtn(strip.add, MockupIcon::add, K::plAdd, false, 21);
  paintBtn(strip.remove, MockupIcon::remove, K::plRemove, false, 21);
  drawFooterSep(p, strip.sep);
  paintBtn(strip.sort, MockupIcon::sort, K::plSort, true, 21);
  paintBtn(strip.options, MockupIcon::options, K::plOptions, true, 21);
  paintBtn(strip.prev, MockupIcon::previous, K::plPrev, false, 18);
  paintBtn(strip.play, view.playing ? MockupIcon::pause : MockupIcon::play, K::plPlay, false, 18,
           view.playing);
  paintBtn(strip.next, MockupIcon::next, K::plNext, false, 18);
  drawScreenWell(p, strip.total);
  p.setFont(totalLabel);
  p.setPen(T().phosDim);
  p.drawText(strip.total.adjusted(18, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
             QStringLiteral("TOTAL"));
  drawGlowText(p, strip.total.adjusted(0, 0, -18, 0), totalText, totalValue, T().phos,
               withAlpha(T().phos, 115), 4, Qt::AlignVCenter | Qt::AlignRight);
  drawScreenOverlay(p, strip.total);
  const bool refreshEnabled = view.goldenDemo ? true : view.playlistRefreshEnabled;
  const bool refreshLit = view.playlistRefreshing;
  drawBtn(p, strip.refresh, faceOf(phases, K::plRefresh, refreshLit));
  {
    const qreal icon = 16;
    const QRectF box(strip.refresh.center().x() - icon / 2, strip.refresh.center().y() - icon / 2,
                     icon, icon);
    const QColor ink =
        refreshLit ? T().btnOnInk : refreshEnabled ? T().glyphInk : T().inkFaint;
    drawReload(p, box, ink);
  }

  const QFont statusFont = condensedFont(12, 0.18);
  p.setFont(statusFont);
  p.setPen(T().inkFaint);
  const QRectF status(footer.left() + 6, footer.bottom() - 26, footer.width() - 28, 26);
  qreal sx = status.left();
  auto statusBit = [&](const QString& text) {
    const qreal w = textWidth(statusFont, text);
    p.drawText(QRectF(sx, status.top(), w, 26), Qt::AlignVCenter, text);
    sx += w;
  };
  statusBit(statusName);
  drawStatusDot(p, QPointF(sx + 18 + 2.5, status.center().y()));
  sx += 18 + 5 + 18;
  statusBit(QStringLiteral("%1 TRACKS").arg(view.goldenDemo ? rows.size() : view.playlistTrackCount));
  drawStatusDot(p, QPointF(sx + 18 + 2.5, status.center().y()));
  sx += 18 + 5 + 18;
  statusBit(playingN > 0 ? QStringLiteral("PLAYING %1").arg(playingN) : QStringLiteral("STOPPED"));
  drawStatusDot(p, QPointF(sx + 18 + 2.5, status.center().y()));
  const QString drop = QStringLiteral("DROP FILES HERE TO ENQUEUE");
  const qreal dropW = textWidth(statusFont, drop);
  p.drawText(QRectF(status.right() - dropW, status.top(), dropW, 26), Qt::AlignVCenter, drop);
}

void paintSettings(QPainter& p, const QRectF& body, const SessionView& view,
                   const ChromePhases& phases) {
  using K = ChromeHit::Kind;
  const int tabIndex = view.goldenDemo ? 0 : view.settingsTab;
  const bool resume = view.goldenDemo ? true : view.resumeLastSession;
  const bool confirm = view.goldenDemo ? true : view.confirmBeforeQuit;
  const bool scroll = view.goldenDemo ? true : view.scrollTitle;
  const bool minimize = view.goldenDemo ? false : view.minimizeHidesSecondaries;
  const int snap = view.goldenDemo ? 1 : view.dockSnap;

  p.fillRect(QRectF(body.left(), body.top(), 108, body.height()), T().shellDeep);
  auto tab = [&](qreal y, const QString& label, ChromeHit::Kind kind, bool on) {
    const QRectF r(body.left(), y, 108, 42);
    const BtnFace face = faceOf(phases, kind, on);
    const int wash = int(std::lround(102 * face.on + 44 * face.hover));
    if (wash > 0) p.fillRect(r, withAlpha(T().shellHi, wash));
    if (face.on > 0.004) {
      p.fillRect(QRectF(r.left(), r.top(), 3, r.height()),
                 withAlpha(T().phos, int(std::lround(255 * face.on))));
    }
    p.setFont(condensedFont(12, 0.1));
    p.setPen(mix(T().inkDim, T().phos, face.on));
    p.drawText(r.adjusted(12, 0, 0, 0), Qt::AlignVCenter, label);
  };
  tab(body.top(), QStringLiteral("General"), K::settingsGeneral, tabIndex == 0);
  tab(body.top() + 42, QStringLiteral("Skins"), K::settingsSkins, tabIndex == 1);

  const QRectF pane = settingsPane(body);
  if (tabIndex == 1) {
    const QVector<SkinCatalogEntry> skins = view.goldenDemo ? QVector<SkinCatalogEntry>{} : view.skins;
    const QString active = view.goldenDemo ? QStringLiteral("builtin") : view.activeSkinId;
    const QRectF viewport = skinsListViewport(pane);
    const int scroll = view.goldenDemo ? 0 : view.skinsScroll;
    p.save();
    p.setClipRect(viewport);
    for (int i = 0; i < skins.size(); ++i) {
      const QRectF row = skinsListRow(viewport, i, scroll);
      if (row.bottom() < viewport.top() || row.top() > viewport.bottom()) continue;
      const bool on = skins[i].id == active;
      if (on) {
        p.fillRect(row, QColor(T().shellHi.red(), T().shellHi.green(), T().shellHi.blue(), 115));
      }
      p.setFont(condensedFont(12, 0.08));
      p.setPen(on ? T().phos : T().ink);
      p.drawText(row.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, skins[i].name);
      if (!skins[i].author.isEmpty()) {
        p.setFont(monoFont(10));
        p.setPen(T().inkDim);
        const qreal nameW = textWidth(condensedFont(12, 0.08), skins[i].name);
        p.drawText(row.adjusted(14 + nameW, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   skins[i].author);
      }
    }
    p.restore();
    const int maxScroll = skinsListMaxScroll(skins.size(), viewport.height());
    if (maxScroll > 0) {
      const QRectF track = skinsListScrollTrack(viewport);
      const QRectF thumb = skinsListThumb(track, skins.size(), scroll);
      drawScrollbar(p, track, thumb.top() - track.top(), thumb.height());
    }
    if (!view.skinsError.isEmpty() && !view.goldenDemo) {
      p.setFont(monoFont(10));
      p.setPen(T().accent);
      p.drawText(QRectF(pane.left() + 12, pane.bottom() - 92, pane.width() - 24, 28),
                 Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, view.skinsError);
    }
    const qreal btnY = pane.bottom() - kSkinsBtnStackH;
    drawBtn(p, QRectF(pane.left() + 12, btnY, 148, 26), faceOf(phases, K::settingsInstallZip, false),
            QStringLiteral("Install zip"));
    drawBtn(p, QRectF(pane.left() + 168, btnY, 160, 26),
            faceOf(phases, K::settingsInstallFolder, false), QStringLiteral("Install folder"));
    drawBtn(p, QRectF(pane.left() + 12, btnY + 30, 148, 26),
            faceOf(phases, K::settingsSkinsFolder, false), QStringLiteral("Skins folder"));
    drawBtn(p, QRectF(pane.left() + 168, btnY + 30, 160, 26),
            faceOf(phases, K::settingsResetSkinsFolder, false), QStringLiteral("Reset folder"));
    p.setFont(condensedFont(12, 0.1));
    p.setPen(T().accent);
    p.drawText(QRectF(body.left() + 12, body.bottom() - 36, 160, 24), Qt::AlignVCenter,
               QStringLiteral("Reset Settings"));
    return;
  }
  struct Toggle {
    const char* label;
    ChromeHit::Kind kind;
    bool on;
  };
  const Toggle rows[] = {
      {"Resume last session", K::settingsResume, resume},
      {"Confirm before quit", K::settingsConfirm, confirm},
      {"Scroll title", K::settingsScroll, scroll},
      {"Minimize hides secondaries", K::settingsMinimize, minimize},
  };
  for (const Toggle& toggle : rows) {
    const int i = int(&toggle - rows);
    const QRectF row(pane.left() + 16, pane.top() + 12 + i * 36, pane.width() - 32, 32);
    p.setFont(condensedFont(12, 0.08));
    p.setPen(T().ink);
    p.drawText(row, Qt::AlignVCenter, QString::fromLatin1(toggle.label));
    const BtnFace face = faceOf(phases, toggle.kind, toggle.on);
    const QRectF sw(row.right() - 40, row.center().y() - 10, 36, 20);
    // The thumb slides the width of the track rather than teleporting between
    // its ends, which is the whole reason this switch is worth animating.
    fillRound(p, sw, 10,
              scaled(mix(T().btnIdle48, T().phosDeep, face.on), 1 + 0.28 * face.hover));
    p.setBrush(mix(T().inkDim, T().phos, face.on));
    p.setPen(Qt::NoPen);
    const qreal travel = sw.width() - 20;
    p.drawEllipse(QPointF(sw.left() + 10 + travel * face.on, sw.center().y()), 7, 7);
  }
  p.setFont(condensedFont(12, 0.08));
  p.setPen(T().ink);
  p.drawText(QRectF(pane.left() + 16, pane.top() + 168, pane.width() - 32, 20),
             Qt::AlignVCenter, QStringLiteral("Dock snap strength"));
  const char* segs[] = {"Off", "Normal", "Strong"};
  const ChromeHit::Kind segKinds[] = {K::settingsSnapOff, K::settingsSnapNormal,
                                      K::settingsSnapStrong};
  qreal sx = pane.left() + 16;
  for (int i = 0; i < 3; ++i) {
    drawBtn(p, QRectF(sx, pane.top() + 194, 88, 28), faceOf(phases, segKinds[i], i == snap),
            QString::fromLatin1(segs[i]));
    sx += 96;
  }
  p.setFont(condensedFont(12, 0.1));
  p.setPen(T().accent);
  p.drawText(QRectF(body.left() + 12, body.bottom() - 36, 160, 24), Qt::AlignVCenter,
             QStringLiteral("Reset Settings"));
}

void paintAbout(QPainter& p, const QRectF& body, const QImage* logo, const SessionView& view) {
  const QRectF inner = body.adjusted(16, 14, -16, -14);
  const QRectF badge(inner.left(), inner.top(), 58, 58);
  drawDiscLogo(p, badge, logo, false);

  const qreal textLeft = badge.right() + 15;
  drawStyledText(p, QRectF(badge.right() + 18, inner.top() + 4, 220, 32), QStringLiteral("TRAMP"),
                 condensedFont(28, 0.22), T().wordmark, Qt::AlignLeft | Qt::AlignVCenter,
                 {
                     {withAlpha(T().hoverLift, 77), QPointF(0, -1), 0},
                     {QColor(0, 0, 0, 217), QPointF(0, 2), 0},
                     {withAlpha(T().phos, 87), QPointF(), 10},
                 });

  const QString words = QStringLiteral("THE RIDICULOUSLY ATTRACTIVE MUSIC PLAYER");
  const QFont backFont = condensedFont(10, 0);
  QFont spaced = backFont;
  spaced.setLetterSpacing(QFont::AbsoluteSpacing, 1.9);
  QFontMetricsF fm(spaced);
  qreal tx = textLeft;
  const qreal ty = inner.top() + 32 + 9;
  for (const QString& word : words.split(QChar(' '))) {
    const QString initial = word.left(1);
    const QString rest = word.mid(1) + QStringLiteral(" ");
    drawStyledText(p, QRectF(tx, ty, fm.horizontalAdvance(initial) + 4, 14), initial, spaced,
                   T().phos, Qt::AlignLeft | Qt::AlignVCenter,
                   {{withAlpha(T().phos, 115), QPointF(), 7}});
    tx += fm.horizontalAdvance(initial);
    p.setFont(spaced);
    p.setPen(T().inkDim);
    p.drawText(QPointF(tx, ty + fm.ascent()), rest);
    tx += fm.horizontalAdvance(rest);
  }

  const QString ver = QLatin1String("V ") + QLatin1String(TRAMP_VERSION);
  const QFont verFont = monoFont(10, 0.1);
  const qreal verW = 7 + textWidth(verFont, ver) + 7;
  const qreal verH = 22;
  const QRectF verBox(inner.right() - verW, inner.top() + (58 - verH) / 2, verW, verH);
  fillRound(p, verBox, 2, T().well);
  p.setPen(QPen(QColor(0, 0, 0, 179), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(verBox.adjusted(0.5, 0.5, -0.5, -0.5), 2, 2);
  drawGlowText(p, verBox, ver, verFont, T().phos, withAlpha(T().phos, 140), 7, Qt::AlignCenter);

  const qreal taglineTop = inner.top() + 58 + 12;
  {
    constexpr qreal ts = 10.5 / 10.0;
    p.save();
    p.translate(inner.left(), taglineTop);
    p.scale(ts, ts);
    p.setFont(monoFont(10));
    p.setPen(QColor(T().ink.red(), T().ink.green(), T().ink.blue(), 214));
    p.drawText(QRectF(0, 0, inner.width() / ts, 16 / ts), Qt::AlignVCenter,
               QStringLiteral("Local files, honest tags, and chrome you can feel."));
    p.restore();
  }

  constexpr qreal plateH = 48;
  constexpr qreal gap = 12;
  const QRectF well(inner.left(), taglineTop + 16 + gap, inner.width(),
                    inner.bottom() - plateH - gap - (taglineTop + 16 + gap));
  drawScreenWell(p, well);
  {
    QFont kicker = condensedFont(9, 0);
    kicker.setLetterSpacing(QFont::AbsoluteSpacing, 2.6);
    p.setFont(kicker);
    p.setPen(withAlpha(T().phos, 153));
    p.drawText(QRectF(well.left(), well.top() + 11, well.width(), 14), Qt::AlignHCenter,
               QStringLiteral("ON THIS MACHINE"));
  }
  struct Stat {
    const char* label;
    QString value;
  };
  QString playlists = groupedInt(view.aboutPlaylists);
  QString tracks = groupedInt(view.aboutTracks);
  QString totalTime = formatTotalTime(view.aboutTimeMs);
  QString spins = groupedInt(view.aboutSpins);
  if (view.goldenDemo) {
    playlists = QStringLiteral("12");
    tracks = QStringLiteral("1,284");
    totalTime = QStringLiteral("3 d 22 h");
    spins = QStringLiteral("4,096");
  }
  const Stat stats[] = {
      {"PLAYLISTS", playlists},
      {"TRACKS", tracks},
      {"TOTAL TIME", totalTime},
      {"SPINS", spins},
  };
  const QFont labFont = condensedFont(10, 0);
  QFont labSpaced = labFont;
  labSpaced.setLetterSpacing(QFont::AbsoluteSpacing, 1.8);
  const QFont valFont = monoFont(11);
  for (int i = 0; i < 4; ++i) {
    const QRectF row(well.left() + 18, well.top() + 28 + i * ((well.height() - 40) / 4.0),
                     well.width() - 36, (well.height() - 40) / 4.0);
    p.setFont(labSpaced);
    p.setPen(T().inkDim);
    p.drawText(row, Qt::AlignVCenter | Qt::AlignLeft, QString::fromLatin1(stats[i].label));
    drawGlowText(p, row, stats[i].value, valFont,
                 withAlpha(T().phos, 230), withAlpha(T().phos, 77), 6,
                 Qt::AlignVCenter | Qt::AlignRight);
    const QFontMetricsF lm(labSpaced);
    const QFontMetricsF vm(valFont);
    const qreal left =
        row.left() + lm.horizontalAdvance(QString::fromLatin1(stats[i].label)) + 9;
    const qreal right =
        row.right() - vm.horizontalAdvance(stats[i].value) - 9;
    for (qreal x = right; x >= left; x -= 4) {
      p.fillRect(QRectF(x, row.center().y(), 1, 1), QColor(T().inkDim.red(), T().inkDim.green(),
                                                          T().inkDim.blue(), 102));
    }
  }
  drawScreenOverlay(p, well);

  const QRectF plate(inner.left(), inner.bottom() - 48, inner.width(), 48);
  QLinearGradient pg(plate.topLeft(), plate.bottomLeft());
  pg.setColorAt(0, T().plateFace);
  pg.setColorAt(1, T().shellLo);
  fillRound(p, plate, 4, pg);
  p.setPen(QPen(QColor(0, 0, 0, 153), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(plate.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  p.setPen(QPen(QColor(T().coolSheen.red(), T().coolSheen.green(), T().coolSheen.blue(), 15), 1));
  p.drawLine(QPointF(plate.left() + 4, plate.top() + 1),
             QPointF(plate.right() - 4, plate.top() + 1));
  const QImage proxima = loadProximaMark();
  QRectF mark(plate.left() + 13, plate.center().y() - 13.5, 48, 27);
  if (!proxima.isNull()) {
    const qreal aspect = qreal(proxima.width()) / qMax(1, proxima.height());
    mark.setWidth(27 * aspect);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(mark, proxima);
  }
  const qreal companyLeft = mark.right() + 12;
  constexpr qreal companyScale = 11.5 / 11.0;
  QFont company = condensedFont(11, 0);
  company.setLetterSpacing(QFont::AbsoluteSpacing, 2.4 / companyScale);
  p.setPen(withAlpha(T().ink, 235));
  p.save();
  p.setFont(company);
  p.translate(companyLeft, plate.top() + 8);
  p.scale(companyScale, companyScale);
  p.drawText(QRectF(0, 0, 220 / companyScale, 16 / companyScale),
             Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("PROXIMA MAGNIFICA"));
  p.restore();
  p.setFont(monoFont(9));
  p.setPen(T().inkDim);
  p.drawText(QRectF(companyLeft, plate.top() + 26, 220, 14), Qt::AlignLeft | Qt::AlignVCenter,
             QStringLiteral("© 2026 Free Forever"));
  const QString web = QStringLiteral("tramp.music");
  const QFont webFont = monoFont(10);
  const QRectF webBox = aboutWebPill(plate, textWidth(webFont, web));
  fillRound(p, webBox, 3, QColor(T().well.red(), T().well.green(), T().well.blue(), 217));
  p.setPen(QPen(withAlpha(T().phos, 71), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(webBox.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
  drawGlowText(p, webBox, web, webFont, T().phos, withAlpha(T().phos, 128), 7, Qt::AlignCenter);
}

}  // namespace

void paintWindowBody(QPainter& painter, WindowId id, QSize logical, const QImage* logo,
                     const SessionView& view, BodyPaint pass, const ChromePhases& phases) {
  const QRectF body = panelBody(logical);
  switch (id) {
    case WindowId::main:
      paintMain(painter, body, view, pass, phases);
      break;
    case WindowId::equalizer:
      paintEq(painter, body, logo, view, pass, phases);
      break;
    case WindowId::playlist:
      paintPlaylist(painter, body, logo, view, phases);
      break;
    case WindowId::settings:
      paintSettings(painter, body, view, phases);
      break;
    case WindowId::about:
      paintAbout(painter, body, logo, view);
      break;
  }
}

}  // namespace tramp
