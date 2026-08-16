#pragma once

#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QSize>

namespace tramp {

void paintWindowBody(QPainter& painter, WindowId id, QSize logical,
                     const QImage* logo = nullptr);

}  // namespace tramp
