#pragma once

#include "session_view.h"
#include "window_spec.h"

#include <QRect>
#include <QSize>
#include <QString>
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

ChromeHit hitTest(WindowId id, QSize logical, QPoint pos, const SessionView& view);

QRect mainOptionsHit(QSize logical);
QRect mainEqHit(QSize logical);
QRect mainPlHit(QSize logical);

}  // namespace tramp
