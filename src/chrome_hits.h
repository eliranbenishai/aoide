#pragma once

#include "session_view.h"
#include "window_spec.h"

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <algorithm>
#include <optional>

namespace aoide {

struct ChromeHit {
  enum class Kind {
    none,
    options,
    skins,
    trackInfo,
    timeToggle,
    mute,
    volume,
    mono,
    eqToggle,
    plToggle,
    seek,
    prev,
    play,
    pause,
    stop,
    next,
    eject,
    shuffle,
    repeat,
    eqOn,
    eqAuto,
    eqPresets,
    eqPreamp,
    eqBand,
    plCollapse,
    plCollectionRow,
    plAddCollection,
    plCreate,
    plRename,
    plRemoveCollection,
    plDivider,
    plTrackRow,
    plAdd,
    plRemove,
    plSave,
    plSort,
    plOptions,
    plPrev,
    plPlay,
    plNext,
    plRefresh,
    settingsGeneral,
    settingsAudio,
    settingsResume,
    settingsConfirm,
    settingsScroll,
    settingsMinimize,
    settingsSnapOff,
    settingsSnapNormal,
    settingsSnapStrong,
    settingsReset,
    settingsAudioDevice,
    settingsExclusive,
    settingsSkinRow,
    settingsSkinRemove,
    settingsSkinScroll,
    settingsSkinAdd,
    settingsSkinsFolder,
    settingsSkinsRefresh,
    aboutWeb,
    plResize,
  };

  Kind kind = Kind::none;
  int index = -1;
  QRect rect;
  /// Which sides a `plResize` grab moves. Off `Kind` so every resize stays one
  /// inert hit for pointer feedback and tooltips.
  quint8 resizeEdges = 0;
};

inline constexpr quint8 kResizeEdgeWest = 1 << 0;
inline constexpr quint8 kResizeEdgeEast = 1 << 1;
inline constexpr quint8 kResizeEdgeSouth = 1 << 2;
inline constexpr quint8 kResizeEdgeNorth = 1 << 3;

inline constexpr int kPlaylistResizeGrip = 18;
inline constexpr int kPlaylistResizeBand = 5;

struct PlaylistResizeEdges {
  bool west = false;
  bool east = false;
  bool north = false;
  bool south = false;
};

inline PlaylistResizeEdges playlistResizeEdgesFromMask(quint8 mask) {
  return {bool(mask & kResizeEdgeWest), bool(mask & kResizeEdgeEast),
          bool(mask & kResizeEdgeNorth), bool(mask & kResizeEdgeSouth)};
}

/// Next playlist rect for a press-and-drag on [edges]. Inactive edges stay
/// put. Size is clamped to [minSize] against the fixed edge — the origin must
/// not keep following the pointer once the drag is past the minimum.
inline QRectF playlistResizeRect(QRectF start, PlaylistResizeEdges edges, QPointF press,
                                 QPointF pointer, QSizeF minSize) {
  const QPointF delta = pointer - press;
  qreal left = start.left();
  qreal top = start.top();
  qreal right = start.left() + start.width();
  qreal bottom = start.top() + start.height();
  if (edges.west) left += delta.x();
  if (edges.east) right += delta.x();
  if (edges.north) top += delta.y();
  if (edges.south) bottom += delta.y();

  const qreal minW = qMax(minSize.width(), qreal(0));
  const qreal minH = qMax(minSize.height(), qreal(0));
  if (right - left < minW) {
    if (edges.west) left = right - minW;
    else right = left + minW;
  }
  if (bottom - top < minH) {
    if (edges.north) top = bottom - minH;
    else bottom = top + minH;
  }
  return QRectF(QPointF(left, top), QPointF(right, bottom));
}

/// Fraction 0..1 along a horizontal well from a pointer in the well's logical space.
inline qreal sliderFractionX(const QRect& track, int x) {
  if (track.width() <= 1) return 0.0;
  return std::clamp((x - track.left()) / double(track.width()), 0.0, 1.0);
}

/// Fraction 0..1 along a vertical well from a pointer in the well's logical space.
inline qreal sliderFractionY(const QRect& track, int y) {
  if (track.height() <= 1) return 0.0;
  return std::clamp((y - track.top()) / double(track.height()), 0.0, 1.0);
}

/// Pointer that sets a well slider on press. The well is the hit target, so this
/// is the press point — not the well center (that snaps every click to ~50%).
inline QPoint sliderPressPoint(const QRect& well, QPoint logical) {
  Q_UNUSED(well);
  return logical;
}

/// The two clock stamps the seek row paints. The row shows elapsed time even
/// while the display well above it shows remaining, and the seek well's left
/// edge is the measured width of the first stamp — so hit-testing and painting
/// read the strings from here rather than each picking their own.
struct SeekStamps {
  QString elapsed;
  QString duration;
};

SeekStamps mainSeekStamps(const SessionView& view);

/// Track Info and Save stay hits so their tooltips can name why they are dead.
/// Everything else that has a kind is live.
inline bool chromeHitEnabled(const ChromeHit& hit, const SessionView& view) {
  if (hit.kind == ChromeHit::Kind::trackInfo) return view.trackInfoEnabled;
  if (hit.kind == ChromeHit::Kind::plSave) return view.playlistAltered;
  return hit.kind != ChromeHit::Kind::none;
}

ChromeHit hitTest(WindowId id, QSize logical, QPoint pos, const SessionView& view);

QRect mainOptionsHit(QSize logical);
QRect mainEqHit(QSize logical);
QRect mainPlHit(QSize logical);

}  // namespace aoide
