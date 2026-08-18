#include "app_icon.h"

#include "tramp_fonts.h"

#include <QPixmap>

namespace tramp {

QIcon appIcon() {
  const QPixmap source(assetPath("branding/app_icon.png"));
  if (source.isNull()) return {};
  QIcon icon;
  icon.addPixmap(source);
  for (int side : {16, 24, 32, 48, 64, 128, 256}) {
    if (source.width() == side && source.height() == side) continue;
    icon.addPixmap(source.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  return icon;
}

}  // namespace tramp
