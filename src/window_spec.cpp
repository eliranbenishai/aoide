#include "window_spec.h"

#include "panel_registry.h"
#include "tramp_metrics.h"

#include <QMap>
#include <algorithm>

namespace tramp {

std::array<WindowSpec, kPanelCount> windowSpecs() {
  // The first-run arrangement, and only that: panels stack down their column in
  // registry order, columns run left to right, and a gap separates everything.
  // Where a panel actually sits comes out of the frame the layout owns; this is
  // the seed a fresh install starts from.
  constexpr int gap = 16;
  constexpr int x0 = 48;
  constexpr int y0 = 48;

  QMap<int, int> columnWidth;
  for (const PanelSpec& panel : panelSpecs()) {
    const int width = nativeUnmappedSeed(panel.logicalSize).width();
    columnWidth[panel.seedColumn] = std::max(columnWidth.value(panel.seedColumn, 0), width);
  }
  QMap<int, int> columnLeft;
  int left = x0;
  for (auto it = columnWidth.constBegin(); it != columnWidth.constEnd(); ++it) {
    columnLeft[it.key()] = left;
    left += it.value() + gap;
  }

  QMap<int, int> columnTop;
  std::array<WindowSpec, kPanelCount> specs{};
  for (const PanelSpec& panel : panelSpecs()) {
    const QSize seed = nativeUnmappedSeed(panel.logicalSize);
    const int top = columnTop.value(panel.seedColumn, y0);
    specs[panelIndex(panel.id)] = WindowSpec{
        panel.id, panel.title, panel.logicalSize, seed,
        QPoint(columnLeft.value(panel.seedColumn, x0), top)};
    columnTop[panel.seedColumn] = top + seed.height() + gap;
  }
  return specs;
}

Qt::WindowFlags hostWindowFlags() {
  return Qt::FramelessWindowHint | Qt::Window;
}

}  // namespace tramp
