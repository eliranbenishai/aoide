#pragma once

#include <array>

#include <QPoint>
#include <QSize>
#include <QString>
#include <Qt>

namespace tramp {

enum class WindowId { main, equalizer, playlist, settings, about };

struct WindowSpec {
  WindowId id;
  QString title;
  QSize logicalSize;
  QSize size;
  QPoint origin;
  bool skipTaskbar;
};

std::array<WindowSpec, 5> windowSpecs();

/// Frameless toplevel. Extras are [Qt::Dialog] (not [Qt::Tool] — Tool includes
/// the Popup bit and becomes an xdg_popup on Wayland). Never OR Tool with Window.
Qt::WindowFlags hostWindowFlags(bool skipTaskbar);

}  // namespace tramp
