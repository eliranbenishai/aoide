#include "title_chrome.h"

#include "tramp_metrics.h"

namespace tramp {

namespace {

constexpr int kPadRight = 9;
constexpr int kBtnW = 26;
constexpr int kBtnH = 22;
constexpr int kBtnGap = 5;
constexpr int kGapBeforeButtons = 12;
constexpr int kZoomReadoutW = 44;
constexpr int kGapBeforeZoomReadout = 8;

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

  const int n = layout.showZoom ? 4 : 2;
  const int cluster = n * kBtnW + (n - 1) * kBtnGap;
  layout.buttonsLeft = logical.width() - kPadRight - cluster;
  layout.dragRight = layout.buttonsLeft - kGapBeforeButtons;

  const int btnY = (kTitleBar - kBtnH) / 2;
  if (layout.showZoom) {
    layout.zoomReadout =
        QRect(layout.buttonsLeft - kGapBeforeButtons - kZoomReadoutW, btnY, kZoomReadoutW, kBtnH);
    layout.dragRight = layout.zoomReadout.left() - kGapBeforeZoomReadout;
  }
  int x = layout.buttonsLeft;
  auto place = [&](QRect& slot) {
    slot = QRect(x, btnY, kBtnW, kBtnH);
    x += kBtnW + kBtnGap;
  };

  if (layout.showZoom) {
    place(layout.minimize);
    place(layout.zoomOut);
    place(layout.zoomIn);
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

}  // namespace tramp
