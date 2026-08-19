#pragma once

#include "tramp_metrics.h"

#include <QRectF>
#include <QSize>
#include <algorithm>
#include <cmath>

namespace tramp {

inline constexpr qreal kDisplayMetaGap = 22;
inline constexpr qreal kDisplayChipGap = 10;
inline constexpr qreal kDisplayMetaH = 18;

struct DisplayMetaLayout {
  QRectF bitrate;
  QRectF rate;
  QRectF channels;
  QRectF playlist;
  QRectF format;
};

/// Bitrate / rate flow from the left; format, playlist chip, and STEREO pack from
/// the right. The STEREO–PLAYLIST gap is never smaller than [kDisplayMetaGap],
/// even when a skin's font makes the strings wider than the builtin face.
inline DisplayMetaLayout layoutDisplayMetaRow(const QRectF& inner, qreal y, qreal bitrateW,
                                              qreal rateW, qreal channelsW, qreal playlistW,
                                              qreal formatW) {
  DisplayMetaLayout out;
  out.format = QRectF(inner.right() - formatW, y, formatW, kDisplayMetaH);
  out.playlist =
      QRectF(out.format.left() - kDisplayChipGap - playlistW, y, playlistW, kDisplayMetaH);
  out.bitrate = QRectF(inner.left(), y, bitrateW, kDisplayMetaH);
  out.rate = QRectF(out.bitrate.right() + kDisplayMetaGap, y, rateW, kDisplayMetaH);
  const qreal flowX = out.rate.right() + kDisplayMetaGap;
  const qreal anchoredX = out.playlist.left() - kDisplayMetaGap - channelsW;
  const qreal x = (flowX + channelsW + kDisplayMetaGap <= out.playlist.left()) ? flowX : anchoredX;
  out.channels = QRectF(x, y, channelsW, kDisplayMetaH);
  return out;
}

inline constexpr qint64 kMarqueeHoldMs = 1200;
inline constexpr qreal kMarqueePxPerSec = 40;
inline constexpr qreal kMarqueeGap = 40;

inline qreal marqueeLoopWidth(qreal textWidth, qreal clipWidth) {
  if (textWidth <= clipWidth) return 0;
  return textWidth + kMarqueeGap;
}

/// Pixels to shift an overflowing display-well line left. Holds at the start,
/// then loops with [kMarqueeGap] between copies. Disabled or fitting text stays 0.
inline qreal marqueeOffset(qreal textWidth, qreal clipWidth, qint64 elapsedMs, bool enabled) {
  if (!enabled || textWidth <= clipWidth || elapsedMs <= 0) return 0;
  const qreal loop = marqueeLoopWidth(textWidth, clipWidth);
  if (loop <= 0) return 0;
  if (elapsedMs <= kMarqueeHoldMs) return 0;
  const qreal scrolled = qreal(elapsedMs - kMarqueeHoldMs) / 1000.0 * kMarqueePxPerSec;
  return std::fmod(scrolled, loop);
}

inline constexpr int kSkinRowStride = 36;
inline constexpr int kSkinRowH = 32;
inline constexpr int kSkinRowTop = 10;
inline constexpr int kSkinsBtnStackH = 58;
inline constexpr int kSkinsBtnGap = 8;
inline constexpr int kSkinsScrollW = 14;
inline constexpr int kSkinsScrollGap = 10;

inline QRectF settingsBody(QSize logical) {
  return QRectF(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
}

inline QRectF settingsPane(QSize logical) {
  const QRectF body = settingsBody(logical);
  return QRectF(body.left() + 108, body.top(), body.width() - 108, body.height() - 40);
}

inline QRectF skinsListViewport(const QRectF& pane) {
  const qreal top = pane.top() + kSkinRowTop;
  const qreal bottom = pane.bottom() - kSkinsBtnStackH - kSkinsBtnGap;
  const qreal scroll = kSkinsScrollGap + kSkinsScrollW;
  return QRectF(pane.left() + 12, top, qMax<qreal>(0, pane.width() - 24 - scroll),
                qMax<qreal>(0, bottom - top));
}

inline QRectF skinsListScrollTrack(const QRectF& viewport) {
  return QRectF(viewport.right() + kSkinsScrollGap, viewport.top(), kSkinsScrollW,
                viewport.height());
}

inline int skinsListContentH(int count) { return count * kSkinRowStride; }

inline int skinsListMaxScroll(int count, qreal viewportH) {
  return std::max(0, skinsListContentH(count) - int(viewportH));
}

inline QRectF skinsListRow(const QRectF& viewport, int index, int scroll) {
  return QRectF(viewport.left(), viewport.top() + index * kSkinRowStride - scroll, viewport.width(),
                kSkinRowH);
}

inline QRectF skinsListThumb(const QRectF& track, int count, int scroll) {
  const qreal content = qMax<qreal>(1, skinsListContentH(count));
  const qreal thumbH = qMin(track.height(), track.height() * track.height() / content);
  const int maxScroll = skinsListMaxScroll(count, track.height());
  const qreal t = maxScroll <= 0 ? 0 : qreal(scroll) / qreal(maxScroll);
  const qreal thumbTop = t * (track.height() - thumbH);
  return QRectF(track.left() + 1, track.top() + thumbTop, track.width() - 2, thumbH);
}

}  // namespace tramp
