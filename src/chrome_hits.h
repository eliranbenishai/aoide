#pragma once

#include "session_view.h"
#include "window_spec.h"

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <algorithm>
#include <optional>

namespace tramp {

struct ChromeHit {
  enum class Kind {
    none,
    options,
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
    plSort,
    plOptions,
    plPrev,
    plPlay,
    plNext,
    settingsGeneral,
    settingsSkins,
    settingsResume,
    settingsConfirm,
    settingsScroll,
    settingsMinimize,
    settingsSnapOff,
    settingsSnapNormal,
    settingsSnapStrong,
    settingsReset,
    settingsSkinRow,
    settingsInstallZip,
    settingsInstallFolder,
    settingsSkinsFolder,
    settingsResetSkinsFolder,
    aboutWeb,
    plResize,
  };

  Kind kind = Kind::none;
  int index = -1;
  QRect rect;
};

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

ChromeHit hitTest(WindowId id, QSize logical, QPoint pos, const SessionView& view);

QRect mainOptionsHit(QSize logical);
QRect mainEqHit(QSize logical);
QRect mainPlHit(QSize logical);

}  // namespace tramp
