#pragma once

#include "window_spec.h"

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

namespace tramp {

/// Logical-pixel hit map for mockup `.tbar`. Drag never includes `.wbtn`.
struct TitleChromeLayout {
  enum class Hit {
    none,
    drag,
    minimize,
    collapse,
    zoomOut,
    zoomIn,
    close,
  };

  QSize logical;
  QRect titleBar;
  QRect minimize;
  QRect zoomOut;
  QRect zoomIn;
  QRect close;
  int buttonsLeft = 0;
  int dragRight = 0;
  bool showBrand = false;
  bool showZoom = false;
  QString roleName;

  static TitleChromeLayout forWindow(WindowId id, QSize logical);

  bool inDragRegion(QPoint p) const;
  Hit hit(QPoint p) const;
};

QString roleTitle(WindowId id);

}  // namespace tramp
