#pragma once

#include <array>

#include <QColor>
#include <QPoint>
#include <QSize>
#include <QString>

namespace tramp {

enum class WindowId { main, equalizer, playlist, settings, about };

struct WindowSpec {
  WindowId id;
  QString title;
  QSize size;
  QPoint origin;
  QColor panel;
  bool skipTaskbar;
};

std::array<WindowSpec, 5> windowSpecs();

}  // namespace tramp
