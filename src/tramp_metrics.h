#pragma once

#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace tramp {

inline constexpr int kDefaultZoomPercent = 75;
inline constexpr int kZoomSteps[] = {75, 100, 125, 150};

inline int nextZoomPercent(int current) {
  for (int step : kZoomSteps) {
    if (step > current) {
      return step;
    }
  }
  return kZoomSteps[sizeof(kZoomSteps) / sizeof(kZoomSteps[0]) - 1];
}

inline int prevZoomPercent(int current) {
  int found = kZoomSteps[0];
  for (int step : kZoomSteps) {
    if (step < current) {
      found = step;
    }
  }
  return found;
}

/// A zoom percent restored from an older build may name a step the ladder no
/// longer carries. Snap it onto the nearest surviving step; a tie keeps the
/// smaller one, which is the step more likely to fit the display.
inline int snapZoomPercent(int percent) {
  int nearest = kZoomSteps[0];
  for (int step : kZoomSteps) {
    if (qAbs(step - percent) < qAbs(nearest - percent)) {
      nearest = step;
    }
  }
  return nearest;
}

inline constexpr int kTitleBar = 42;
inline constexpr int kShellRadius = 6;

/// Dark wells (display, track list, saved playlists, EQ curve) carry the panel
/// corner, not the tighter 3px the mockup used.
inline constexpr qreal kWellRadius = kShellRadius;
inline constexpr qreal kButtonRadius = 4;

/// Requested radius, or a fraction of the shorter side, whichever is smaller.
/// Zero and empty rects stay sharp.
inline qreal cappedCornerRadius(QSizeF size, qreal requested, qreal maxFraction) {
  if (requested <= 0 || size.width() <= 0 || size.height() <= 0) return 0;
  return qMin(requested, qMin(size.width(), size.height()) * maxFraction);
}

/// Windows and dark wells stay rectangular: at most a quarter of the shorter
/// side, so at least half of that side remains a straight edge. The cap scales
/// with the element's current size.
inline qreal rectangularCornerRadius(QSizeF size, qreal requested) {
  return cappedCornerRadius(size, requested, 0.25);
}

inline qreal insetCornerRadius(qreal radius, qreal inset) {
  return qMax(qreal(0), radius - inset);
}

/// Mockup `.time b` is 46px in a 50px slot (`player-mockup-2.html`).
inline constexpr int kElapsedTimePx = 46;
inline constexpr int kElapsedTimeBoxH = 50;

inline constexpr QSize kMainPlayer{825, 348};
inline constexpr QSize kEqualizer{825, 348};
inline constexpr QSize kPlaylistDefault{1073, 696};
inline constexpr QSize kSettings{520, 420};
inline constexpr QSize kAbout{480, 360};
inline constexpr QSize kSkins{600, 480};
/// Compact-strip floor with a reserved TOTAL well (180). Host uses
/// [playlistMinLogical] with the measured well so a short list can go narrower.
inline constexpr QSize kPlaylistMin{585, 280};
inline constexpr int kPlaylistCollectionMinWidth = 180;
inline constexpr int kPlaylistDividerWidth = 8;
inline constexpr QSize kPlaylistMinWithCollection{751, 280};

inline QSize zoomed(QSize logical, int zoomPercent) {
  const qreal z = zoomPercent / 100.0;
  return QSize(qRound(logical.width() * z), qRound(logical.height() * z));
}

/// Whether a cluster whose logical bounding box is [logical] still fits inside
/// [workArea] once it is scaled to [percent]. An empty work area is not a
/// display of no size, it is not knowing yet: a step is taken off the ladder on
/// evidence, never on a missing answer.
inline bool zoomStepFits(QSizeF logical, QSize workArea, int percent) {
  if (workArea.isEmpty() || logical.isEmpty()) return true;
  const QSize at = zoomed(QSize(qRound(logical.width()), qRound(logical.height())), percent);
  return at.width() <= workArea.width() && at.height() <= workArea.height();
}

/// Backing-store size for chrome rasterized at the widget's device pixels.
inline QSize chromePaintBufferSize(QSize widget, qreal devicePixelRatio) {
  const qreal dpr = qMax(devicePixelRatio, qreal(0.5));
  return QSize(qMax(1, qRound(widget.width() * dpr)),
               qMax(1, qRound(widget.height() * dpr)));
}

/// Rounded 75% seed used as the native unmapped default (same as Dart
/// `TrampMetrics.nativeUnmappedSeed`).
inline QSize nativeUnmappedSeed(QSize logical) {
  return zoomed(logical, kDefaultZoomPercent);
}

}  // namespace tramp
