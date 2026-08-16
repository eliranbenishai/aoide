#pragma once

#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSize>

namespace tramp {

struct BodyChrome {
  bool eqOn = true;
  bool plOn = true;
};

void paintWindowBody(QPainter& painter, WindowId id, QSize logical,
                     const QImage* logo = nullptr,
                     const BodyChrome& chrome = {});

QRect mainOptionsHit(QSize logical);
QRect mainEqHit(QSize logical);
QRect mainPlHit(QSize logical);

}  // namespace tramp
