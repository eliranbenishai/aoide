#pragma once

#include <QString>
#include <QStringView>
#include <QtGlobal>

class QWindow;

namespace aoide {

/// JS KWin runs to set keepAbove on the host. xdg-shell has no keep-above;
/// Plasma's "Keep Above Others" is this property. The script matches pid,
/// caption, desktop file, or WM class `aoide`.
QString kwinKeepAboveScript(qint64 pid, QStringView caption, QStringView desktopFile, bool on);

/// Where that script is written, given [runtimeDir] ($XDG_RUNTIME_DIR). KWin
/// reads it by path from *its* process, so the file has to sit somewhere the
/// host and the app agree on. It must therefore be a **subdirectory**: a
/// Flatpak's $XDG_RUNTIME_DIR is a private mount that the host cannot see, and
/// `--filesystem=xdg-run/aoide:create` shares exactly this one directory at the
/// same absolute path on both sides. Empty when [runtimeDir] is empty.
QString kwinKeepAboveScriptPath(const QString& runtimeDir);

/// Whether [platformName] can honour keep-above. Allowlist: windows, cocoa,
/// and xcb use the Qt flag. wayland and wayland-* only when [kwinReachable].
/// Every other QPA is false — the flag can still be set, but that is not
/// compositor stacking. The OS is not consulted; the QPA is.
bool compositorKeepAboveAvailableOn(QStringView platformName, bool kwinReachable);

/// Whether this platform can actually hold the window above other apps.
/// Feeds the live QPA name and the cached KWin probe into
/// [compositorKeepAboveAvailableOn]. Builds without D-Bus cannot probe KWin,
/// so Wayland is false there. Cheap enough for a menu (the KWin probe is
/// cached).
bool compositorKeepAboveAvailable();

/// Qt flag on [window] where that flag is the stacking mechanism (not Wayland).
/// On Plasma Wayland, a one-shot KWin keepAbove write instead. Offscreen and
/// non-KDE compositors get the flag only — that is not proof the host stacked
/// above other apps.
void applyCompositorKeepAbove(QWindow* window, bool on);

/// Drop the KWin plugin and the runtime script file. The script is a one-shot
/// property write; leaving it loaded after exit left `isScriptLoaded` true with
/// a file naming a dead pid. Safe when neither exists, including builds
/// without D-Bus.
void releaseCompositorKeepAbove();

}  // namespace aoide
