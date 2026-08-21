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

/// Main and EQ bodies inset their rows by the same pad.
inline constexpr qreal kBodySidePad = 22;

inline constexpr qreal kMainOptionsTop = 18;
inline constexpr qreal kMainOptionsSize = 26;
inline constexpr qreal kDisplayWellLeft = 96;
inline constexpr qreal kDisplayWellTop = 14;
inline constexpr qreal kDisplayWellW = 705;
inline constexpr qreal kDisplayWellH = 132;

struct MainDisplayRow {
  QRectF options;
  QRectF well;
};

/// The options cog sits in the gutter left of the display well, which takes
/// the rest of the row. The whole well toggles elapsed against remaining.
inline MainDisplayRow layoutMainDisplay(const QRectF& body) {
  MainDisplayRow out;
  out.options = QRectF(body.left() + kBodySidePad, body.top() + kMainOptionsTop, kMainOptionsSize,
                       kMainOptionsSize);
  out.well = QRectF(body.left() + kDisplayWellLeft, body.top() + kDisplayWellTop, kDisplayWellW,
                    kDisplayWellH);
  return out;
}

/// Well 705, inner pad 16 per side, title meta inset 288.
inline constexpr qreal kDisplayTitleClipW = kDisplayWellW - 32 - 288;

/// Moving lines paint on the live pass; static lines stay on the chassis.
inline bool displayTitleOnLivePass(qreal offset) { return offset > 0; }

/// Slider thumbs are painted taller and wider than their groove and centred on
/// it, so a hit region the size of the groove leaves half of the visible grab
/// target dead. Paint and hit-test both derive from these.
inline constexpr qreal kSeekThumbW = 22;
inline constexpr qreal kSeekThumbH = 32;
inline constexpr qreal kVolumeThumbW = 20;
inline constexpr qreal kVolumeThumbH = 30;
/// The equaliser bands run the same way vertically: the thumb is centred on the
/// value point, so at ±12 dB half of it hangs past the end of the well.
inline constexpr qreal kEqBandThumbW = 34;
inline constexpr qreal kEqBandThumbH = 18;

/// Rounds outwards: a well whose edges fall between logical pixels — the seek
/// well starts at the measured width of the elapsed stamp — would otherwise
/// leave the pixels it paints on either end dead.
inline QRect sliderHitRect(const QRectF& track, qreal thumbH) {
  const qreal h = std::max(track.height(), thumbH);
  return QRectF(track.left(), track.center().y() - h / 2, track.width(), h).toAlignedRect();
}

inline constexpr qreal kVolRowTop = 156;
inline constexpr qreal kVolRowH = 40;
inline constexpr qreal kVolMuteW = 40;
inline constexpr qreal kVolLabelGap = 14;
inline constexpr qreal kVolLabelW = 34;
inline constexpr qreal kVolTrackGap = 10;
inline constexpr qreal kVolTrackH = 14;
inline constexpr qreal kVolTrackToMono = 14;
inline constexpr qreal kVolBtnH = 38;
inline constexpr qreal kVolBtnInset = 1;
inline constexpr qreal kMonoBtnW = 86;
inline constexpr qreal kMonoToEqGap = 14;
inline constexpr qreal kPanelBtnW = 74;
inline constexpr qreal kPanelBtnGap = 8;

struct MainVolumeRow {
  QRectF row;
  QRectF mute;
  QRectF label;
  QRectF track;
  QRectF mono;
  QRectF eq;
  QRectF pl;
};

/// Mute and the VOL label flow from the left; PL, EQ and MONO pack from the
/// right; the volume well takes the span between them.
inline MainVolumeRow layoutMainVolumeRow(const QRectF& body) {
  MainVolumeRow out;
  out.row = QRectF(body.left() + kBodySidePad, body.top() + kVolRowTop,
                   body.width() - 2 * kBodySidePad, kVolRowH);
  const QRectF& row = out.row;
  out.mute = QRectF(row.left(), row.top(), kVolMuteW, kVolRowH);
  out.label = QRectF(out.mute.right() + kVolLabelGap, row.top(), kVolLabelW, kVolRowH);
  const qreal btnY = row.top() + kVolBtnInset;
  out.pl = QRectF(row.right() - kPanelBtnW, btnY, kPanelBtnW, kVolBtnH);
  out.eq = QRectF(out.pl.left() - kPanelBtnGap - kPanelBtnW, btnY, kPanelBtnW, kVolBtnH);
  out.mono = QRectF(out.eq.left() - kMonoToEqGap - kMonoBtnW, btnY, kMonoBtnW, kVolBtnH);
  const qreal trackLeft = out.label.right() + kVolTrackGap;
  const qreal trackRight = out.mono.left() - kVolTrackToMono;
  out.track = QRectF(trackLeft, row.center().y() - kVolTrackH / 2, trackRight - trackLeft,
                     kVolTrackH);
  return out;
}

inline constexpr qreal kSeekRowTop = 206;
inline constexpr qreal kSeekRowH = 32;
inline constexpr qreal kSeekStampGap = 14;
inline constexpr qreal kSeekTrackH = 16;

struct MainSeekRow {
  QRectF row;
  QRectF elapsed;
  QRectF duration;
  QRectF track;
};

/// The seek well sits between the two clock stamps, so its left edge moves with
/// the rendered width of the elapsed time: `10:01` pushes it a digit further
/// right than `9:59`, and a skin's LCD face moves it again. Callers pass the
/// measured widths; layout stays independent of the font machinery.
inline MainSeekRow layoutMainSeekRow(const QRectF& body, qreal posW, qreal durW) {
  MainSeekRow out;
  out.row = QRectF(body.left() + kBodySidePad, body.top() + kSeekRowTop,
                   body.width() - 2 * kBodySidePad, kSeekRowH);
  const QRectF& row = out.row;
  out.elapsed = QRectF(row.left(), row.top(), posW, kSeekRowH);
  out.duration = QRectF(row.right() - durW, row.top(), durW, kSeekRowH);
  out.track = QRectF(row.left() + posW + kSeekStampGap, row.center().y() - kSeekTrackH / 2,
                     row.width() - posW - durW - 2 * kSeekStampGap, kSeekTrackH);
  return out;
}

inline constexpr qreal kPlayRowTop = 246;
inline constexpr qreal kPlayRowH = 50;
inline constexpr qreal kPlayBtnW = 66;
inline constexpr qreal kPlayBtnGap = 6;
inline constexpr qreal kPlayPlayW = 78;
inline constexpr qreal kPlayEjectGap = 10;

struct MainTransportRow {
  QRectF row;
  QRectF prev;
  QRectF play;
  QRectF pause;
  QRectF stop;
  QRectF next;
  QRectF eject;
  QRectF shuffle;
  QRectF repeat;
};

/// Transport flows from the left with Eject held off the cluster; the two
/// toggles pack from the right and are sized to their own labels, so callers
/// pass the measured widths.
inline MainTransportRow layoutMainTransportRow(const QRectF& body, qreal shuffleW, qreal repeatW) {
  MainTransportRow out;
  out.row = QRectF(body.left() + kBodySidePad, body.top() + kPlayRowTop,
                   body.width() - 2 * kBodySidePad, kPlayRowH);
  const QRectF& row = out.row;
  qreal x = row.left();
  auto place = [&](qreal w) {
    const QRectF r(x, row.top(), w, kPlayRowH);
    x += w + kPlayBtnGap;
    return r;
  };
  out.prev = place(kPlayBtnW);
  out.play = place(kPlayPlayW);
  out.pause = place(kPlayBtnW);
  out.stop = place(kPlayBtnW);
  out.next = place(kPlayBtnW);
  x += kPlayEjectGap;
  out.eject = place(kPlayBtnW);
  out.repeat = QRectF(row.right() - repeatW, row.top(), repeatW, kPlayRowH);
  out.shuffle = QRectF(out.repeat.left() - kPlayBtnGap - shuffleW, row.top(), shuffleW, kPlayRowH);
  return out;
}

inline constexpr qreal kEqHeaderTop = 16;
inline constexpr qreal kEqHeaderBtnH = 38;
inline constexpr qreal kEqHeaderGap = 8;
inline constexpr qreal kEqCurveLabelGap = 14;
inline constexpr qreal kEqCurveLabelW = 180;
inline constexpr qreal kEqCurveWellW = 372;
inline constexpr qreal kEqCurveWellH = 62;

struct EqHeaderRow {
  QRectF on;
  QRectF autoBtn;
  QRectF presets;
  QRectF curveLabel;
  QRectF curveWell;
};

/// ON / AUTO / PRESETS flow from the left, each sized to its own label, with
/// the CURVE readout after them and the curve well on the right edge.
inline EqHeaderRow layoutEqHeader(const QRectF& body, qreal onW, qreal autoW, qreal presetsW) {
  EqHeaderRow out;
  const qreal y = body.top() + kEqHeaderTop;
  qreal x = body.left() + kBodySidePad;
  out.on = QRectF(x, y, onW, kEqHeaderBtnH);
  x += onW + kEqHeaderGap;
  out.autoBtn = QRectF(x, y, autoW, kEqHeaderBtnH);
  x += autoW + kEqHeaderGap;
  out.presets = QRectF(x, y, presetsW, kEqHeaderBtnH);
  x += presetsW + kEqCurveLabelGap;
  out.curveLabel = QRectF(x, y, kEqCurveLabelW, kEqHeaderBtnH);
  out.curveWell =
      QRectF(body.right() - kBodySidePad - kEqCurveWellW, y, kEqCurveWellW, kEqCurveWellH);
  return out;
}

inline constexpr int kEqBandCount = 11;
inline constexpr qreal kEqBandRowTop = 92;
inline constexpr qreal kEqBandRowH = 196;
inline constexpr qreal kEqScaleW = 36;
inline constexpr qreal kEqScaleH = 14;
inline constexpr qreal kEqBandsLeft = 44;
inline constexpr qreal kEqPreampW = 62;
inline constexpr qreal kEqPreampGap = 16;
inline constexpr qreal kEqBandW = 50;
inline constexpr qreal kEqGainH = 18;
inline constexpr qreal kEqWellH = 148;
inline constexpr qreal kEqBandLabelTop = 166;
inline constexpr qreal kEqBandLabelH = 26;

inline QRectF eqBandRow(const QRectF& body) {
  return QRectF(body.left() + kBodySidePad, body.top() + kEqBandRowTop,
                body.width() - 2 * kBodySidePad, kEqBandRowH);
}

/// The +12 / 0 / −12 marks down the left gutter, level with the top, middle
/// and bottom of the wells beside them.
inline QRectF eqScaleMark(const QRectF& bandRow, int index) {
  return QRectF(bandRow.left(), bandRow.top() + kEqGainH + index * (kEqWellH - kEqScaleH) / 2,
                kEqScaleW, kEqScaleH);
}

struct EqBandColumn {
  QRectF gain;
  QRectF well;
  QRectF label;
};

/// Column 0 is the preamp: wider than the ten bands and set off from them.
inline EqBandColumn eqBandColumn(const QRectF& bandRow, int index) {
  qreal x = bandRow.left() + kEqBandsLeft;
  for (int i = 0; i < index; ++i) {
    x += i == 0 ? kEqPreampW + kEqPreampGap : kEqBandW;
  }
  const qreal w = index == 0 ? kEqPreampW : kEqBandW;
  EqBandColumn out;
  out.gain = QRectF(x, bandRow.top(), w, kEqGainH);
  out.well = QRectF(x, bandRow.top() + kEqGainH, w, kEqWellH);
  out.label = QRectF(x, bandRow.top() + kEqBandLabelTop, w, kEqBandLabelH);
  return out;
}

/// What the pointer may grab on a band. The readout above the well is part of
/// the control the eye sees, so it drags the band, and the thumb hangs half its
/// height below the well at −12 dB — but the rect the hit carries stays the
/// well alone, so the drag still reads the full ±12 dB off the value's domain.
inline QRect bandHitRect(const EqBandColumn& column, qreal thumbH) {
  return QRectF(column.well.left(), column.gain.top(), column.well.width(),
                column.well.bottom() + thumbH / 2 - column.gain.top())
      .toAlignedRect();
}

/// The maker's-plate web pill is sized to its own text, so a fixed-width hit box
/// drifts from it as soon as a skin changes the LCD face. Callers pass the
/// measured text width; layout stays independent of the font machinery.
inline constexpr qreal kAboutWebPadX = 9;
inline constexpr qreal kAboutWebRightInset = 13;
inline constexpr qreal kAboutWebH = 24;

inline QRectF aboutWebPill(const QRectF& plate, qreal textW) {
  const qreal w = kAboutWebPadX * 2 + textW;
  return QRectF(plate.right() - kAboutWebRightInset - w, plate.center().y() - kAboutWebH / 2, w,
                kAboutWebH);
}

inline constexpr int kSkinRowStride = 36;
inline constexpr int kSkinRowH = 32;
inline constexpr int kSkinRowTop = 10;
inline constexpr int kSkinsBtnStackH = 58;
inline constexpr int kSkinsBtnGap = 8;
inline constexpr int kSkinsScrollW = 14;
inline constexpr int kSkinsScrollGap = 10;

/// Everything a panel owns below its title bar. Hit-testing and painting both
/// start here, so it is the one place the title bar is subtracted.
inline QRectF panelBody(QSize logical) {
  return QRectF(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
}

inline QRectF settingsPane(const QRectF& body) {
  return QRectF(body.left() + 108, body.top(), body.width() - 108, body.height() - 40);
}

inline QRectF settingsPane(QSize logical) { return settingsPane(panelBody(logical)); }

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
inline constexpr qreal kPlaylistRowPadX = 16;
inline constexpr qreal kPlaylistRowIndexW = 34;
inline constexpr qreal kPlaylistRowTitleX = 64;
inline constexpr qreal kPlaylistRowTitleGap = 16;
inline constexpr qreal kPlaylistRowTimeMinW = 36;

struct PlaylistRowColumns {
  QRectF index;
  QRectF title;
  QRectF time;
};

/// Index / title / time within one track row. The time column is sized to the
/// widest stamp in the list rather than to a constant: `12:34` needs a character
/// more than `4:12`, and a skin's LCD face widens every digit again. Against a
/// fixed box the right-aligned stamp lost its leading minute digit on anything
/// past ten minutes. Title takes whatever is left.
inline PlaylistRowColumns playlistRowColumns(const QRectF& row, qreal timeTextW) {
  const qreal timeW = std::max(kPlaylistRowTimeMinW, timeTextW);
  PlaylistRowColumns out;
  out.index = QRectF(row.left() + kPlaylistRowPadX, row.top(), kPlaylistRowIndexW, row.height());
  out.time = QRectF(row.right() - kPlaylistRowPadX - timeW, row.top(), timeW, row.height());
  const qreal titleX = row.left() + kPlaylistRowTitleX;
  const qreal titleW = out.time.left() - kPlaylistRowTitleGap - titleX;
  out.title = QRectF(titleX, row.top(), std::max<qreal>(0, titleW), row.height());
  return out;
}
inline constexpr qreal kPlaylistScrollW = 14;
inline constexpr qreal kPlaylistScrollGap = 10;
inline constexpr qreal kPlaylistFooterH = 110;
inline constexpr qreal kPlaylistFooterGap = 10;
inline constexpr qreal kPlaylistPanePad = 12;
inline constexpr qreal kPlaylistStripGap = 8;
inline constexpr qreal kPlaylistStripBtn = 52;
inline constexpr qreal kPlaylistStripBtnH = 52;
inline constexpr qreal kPlaylistStripTotalH = 34;
inline constexpr qreal kPlaylistStripRefresh = 34;
inline constexpr qreal kPlaylistStripSepExtra = 6;
inline constexpr qreal kPlaylistStripSepW = 1;
inline constexpr qreal kPlaylistStripSepAfter = 14;

struct PlaylistStripLayout {
  QRectF add;
  QRectF remove;
  QRectF sep;
  QRectF sort;
  QRectF options;
  QRectF prev;
  QRectF play;
  QRectF next;
  QRectF total;
  QRectF refresh;
};

/// `.pl-total` horizontal pad 18 + label/value gap 12.
inline qreal playlistStripTotalWidth(qreal labelW, qreal valueW) {
  return 18 + labelW + 12 + valueW + 18;
}

/// Playlist Manager button row under the track list. Refresh sits on the
/// right edge; TOTAL sits to its left; the transport cluster is packed from
/// the right against TOTAL, with [kPlaylistStripGap] between Next and TOTAL
/// (mockup `.pl-strip` flex gap).
inline PlaylistStripLayout layoutPlaylistStrip(const QRectF& deckInner, qreal totalW) {
  PlaylistStripLayout out;
  const qreal y = deckInner.top();
  const qreal w = kPlaylistStripBtn;
  const qreal h = kPlaylistStripBtnH;
  const qreal gap = kPlaylistStripGap;
  qreal x = deckInner.left();
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

  const qreal refreshW = kPlaylistStripRefresh;
  const qreal refreshH = kPlaylistStripTotalH;
  out.refresh =
      QRectF(deckInner.right() - refreshW, y + (h - refreshH) / 2, refreshW, refreshH);
  const qreal totalLeft = out.refresh.left() - gap - totalW;
  const qreal cluster = w * 3 + gap * 2;
  const qreal nextRight = totalLeft - gap;
  const qreal prevLeft = nextRight - cluster;
  out.prev = QRectF(prevLeft, y, w, h);
  out.play = QRectF(prevLeft + w + gap, y, w, h);
  out.next = QRectF(prevLeft + 2 * (w + gap), y, w, h);
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

inline constexpr qreal kPlaylistReopenGap = 4;
inline constexpr qreal kPlaylistReopenW = 14;
inline constexpr qreal kPlaylistReopenH = 56;
inline constexpr qreal kPlaylistReopenTop = 12;
inline constexpr qreal kPlaylistDividerW = 8;

/// The column a collapsed collection keeps for the tab that reopens it: the tab
/// with the same gap either side of it.
inline constexpr qreal kPlaylistReopenColumn = kPlaylistReopenGap * 2 + kPlaylistReopenW;

/// The tab that reopens a collapsed collection. Paint and hit-test both come
/// from here: while they were separate copies of the same numbers, the strip
/// could take pixels off the track rows and paint none of them.
inline QRectF playlistReopenTab(const QRectF& body) {
  return QRectF(body.left() + kPlaylistReopenGap, body.top() + kPlaylistReopenTop,
                kPlaylistReopenW, kPlaylistReopenH);
}

/// The track pane takes what the collection leaves. Collapsed, the collection
/// is down to its reopen tab and keeps that tab's column — the tracks running
/// underneath it is what cost the first rows their left edge.
inline QRectF playlistTracksPane(const QRectF& body, qreal collectionW) {
  const qreal gutter =
      collectionW > 0 ? collectionW + kPlaylistDividerW : kPlaylistReopenColumn;
  return QRectF(body.left() + gutter, body.top(), body.width() - gutter, body.height());
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
