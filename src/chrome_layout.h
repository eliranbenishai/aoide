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

/// Well 705, inner pad 16 per side, title meta inset 288.
inline constexpr qreal kDisplayTitleClipW = 705 - 32 - 288;

/// Moving lines paint on the live pass; static lines stay on the chassis.
inline bool displayTitleOnLivePass(qreal offset) { return offset > 0; }

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

inline constexpr qreal kPlaylistRowStride = 37;
inline constexpr qreal kPlaylistRowPadTop = 6;
inline constexpr qreal kPlaylistScrollW = 14;
inline constexpr qreal kPlaylistScrollGap = 10;
inline constexpr qreal kPlaylistFooterH = 110;
inline constexpr qreal kPlaylistFooterGap = 10;
inline constexpr qreal kPlaylistPanePad = 12;
inline constexpr qreal kPlaylistStripGap = 8;
inline constexpr qreal kPlaylistStripBtn = 52;
inline constexpr qreal kPlaylistStripBtnH = 52;
inline constexpr qreal kPlaylistStripTotalH = 34;
inline constexpr qreal kPlaylistStripSepExtra = 6;
inline constexpr qreal kPlaylistStripSepW = 1;
inline constexpr qreal kPlaylistStripSepAfter = 14;

struct PlaylistStripLayout {
  QRectF add;
  QRectF remove;
  QRectF sep;
  QRectF sort;
  QRectF options;
  QRectF rail;
  QRectF prev;
  QRectF play;
  QRectF next;
  QRectF total;
};

/// `.pl-total` horizontal pad 18 + label/value gap 12.
inline qreal playlistStripTotalWidth(qreal labelW, qreal valueW) {
  return 18 + labelW + 12 + valueW + 18;
}

/// Playlist Manager button row under the track list. Transport cluster is
/// packed from the right against the length well, with [kPlaylistStripGap]
/// between Next and TOTAL (mockup `.pl-strip` flex gap).
inline PlaylistStripLayout layoutPlaylistStrip(const QRectF& plateInner, qreal totalW) {
  PlaylistStripLayout out;
  const qreal y = plateInner.top();
  const qreal w = kPlaylistStripBtn;
  const qreal h = kPlaylistStripBtnH;
  const qreal gap = kPlaylistStripGap;
  qreal x = plateInner.left();
  auto place = [&](QRectF& r) {
    r = QRectF(x, y, w, h);
    x += w + gap;
  };
  place(out.add);
  place(out.remove);
  x += kPlaylistStripSepExtra;
  out.sep = QRectF(x, y + 6, kPlaylistStripSepW, 40);
  x += kPlaylistStripSepW + kPlaylistStripSepAfter;
  place(out.sort);
  place(out.options);

  const qreal totalLeft = plateInner.right() - totalW;
  const qreal cluster = w * 3 + gap * 2;
  const qreal nextRight = totalLeft - gap;
  const qreal prevLeft = nextRight - cluster;
  out.prev = QRectF(prevLeft, y, w, h);
  out.play = QRectF(prevLeft + w + gap, y, w, h);
  out.next = QRectF(prevLeft + 2 * (w + gap), y, w, h);
  const qreal railRight = prevLeft - gap;
  if (railRight - x > 4) {
    out.rail = QRectF(x, y, railRight - x, h);
  }
  out.total = QRectF(totalLeft, y + (h - kPlaylistStripTotalH) / 2, totalW, kPlaylistStripTotalH);
  return out;
}

inline qreal playlistListWellHeight(qreal playlistLogicalH) {
  const qreal bodyH = playlistLogicalH - kTitleBar;
  const qreal trackInnerH = bodyH - kPlaylistPanePad * 2;
  return trackInnerH - kPlaylistFooterH - kPlaylistFooterGap;
}

inline int playlistVisibleRows(qreal wellH) {
  return std::max(0, int((wellH - kPlaylistRowPadTop) / kPlaylistRowStride));
}

inline int playlistListMaxScroll(int count, qreal wellH) {
  return std::max(0, count - playlistVisibleRows(wellH));
}

inline QRectF playlistTracksPane(const QRectF& body, qreal collectionW) {
  const qreal divider = collectionW > 0 ? 8 : 0;
  return QRectF(body.left() + collectionW + divider, body.top(),
                body.width() - collectionW - divider, body.height());
}

inline QRectF playlistTrackInner(const QRectF& tracksPane) {
  return tracksPane.adjusted(kPlaylistPanePad, kPlaylistPanePad, -kPlaylistPanePad,
                             -kPlaylistPanePad);
}

inline QRectF playlistListRowRect(const QRectF& trackInner) {
  return QRectF(trackInner.left(), trackInner.top(), trackInner.width(),
                trackInner.height() - kPlaylistFooterH - kPlaylistFooterGap);
}

inline QRectF playlistListWell(const QRectF& listRow, int trackCount) {
  const qreal gutter =
      playlistListMaxScroll(trackCount, listRow.height()) > 0
          ? (kPlaylistScrollGap + kPlaylistScrollW)
          : 0;
  return QRectF(listRow.left(), listRow.top(), listRow.width() - gutter, listRow.height());
}

inline QRectF playlistListScrollTrack(const QRectF& listWell) {
  return QRectF(listWell.right() + kPlaylistScrollGap, listWell.top(), kPlaylistScrollW,
                listWell.height());
}

inline QRectF playlistListThumb(const QRectF& track, int count, int scrollRows, qreal wellH) {
  const qreal content = qMax<qreal>(1, count * kPlaylistRowStride);
  const qreal thumbH = qMin(track.height(), track.height() * track.height() / content);
  const int maxScroll = playlistListMaxScroll(count, wellH);
  const qreal t = maxScroll <= 0 ? 0 : qreal(scrollRows) / qreal(maxScroll);
  return QRectF(track.left() + 1, track.top() + t * (track.height() - thumbH), track.width() - 2,
                thumbH);
}

}  // namespace tramp
