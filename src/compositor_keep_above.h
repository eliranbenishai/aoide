#pragma once

#include <QString>
#include <QStringView>
#include <QtGlobal>

class QWindow;

namespace tramp {

/// JS KWin runs to set keepAbove on the host. xdg-shell has no keep-above;
/// Plasma's "Keep Above Others" is this property. The script matches pid,
/// caption, desktop file, or WM class `tramp`.
QString kwinKeepAboveScript(qint64 pid, QStringView caption, QStringView desktopFile, bool on);

/// Qt flag on [window], then a KWin request on Plasma. Offscreen and non-KDE
/// compositors get the flag only — that is not proof the host stacked above
/// other apps.
void applyCompositorKeepAbove(QWindow* window, bool on);

}  // namespace tramp
