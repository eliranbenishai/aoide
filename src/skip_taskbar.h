#pragma once

class QWindow;

namespace tramp {

/// Hide a toplevel from the taskbar / pager without turning it into a popup.
void applySkipTaskbar(QWindow* window);

}  // namespace tramp
