#pragma once

#include "window_spec.h"

#include <QPainter>
#include <QRectF>
#include <QSize>

namespace tramp {

void paintWindowBody(QPainter& painter, WindowId id, QSize logical);

}  // namespace tramp
