#include "chrome_bodies.h"

#include "chrome_layout.h"
#include "look.h"
#include "mockup_draw.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
#include "tramp_version.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHash>
#include <QImage>
#include <QPainterPath>
#include <QVector>
#include <array>
#include <cmath>

namespace tramp {
namespace {

/// Every panel painter below leaves the painter as it found it, the contract
/// `mockup_draw.h` states for the primitives they call. They set a pen and a
/// font per readout and used to leave the last one behind, and the About plate
/// left `QPainter::SmoothPixmapTransform` behind on top of that. Nothing was
/// broken by it, because each readout sets what it draws with before it draws —
/// which was equally true of the playlist footer right up until a dot went in
/// between two of them and the ones after it lost their pen. So each painter
/// holds its own state, and [paintWindowBody] holds it again at the boundary:
/// the net is not the fix, and neither is a reason to skip the other.
const ChromeTokens& T() { return currentLook(); }

/// A control's face. Panels paint from the phase store so a state change
/// cross-fades instead of snapping; a golden dump or a test paints without a
/// live store and takes plain session state.
BtnFace faceOf(const ChromePhases& phases, ChromeHit::Kind kind, bool latched, int index = -1) {
  return phases.live() ? phases.face(kind, index) : BtnFace(latched);
}

QImage loadCachedSkinPreview(const QString& path) {
  struct Slot {
    QDateTime mtime;
    QImage img;
  };
  static QHash<QString, Slot> cache;
  if (path.isEmpty()) return {};
  const QFileInfo fi(path);
  if (!fi.exists()) return {};
  const QDateTime mtime = fi.lastModified();
  const auto it = cache.constFind(path);
  if (it != cache.cend() && it->mtime == mtime && !it->img.isNull()) return it->img;
  QImage img;
  if (!img.load(path) || img.isNull()) return {};
  cache.insert(path, Slot{mtime, img});
  return img;
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

/// Word-wrapped text that says when it has been cut. Text longer than the box
/// has room for is clipped mid-line, and a clipped line reads as a sentence
/// that simply stopped rather than one there is more of; keep the longest
/// prefix that fits and end it in an ellipsis. Text that fits goes through
/// [QPainter::drawText] untouched.
void drawWrappedElided(QPainter& p, const QRectF& box, int flags, const QString& text) {
  const auto fits = [&](const QString& candidate) {
    return p.boundingRect(box, flags, candidate).height() <= box.height();
  };
  if (fits(text)) {
    p.drawText(box, flags, text);
    return;
  }
  // Wrapped height only grows with the text, so the longest prefix that fits
  // can be halved out rather than walked a character at a time.
  const auto cut = [&](int chars) {
    QString head = text.left(chars);
    while (head.endsWith(QLatin1Char(' '))) head.chop(1);
    return head + QStringLiteral("…");
  };
  int keep = 0;
  int most = int(text.size());
  while (keep < most) {
    const int mid = keep + (most - keep + 1) / 2;
    if (fits(cut(mid))) {
      keep = mid;
    } else {
      most = mid - 1;
    }
  }
  p.drawText(box, flags, cut(keep));
}

void paintMain(QPainter& p, const QRectF& body, const SessionView& view, BodyPaint pass,
               const ChromePhases& phases) {
  const PainterStateScope hold(p);
  using K = ChromeHit::Kind;
  const QString time = formatClock(view.showElapsed
                                       ? view.positionMs
                                       : qMax<qint64>(0, view.durationMs - view.positionMs));
  const QString timeLabel =
      view.showElapsed ? QStringLiteral("ELAPSED") : QStringLiteral("REMAIN");
  const QString title = mainEmptyTitle(view);
  const QString subtitle = view.subtitle.toUpper();
  const QString& bitrate = view.bitrate;
  const QString& rate = view.sampleRate;
  const QString& channels = view.channels;
  const QString& fmtLabel = view.formatChip;
  const qreal volume = view.muted ? 0 : view.volume;
  const qreal seek = view.durationMs > 0 ? qreal(view.positionMs) / qreal(view.durationMs) : 0;
  const bool playOn = view.playing;
  const bool pauseOn = view.paused;
  const bool shuffleOn = view.shuffle;
  const bool repeatOn = view.repeat != RepeatMode::off;
  const std::array<qreal, 20>& bars = view.spectrum;
  const std::array<qreal, 20>& peaks = view.spectrumPeaks;

  const bool chassis = pass != BodyPaint::live;
  const bool live = pass != BodyPaint::chassis;
  const bool glow = pass == BodyPaint::full;

  const MainDisplayRow display = layoutMainDisplay(body);
  const QRectF& well = display.well;
  const QRectF inner = well.adjusted(16, 12, -16, -12);

  if (chassis) {
    drawScreenWell(p, well);
    drawGlyphBtn(p, display.options, MockupIcon::options, faceOf(phases, K::options, false), 16);
    drawGlyphBtn(p, display.skins, MockupIcon::skins, faceOf(phases, K::skins, view.skinsOn), 16);
    drawGlyphBtn(p, display.trackInfo, MockupIcon::trackInfo, faceOf(phases, K::trackInfo, false), 16,
                 view.trackInfoEnabled);
    const QRectF marks = displayWellMarks(inner);
    qreal markX = marks.left();
    auto paintMark = [&](const QString& label) {
      const QFont font = condensedFont(11, 0.2);
      const qreal w = textWidth(font, label) + 2;
      const QRectF box(markX, marks.top(), w, marks.height());
      if (glow) {
        drawStyledText(p, box, label, font, withAlpha(T().accent, 200),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       {{withAlpha(T().accent, 0x40), QPointF(), 6}});
      } else {
        p.setFont(font);
        p.setPen(T().accent);
        p.drawText(box, Qt::AlignLeft | Qt::AlignVCenter, label);
      }
      markX += w + 12;
    };
    if (view.spectrumUnmeasured) paintMark(QStringLiteral("UNMEAS"));
    if (view.noAudioEngine) paintMark(QStringLiteral("NO AUDIO"));
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

    const QRectF viz(inner.left(), inner.bottom() - kDisplayWellVizH, 248, kDisplayWellVizH);
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
    const SeekStamps stamps = mainSeekStamps(view);
    const MainSeekRow row = layoutMainSeekRow(body, sm.horizontalAdvance(stamps.elapsed),
                                              sm.horizontalAdvance(stamps.duration));
    p.setFont(stamp);
    p.setPen(T().inkDim);
    p.drawText(row.elapsed, Qt::AlignVCenter, stamps.elapsed);
    p.drawText(row.duration, Qt::AlignVCenter, stamps.duration);
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
  const PainterStateScope hold(p);
  using K = ChromeHit::Kind;
  const bool on = view.eq.enabled;
  const bool autoOn = view.eq.auto_;
  const QString curveName =
      view.eq.presetName.isEmpty() ? QStringLiteral("CUSTOM") : view.eq.presetName.toUpper();
  const qreal preamp = view.eq.preamp;
  const std::array<double, EqualizerSettings::kBandCount>& gains = view.eq.gains;

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
    pts.reserve(10);
    auto yFor = [&](qreal g) {
      const qreal t = (12.0 - qBound(-12.0, g, 12.0)) / 24.0;
      return curveWell.top() + t * curveWell.height();
    };
    for (int i = 0; i < 10; ++i) {
      pts.push_back(QPointF(curveWell.left() + curveWell.width() * i / 9.0,
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
    const qreal wellR = T().surfaceRadius(curveWell);
    wellClip.addRoundedRect(curveWell, wellR, wellR);
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

/// Two centred lines in a well that has no rows. Heading in the faint
/// condensed face the PLAYLISTS label already uses; body one step dimmer so
/// the sentence reads as chrome, not as a phosphor track.
void paintEmptyWellCopy(QPainter& p, const QRectF& well, const EmptyWellCopy& copy) {
  const QFont head = condensedFont(12, 0.18);
  const QFont body = condensedFont(11, 0.08);
  const qreal pad = 16;
  const qreal headH = 20;
  const qreal bodyH = 40;
  const qreal top = well.center().y() - (headH + bodyH) / 2;
  const QRectF headBox(well.left() + pad, top, well.width() - 2 * pad, headH);
  const QRectF bodyBox(well.left() + pad, top + headH, well.width() - 2 * pad, bodyH);
  p.setFont(head);
  p.setPen(T().inkFaint);
  p.drawText(headBox, Qt::AlignHCenter | Qt::AlignVCenter, copy.heading);
  p.setFont(body);
  p.setPen(T().inkDim);
  p.drawText(bodyBox, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, copy.body);
}

void paintPlaylist(QPainter& p, const QRectF& body, const QImage* logo, const SessionView& view,
                   const ChromePhases& phases) {
  const PainterStateScope hold(p);
  using K = ChromeHit::Kind;
  const QVector<CollectionRowView>& lists = view.collection;
  const QVector<TrackRowView>& rows = view.tracks;
  const QString totalText = formatClock(view.playlistTotalMs);
  QString statusName = view.playlistName.toUpper();
  if (statusName.isEmpty()) statusName = QStringLiteral("UNTITLED");
  if (view.playlistAltered) statusName += QStringLiteral(" *");
  const int playingN = view.playingIndex ? *view.playingIndex + 1 : 0;
  const qreal collectionW = view.collectionCollapsed ? 0 : view.collectionWidth;

  const QRectF collection = playlistCollectionColumn(body, collectionW);
  const QRectF divider(collection.right(), collection.top(),
                       collectionW > 0 ? kPlaylistDividerW : 0, collection.height());
  const QRectF tracksPane = playlistTracksPane(body, collectionW);
  if (collectionW > 0) {
    p.fillRect(divider, T().shellDeep);
    drawFooterSep(p, QRectF(divider.center().x() - 1, divider.top() + 24, 1,
                            divider.height() - 48));

  const QRectF colInner = playlistCollectionInner(collection);
  p.setFont(condensedFont(11, 0.2));
  p.setPen(T().inkFaint);
  p.drawText(QRectF(colInner.left(), colInner.top(), colInner.width() - 30, 20),
             Qt::AlignVCenter, QStringLiteral("PLAYLISTS"));
  const QRectF collapse(colInner.right() - 24, colInner.top(), 24, 20);
  drawBtn(p, collapse, faceOf(phases, K::plCollapse, false), {});
  drawChevron(p, QRectF(collapse.center().x() - 2.8, collapse.center().y() - 4, 5.6, 8), true,
              T().glyphInk);

  const QRectF colWell = playlistCollectionWell(colInner);
  drawScreenWell(p, colWell);
  p.save();
  QPainterPath colClip;
  const qreal colR = T().surfaceRadius(colWell);
  colClip.addRoundedRect(colWell, colR, colR);
  p.setClipPath(colClip);
  if (lists.isEmpty()) {
    paintEmptyWellCopy(p, colWell, collectionEmptyCopy());
  }
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
  } else {
    const QRectF tab = playlistReopenTab(body);
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
  const qreal listR = T().surfaceRadius(listWell);
  clip.addRoundedRect(listWell, listR, listR);
  p.setClipPath(clip);
  const int scrollRows = view.trackScroll;
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
  if (rows.isEmpty()) {
    paintEmptyWellCopy(p, listWell, playlistEmptyCopy());
  }
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
                      qreal iconSize, bool on = false, bool enabled = true) {
    drawGlyphBtn(p, r, icon, faceOf(phases, kind, on), iconSize, enabled);
    if (menu) {
      drawMenuCaret(p, r);
    }
  };
  paintBtn(strip.save, MockupIcon::save, K::plSave, false, 21, false, view.playlistAltered);
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
  const bool refreshEnabled = view.playlistRefreshEnabled;
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
  const QString tracksText = QStringLiteral("%1 TRACKS").arg(view.playlistTrackCount);
  const QString playingText =
      playingN > 0 ? QStringLiteral("PLAYING %1").arg(playingN) : QStringLiteral("STOPPED");
  const QString drop = QStringLiteral("DROP FILES HERE TO ENQUEUE");
  const qreal tracksW = textWidth(statusFont, tracksText);
  const qreal playingW = textWidth(statusFont, playingText);
  // A playlist can be named anything, and the run had nothing clipping it, so a
  // long enough name on a narrow panel painted off the right edge of the strip.
  // The name is what gives way: the readouts after it are the information the
  // strip exists to carry.
  const qreal nameRoom = playlistStatusNameWidth(status, tracksW, playingW);
  if (textWidth(statusFont, statusName) > nameRoom) {
    statusName = QFontMetricsF(statusFont).elidedText(statusName, Qt::ElideRight, nameRoom);
  }
  const PlaylistStatusRun run =
      layoutPlaylistStatus(status, textWidth(statusFont, statusName), tracksW, playingW,
                           textWidth(statusFont, drop));
  p.drawText(run.name, Qt::AlignVCenter, statusName);
  drawStatusDot(p, run.nameDot);
  p.drawText(run.tracks, Qt::AlignVCenter, tracksText);
  drawStatusDot(p, run.tracksDot);
  p.drawText(run.playing, Qt::AlignVCenter, playingText);
  // The hint is the one part of the strip that says nothing about this
  // playlist, so it is what goes first — whole, rather than crowded.
  if (!run.drop.isEmpty()) {
    drawStatusDot(p, run.dropDot);
    p.drawText(run.drop, Qt::AlignVCenter, drop);
  }
}

void paintSettings(QPainter& p, const QRectF& body, const SessionView& view,
                   const ChromePhases& phases) {
  const PainterStateScope hold(p);
  using K = ChromeHit::Kind;
  const int tabIndex = view.settingsTab;
  const bool resume = view.resumeLastSession;
  const bool confirm = view.confirmBeforeQuit;
  const bool scroll = view.scrollTitle;
  const bool minimize = view.minimizeHidesSecondaries;
  const int snap = view.dockSnap;

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
  tab(body.top() + 42, QStringLiteral("Audio"), K::settingsAudio, tabIndex == 1);

  const QRectF pane = settingsPane(body);
  auto paintReset = [&]() {
    p.setFont(condensedFont(12, 0.1));
    p.setPen(T().accent);
    p.drawText(QRectF(body.left() + 12, body.bottom() - 36, 160, 24), Qt::AlignVCenter,
               QStringLiteral("Reset Settings"));
  };
  if (tabIndex == 1) {
    paintReset();
    return;
  }
  struct Toggle {
    QString label;
    ChromeHit::Kind kind;
    bool on;
  };
  const Toggle rows[] = {
      {resumePlaybackLabel(), K::settingsResume, resume},
      {QStringLiteral("Confirm before quit"), K::settingsConfirm, confirm},
      {QStringLiteral("Scroll title"), K::settingsScroll, scroll},
      {QStringLiteral("Minimize hides secondaries"), K::settingsMinimize, minimize},
  };
  for (const Toggle& toggle : rows) {
    const int i = int(&toggle - rows);
    const QRectF row(pane.left() + 16, pane.top() + 12 + i * 36, pane.width() - 32, 32);
    p.setFont(condensedFont(12, 0.08));
    p.setPen(T().ink);
    p.drawText(row, Qt::AlignVCenter, toggle.label);
    const BtnFace face = faceOf(phases, toggle.kind, toggle.on);
    const QRectF sw(row.right() - 40, row.center().y() - 10, 36, 20);
    // The thumb slides the width of the track rather than teleporting between
    // its ends, which is the whole reason this switch is worth animating.
    fillRound(p, sw, 10,
              scaled(mix(T().btnIdle48, T().phosDeep, face.on), 1 + 0.28 * face.hover));
    const PainterStateScope thumb(p);
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
  if (view.persistWriteFailed) {
    const QRectF mark = settingsPersistMark(pane);
    p.setFont(monoFont(10));
    p.setPen(T().accent);
    p.drawText(mark, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Could not write settings"));
  }
  paintReset();
}

void paintSkins(QPainter& p, const QRectF& body, const SessionView& view,
                const ChromePhases& phases) {
  const PainterStateScope hold(p);
  using K = ChromeHit::Kind;
  const QRectF pane = skinsPane(body);
  const QVector<SkinCatalogEntry>& skins = view.skins;
  const QString& active = view.activeSkinId;
  const QRectF viewport = skinsListViewport(pane);
  const int scroll = view.skinsScroll;
  p.save();
  p.setClipRect(viewport);
  for (int i = 0; i < skins.size(); ++i) {
    const QRectF cell = skinsGridCell(viewport, i, scroll);
    if (cell.bottom() < viewport.top() || cell.top() > viewport.bottom()) continue;
    const bool on = skins[i].id == active;
    const BtnFace hover = on ? BtnFace() : faceOf(phases, K::settingsSkinRow, false, i);
    p.fillRect(cell, T().shell);
    if (on) {
      p.setPen(QPen(T().phos, 2));
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(cell.adjusted(1, 1, -1, -1), 3, 3);
    } else if (hover.hover > 0) {
      p.fillRect(cell, mix(T().shell, T().hoverLift, 0.16 * hover.hover));
    }
    const QRectF photo = skinsGridPhotoRect(cell);
    const QImage preview = loadCachedSkinPreview(skins[i].previewPath);
    if (!preview.isNull()) {
      p.drawImage(photo, preview);
    } else {
      p.fillRect(photo, T().shellMid);
    }
    if (skins[i].canRemove) {
      const QRectF trash = skinsGridTrashcan(cell);
      const BtnFace face = faceOf(phases, K::settingsSkinRemove, false, i);
      fillRound(p, trash, T().buttonRadius(trash),
                mix(T().shell, T().shellHi, 0.35 + 0.25 * face.hover));
      p.setPen(QPen(T().ink, 1.15));
      p.setBrush(Qt::NoBrush);
      const QRectF can = trash.adjusted(5, 6, -5, -3);
      p.drawRoundedRect(can, 1.2, 1.2);
      p.drawLine(QPointF(trash.left() + 4, trash.top() + 6),
                 QPointF(trash.right() - 4, trash.top() + 6));
      p.drawLine(QPointF(trash.center().x() - 2, trash.top() + 4),
                 QPointF(trash.center().x() + 2, trash.top() + 4));
    }
  }
  p.restore();
  const int maxScroll = skinsListMaxScroll(skins.size(), viewport);
  if (maxScroll > 0) {
    const QRectF track = skinsListScrollTrack(viewport);
    const QRectF thumb = skinsListThumb(track, viewport, skins.size(), scroll);
    drawScrollbar(p, track, thumb.top() - track.top(), thumb.height());
  }
  if (!view.skinsError.isEmpty()) {
    const QRectF strip = skinsErrorStrip(viewport);
    p.save();
    p.setClipRect(strip);
    p.setFont(monoFont(10));
    p.setPen(T().accent);
    drawWrappedElided(p, strip, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                      view.skinsError);
    p.restore();
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
}

void paintAbout(QPainter& p, const QRectF& body, const QImage* logo, const SessionView& view) {
  const PainterStateScope hold(p);
  const QRectF inner = body.adjusted(16, 14, -16, -14);
  const QRectF badge(inner.left(), inner.top(), 58, 58);
  drawDiscLogo(p, badge, logo, false);

  const qreal textLeft = badge.right() + 15;
  drawStyledText(p, QRectF(badge.right() + 18, inner.top() + 4, 220, 32), QStringLiteral("TRAMP"),
                 brandFont(28), T().wordmark, Qt::AlignLeft | Qt::AlignVCenter,
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

  constexpr qreal plateH = 48;
  constexpr qreal gap = 12;
  const qreal wellTop = inner.top() + 58 + 12;
  const QRectF well(inner.left(), wellTop, inner.width(),
                    inner.bottom() - plateH - gap - wellTop);
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
  const QString playlists = groupedInt(view.aboutPlaylists);
  const QString tracks = groupedInt(view.aboutTracks);
  const QString totalTime = formatTotalTime(view.aboutTimeMs);
  const QString spins = groupedInt(view.aboutSpins);
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
    // The mark is the one thing on the plate that is a scaled bitmap, so the
    // hint belongs to it and to nothing drawn after it.
    const PainterStateScope smooth(p);
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
  // Each painter below already puts back what it sets. This is the net under
  // them, so a panel that grows a readout tomorrow cannot reach the caller
  // through this door whatever it forgets.
  const PainterStateScope hold(painter);
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
    case WindowId::skins:
      paintSkins(painter, body, view, phases);
      break;
  }
}

}  // namespace tramp
