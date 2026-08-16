#include "tramp_fonts.h"

#include <QDir>
#include <QFontDatabase>
#include <QString>

namespace tramp {
namespace {

QString g_chromeFamily = QStringLiteral("TrampCondensed");

}  // namespace

QString assetPath(const char* relative) {
#ifdef TRAMP_ASSET_DIR
  return QDir(QStringLiteral(TRAMP_ASSET_DIR)).filePath(QString::fromUtf8(relative));
#else
  return QString::fromUtf8(relative);
#endif
}

void loadTrampFonts() {
  const int condensed =
      QFontDatabase::addApplicationFont(assetPath("fonts/TrampCondensed-Bold.ttf"));
  QFontDatabase::addApplicationFont(assetPath("fonts/TrampMono-Medium.ttf"));
  const QStringList families = QFontDatabase::applicationFontFamilies(condensed);
  if (!families.isEmpty()) {
    g_chromeFamily = families.front();
  }
}

QString chromeFamily() {
  return g_chromeFamily;
}

}  // namespace tramp
