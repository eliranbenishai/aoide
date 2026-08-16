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

inline constexpr QSize kMainPlayer{825, 348};
inline constexpr QSize kEqualizer{825, 348};
inline constexpr QSize kPlaylistDefault{1073, 696};
inline constexpr QSize kSettings{520, 420};
inline constexpr QSize kAbout{480, 360};

inline QSize zoomed(QSize logical, int zoomPercent) {
  const qreal z = zoomPercent / 100.0;
  return QSize(qRound(logical.width() * z), qRound(logical.height() * z));
}

/// Rounded 75% seed used as the native unmapped default (same as Dart
/// `TrampMetrics.nativeUnmappedSeed`).
inline QSize nativeUnmappedSeed(QSize logical) {
  return zoomed(logical, kDefaultZoomPercent);
}

}  // namespace tramp
