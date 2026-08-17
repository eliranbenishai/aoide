#pragma once

class QWindow;

namespace tramp {

/// Hide a toplevel from the taskbar / pager without turning it into a popup.
void applySkipTaskbar(QWindow* window);

/// Parent [extra] to [main] then skip the taskbar. Wayland (KWin/GNOME) only
/// keeps extras off the taskbar when they are transients of the main window —
/// do not skip this on Linux.
void attachExtraWindow(QWindow* extra, QWindow* main);

}  // namespace tramp
