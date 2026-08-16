#include "window_spec.h"

namespace tramp {

std::array<WindowSpec, 5> windowSpecs() {
  return {
      WindowSpec{
          WindowId::main,
          QStringLiteral("Tramp"),
          QSize(400, 180),
          QPoint(80, 80),
          QColor(26, 26, 26, 230),
          false,
      },
      WindowSpec{
          WindowId::equalizer,
          QStringLiteral("Equalizer"),
          QSize(400, 140),
          QPoint(80, 280),
          QColor(32, 48, 40, 230),
          true,
      },
      WindowSpec{
          WindowId::playlist,
          QStringLiteral("Playlist"),
          QSize(420, 280),
          QPoint(500, 80),
          QColor(40, 36, 28, 230),
          true,
      },
      WindowSpec{
          WindowId::settings,
          QStringLiteral("Settings"),
          QSize(360, 240),
          QPoint(500, 380),
          QColor(36, 32, 48, 230),
          true,
      },
      WindowSpec{
          WindowId::about,
          QStringLiteral("About"),
          QSize(320, 200),
          QPoint(880, 80),
          QColor(28, 36, 48, 230),
          true,
      },
  };
}

}  // namespace tramp
