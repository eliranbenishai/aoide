#include "title_chrome.h"

#include "aoide_metrics.h"

namespace aoide {

namespace {

constexpr int kPadRight = 9;
constexpr int kBtnW = 26;
constexpr int kBtnH = 22;
constexpr int kBtnGap = 5;
constexpr int kGapBeforeButtons = 12;
constexpr int kGapZoomToWindow = 12;
constexpr int kZoomReadoutW = 44;

}  // namespace

QString roleTitle(WindowId id) {
  switch (id) {
    case WindowId::main:
      return QStringLiteral("Main Player");
    case WindowId::equalizer:
      return QStringLiteral("Equalizer");
    case WindowId::playlist:
      return QStringLiteral("Playlist Manager");
    case WindowId::settings:
      return QStringLiteral("Settings");
    case WindowId::about:
      return QStringLiteral("About");
    case WindowId::skins:
      return QStringLiteral("Skins");
  }
  return {};
}

TitleChromeLayout TitleChromeLayout::forWindow(WindowId id, QSize logical) {
  TitleChromeLayout layout;
  layout.logical = logical;
  layout.showBrand = id == WindowId::main;
  layout.showZoom = id == WindowId::main;
  layout.roleName = roleTitle(id);
  layout.titleBar = QRect(0, 0, logical.width(), kTitleBar);

  const int windowPair = 2 * kBtnW + kBtnGap;
  const int zoomCluster =
      layout.showZoom ? (2 * kBtnW + 2 * kBtnGap + kZoomReadoutW + kGapZoomToWindow) : 0;
  const int cluster = zoomCluster + windowPair;
  layout.buttonsLeft = logical.width() - kPadRight - cluster;
  layout.dragRight = layout.buttonsLeft - kGapBeforeButtons;

  const int btnY = (kTitleBar - kBtnH) / 2;
  int x = layout.buttonsLeft;
  auto place = [&](QRect& slot) {
    slot = QRect(x, btnY, kBtnW, kBtnH);
    x += kBtnW + kBtnGap;
  };

  if (layout.showZoom) {
    place(layout.zoomOut);
    layout.zoomReadout = QRect(x, btnY, kZoomReadoutW, kBtnH);
    x += kZoomReadoutW + kBtnGap;
    place(layout.zoomIn);
    x = layout.zoomIn.x() + kBtnW + kGapZoomToWindow;
    place(layout.minimize);
    place(layout.close);
  } else {
    place(layout.minimize);
    place(layout.close);
  }
  return layout;
}

bool TitleChromeLayout::inDragRegion(QPoint p) const {
  return titleBar.contains(p) && p.x() >= 0 && p.x() < dragRight;
}

TitleChromeLayout::Hit TitleChromeLayout::hit(QPoint p) const {
  if (close.contains(p)) {
    return Hit::close;
  }
  if (showZoom) {
    if (zoomIn.contains(p)) {
      return Hit::zoomIn;
    }
    if (zoomOut.contains(p)) {
      return Hit::zoomOut;
    }
    if (minimize.contains(p)) {
      return Hit::minimize;
    }
  } else if (minimize.contains(p)) {
    return Hit::collapse;
  }
  if (inDragRegion(p)) {
    return Hit::drag;
  }
  return Hit::none;
}

}  // namespace aoide
