#pragma once

#include <QSize>
#include <QtGlobal>

namespace tramp {

inline constexpr int kDefaultZoomPercent = 75;
inline constexpr int kZoomSteps[] = {50, 75, 100, 125, 150, 200, 250, 300};

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

inline constexpr int kTitleBar = 42;
inline constexpr int kShellRadius = 6;

/// Mockup `.time b` is 46px in a 50px slot (`player-mockup-2.html`).
inline constexpr int kElapsedTimePx = 46;
inline constexpr int kElapsedTimeBoxH = 50;

inline constexpr QSize kMainPlayer{825, 348};
inline constexpr QSize kEqualizer{825, 348};
inline constexpr QSize kPlaylistDefault{1073, 696};
inline constexpr QSize kSettings{520, 420};
inline constexpr QSize kAbout{480, 360};
inline constexpr QSize kPlaylistMin{640, 280};
inline constexpr int kPlaylistCollectionMinWidth = 180;
inline constexpr int kPlaylistDividerWidth = 8;
inline constexpr QSize kPlaylistMinWithCollection{
    640 + 8 + 180, 280};

inline QSize zoomed(QSize logical, int zoomPercent) {
  const qreal z = zoomPercent / 100.0;
  return QSize(qRound(logical.width() * z), qRound(logical.height() * z));
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
