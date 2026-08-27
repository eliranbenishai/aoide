#pragma once

#include <array>

#include <QPoint>
#include <QSize>
#include <QString>
#include <Qt>

namespace aoide {

enum class WindowId { main, equalizer, playlist, settings, about, skins };

/// How many panels there are. `WindowId` is dense from zero, so this is also
/// the length of every table keyed by one — see `panel_registry.h`.
inline constexpr int kPanelCount = 6;

struct WindowSpec {
  WindowId id;
  QString title;
  QSize logicalSize;
  QSize size;
  QPoint origin;
};

std::array<WindowSpec, kPanelCount> windowSpecs();

/// Frameless host shell. Qt::Window only — never Tool, Dialog, or Popup.
Qt::WindowFlags hostWindowFlags();

}  // namespace aoide
