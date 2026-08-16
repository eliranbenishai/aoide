#include "tramp_fonts.h"

#include <QDir>
#include <QFontDatabase>
#include <QString>

namespace tramp {
namespace {

QString g_chromeFamily = QStringLiteral("TrampCondensed");
QString g_lcdFamily = QStringLiteral("TrampMono");

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
  const int mono =
      QFontDatabase::addApplicationFont(assetPath("fonts/TrampMono-Medium.ttf"));
  const QStringList families = QFontDatabase::applicationFontFamilies(condensed);
  if (!families.isEmpty()) {
    g_chromeFamily = families.front();
  }
  const QStringList lcd = QFontDatabase::applicationFontFamilies(mono);
  if (!lcd.isEmpty()) {
    g_lcdFamily = lcd.front();
  }
}

QString chromeFamily() {
  return g_chromeFamily;
}

QString lcdFamily() {
  return g_lcdFamily;
}

}  // namespace tramp
