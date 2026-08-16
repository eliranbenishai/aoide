#pragma once

#include <QSize>
#include <QtGlobal>

namespace tramp {

inline constexpr int kDefaultZoomPercent = 75;

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
