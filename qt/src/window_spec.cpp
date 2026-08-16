#include "window_spec.h"

#include "tramp_metrics.h"

namespace tramp {

std::array<WindowSpec, 5> windowSpecs() {
  const QSize mainPx = nativeUnmappedSeed(kMainPlayer);
  const QSize eqPx = nativeUnmappedSeed(kEqualizer);
  const QSize plPx = nativeUnmappedSeed(kPlaylistDefault);
  const QSize setPx = nativeUnmappedSeed(kSettings);
  const QSize aboutPx = nativeUnmappedSeed(kAbout);
  constexpr int gap = 16;
  constexpr int x0 = 48;
  constexpr int y0 = 48;
  const QPoint mainOrigin(x0, y0);
  const QPoint eqOrigin(x0, y0 + mainPx.height() + gap);
  const QPoint plOrigin(x0 + mainPx.width() + gap, y0);
  const QPoint setOrigin(x0 + mainPx.width() + gap, y0 + plPx.height() + gap);
  const QPoint aboutOrigin(x0, y0 + mainPx.height() + gap + eqPx.height() + gap);

  return {
      WindowSpec{
          WindowId::main,
          QStringLiteral("Tramp"),
          kMainPlayer,
          mainPx,
          mainOrigin,
          false,
      },
      WindowSpec{
          WindowId::equalizer,
          QStringLiteral("Equalizer"),
          kEqualizer,
          eqPx,
          eqOrigin,
          true,
      },
      WindowSpec{
          WindowId::playlist,
          QStringLiteral("Playlist"),
          kPlaylistDefault,
          plPx,
          plOrigin,
          true,
      },
      WindowSpec{
          WindowId::settings,
          QStringLiteral("Settings"),
          kSettings,
          setPx,
          setOrigin,
          true,
      },
      WindowSpec{
          WindowId::about,
          QStringLiteral("About"),
          kAbout,
          aboutPx,
          aboutOrigin,
          true,
      },
  };
}

Qt::WindowFlags hostWindowFlags() {
  return Qt::FramelessWindowHint | Qt::Window;
}

}  // namespace tramp
