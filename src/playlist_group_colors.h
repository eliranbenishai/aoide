#pragma once

#include <QColor>

namespace aoide {

/// Group identity is its stable slot, so renaming a group never changes its
/// colour. These marks deliberately stay recognisable across skins.
inline QColor playlistGroupColor(int id) {
  static const QRgb colors[] = {0xfff65b64, 0xfff2a044, 0xffe7d14d, 0xff60c98b,
                                0xff5da9ec, 0xffb286eb, 0xff9da5b2};
  return id >= 0 && id < 7 ? QColor::fromRgb(colors[id]) : QColor();
}

}  // namespace aoide
