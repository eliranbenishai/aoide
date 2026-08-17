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
};

std::array<WindowSpec, 5> windowSpecs();

/// Frameless host shell. Qt::Window only — never Tool, Dialog, or Popup.
Qt::WindowFlags hostWindowFlags();

}  // namespace tramp
