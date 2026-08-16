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

/// Frameless toplevel. Do not OR Qt::Tool with Qt::Window — that is not a
/// valid window type, and extras stay on the taskbar.
Qt::WindowFlags hostWindowFlags();

}  // namespace tramp
