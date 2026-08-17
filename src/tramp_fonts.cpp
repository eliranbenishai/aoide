#include "tramp_fonts.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QString>
#include <QStringList>
#include <cmath>
#include <cstring>

namespace tramp {
namespace {

QString g_chromeFamily = QStringLiteral("TrampCondensed");
QString g_lcdFamily = QStringLiteral("TrampMono");
QString g_lookChrome;
QString g_lookLcd;

}  // namespace

QString bundledDataDir(const char* leaf) {
  const QString name = QString::fromUtf8(leaf);
  QStringList roots;
  if (QCoreApplication::instance()) {
    const QString appDir = QCoreApplication::applicationDirPath();
    roots << QDir(appDir).filePath(name);
    roots << QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../share/tramp/") + name));
  }
#ifdef TRAMP_ASSET_DIR
  if (std::strcmp(leaf, "assets") == 0) {
    roots << QStringLiteral(TRAMP_ASSET_DIR);
  }
#endif
#ifdef TRAMP_SKINS_DIR
  if (std::strcmp(leaf, "skins") == 0) {
    roots << QStringLiteral(TRAMP_SKINS_DIR);
  }
#endif
  for (const QString& root : roots) {
    if (QFileInfo::exists(root)) return root;
  }
  return roots.isEmpty() ? name : roots.front();
}

QString bundledSkinsDir() { return bundledDataDir("skins"); }

QString assetPath(const char* relative) {
  return QDir(bundledDataDir("assets")).filePath(QString::fromUtf8(relative));
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

int pixelSizeFittingLineHeight(QFont font, int requestedPx, qreal maxLineHeight) {
  if (requestedPx <= 0) return 1;
  if (maxLineHeight <= 0) return requestedPx;

  auto lineBottom = [](const QFont& f) {
    const QFontMetricsF metrics(f);
    const QRectF ink = metrics.tightBoundingRect(QStringLiteral("0123456789:"));
    return metrics.ascent() + qMax(ink.bottom(), qreal(0));
  };
  auto usedAt = [&](int px) {
    font.setPixelSize(px);
    return lineBottom(font);
  };

  const qreal used = usedAt(requestedPx);
  if (used <= maxLineHeight) return requestedPx;

  int px = qMax(1, int(std::floor(qreal(requestedPx) * maxLineHeight / used)));
  while (px > 1 && usedAt(px) > maxLineHeight) {
    --px;
  }
  return px;
}

}  // namespace tramp
