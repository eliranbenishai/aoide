#pragma once

#include "title_chrome.h"

#include <QImage>
#include <QPainter>
#include <QRect>
#include <QSize>

namespace tramp {

void paintMockupWindow(QPainter& painter,
                       QSize logical,
                       const TitleChromeLayout& title,
                       const QImage* logo);

}  // namespace tramp
