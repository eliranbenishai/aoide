#include "tramp_fonts.h"

#include <QDir>
#include <QFontDatabase>
#include <QString>

namespace tramp {
namespace {

QString g_chromeFamily = QStringLiteral("TrampCondensed");
QString g_lcdFamily = QStringLiteral("TrampMono");
QString g_lookChrome;
QString g_lookLcd;

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

void setLookFamilies(const QString& chrome, const QString& lcd) {
  g_lookChrome = chrome;
  g_lookLcd = lcd;
}

QString chromeFamily() {
  return g_lookChrome.isEmpty() ? g_chromeFamily : g_lookChrome;
}

QString lcdFamily() {
  return g_lookLcd.isEmpty() ? g_lcdFamily : g_lookLcd;
}

}  // namespace tramp
