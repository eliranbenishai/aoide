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

/// Where that script is written, given [runtimeDir] ($XDG_RUNTIME_DIR). KWin
/// reads it by path from *its* process, so the file has to sit somewhere the
/// host and the app agree on. It must therefore be a **subdirectory**: a
/// Flatpak's $XDG_RUNTIME_DIR is a private mount that the host cannot see, and
/// `--filesystem=xdg-run/tramp:create` shares exactly this one directory at the
/// same absolute path on both sides. Empty when [runtimeDir] is empty.
QString kwinKeepAboveScriptPath(const QString& runtimeDir);

/// Qt flag on [window], then a KWin request on Plasma. Offscreen and non-KDE
/// compositors get the flag only — that is not proof the host stacked above
/// other apps.
void applyCompositorKeepAbove(QWindow* window, bool on);

}  // namespace tramp
