#include "aoide_fonts.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QString>
#include <QStringList>
#include <cmath>
#include <cstring>

namespace aoide {
namespace {

QString g_chromeFamily = QStringLiteral("TrampCondensed");
QString g_lcdFamily = QStringLiteral("TrampMono");
QString g_brandFamily = QStringLiteral("Anton");
QString g_lookChrome;
QString g_lookLcd;

}  // namespace

QString bundledDataDir(const char* leaf) {
  const QString name = QString::fromUtf8(leaf);
  QStringList roots;
  if (QCoreApplication::instance()) {
    const QString appDir = QCoreApplication::applicationDirPath();
    roots << QDir(appDir).filePath(name);
    roots << QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../share/aoide/") + name));
#ifdef Q_OS_MACOS
    // applicationDirPath() is Contents/MacOS; the bundle install puts assets/
    // and skins/ in Contents/Resources. Without this the compile-time source
    // tree wins on a developer machine and users get empty fonts and skins.
    roots << QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../Resources/") + name));
#endif
  }
#ifdef AOIDE_ASSET_DIR
  if (std::strcmp(leaf, "assets") == 0) {
    roots << QStringLiteral(AOIDE_ASSET_DIR);
  }
#endif
#ifdef AOIDE_SKINS_DIR
  if (std::strcmp(leaf, "skins") == 0) {
    roots << QStringLiteral(AOIDE_SKINS_DIR);
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

void loadAoideFonts() {
  const int condensed =
      QFontDatabase::addApplicationFont(assetPath("fonts/TrampCondensed-Bold.ttf"));
  const int mono =
      QFontDatabase::addApplicationFont(assetPath("fonts/TrampMono-Medium.ttf"));
  const int brand = QFontDatabase::addApplicationFont(assetPath("fonts/Anton-Regular.ttf"));
  const QStringList families = QFontDatabase::applicationFontFamilies(condensed);
  if (!families.isEmpty()) {
    g_chromeFamily = families.front();
  }
  const QStringList lcd = QFontDatabase::applicationFontFamilies(mono);
  if (!lcd.isEmpty()) {
    g_lcdFamily = lcd.front();
  }
  const QStringList wordmark = QFontDatabase::applicationFontFamilies(brand);
  if (!wordmark.isEmpty()) {
    g_brandFamily = wordmark.front();
  }
}

void setLookFamilies(const QString& chrome, const QString& lcd) {
  g_lookChrome = chrome;
  g_lookLcd = lcd;
}

QString lookChromeOverride() { return g_lookChrome; }
QString lookLcdOverride() { return g_lookLcd; }

QString chromeFamily() {
  return g_lookChrome.isEmpty() ? g_chromeFamily : g_lookChrome;
}

QString lcdFamily() {
  return g_lookLcd.isEmpty() ? g_lcdFamily : g_lookLcd;
}

QString brandFamily() { return g_brandFamily; }

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

void fitFontToWidth(QFont& font, const QString& text, qreal maxWidth) {
  if (text.isEmpty() || maxWidth <= 0) return;

  auto advance = [&]() { return QFontMetricsF(font).horizontalAdvance(text); };
  if (advance() <= maxWidth) return;

  if (font.letterSpacingType() == QFont::AbsoluteSpacing && font.letterSpacing() > 0) {
    const qreal hi0 = font.letterSpacing();
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    if (advance() <= maxWidth) {
      qreal lo = 0;
      qreal hi = hi0;
      for (int i = 0; i < 24; ++i) {
        const qreal mid = (lo + hi) * 0.5;
        font.setLetterSpacing(QFont::AbsoluteSpacing, mid);
        if (advance() <= maxWidth) {
          lo = mid;
        } else {
          hi = mid;
        }
      }
      font.setLetterSpacing(QFont::AbsoluteSpacing, lo);
      if (advance() <= maxWidth) return;
      font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    }
  }

  int px = font.pixelSize();
  while (px > 1 && advance() > maxWidth) {
    --px;
    font.setPixelSize(px);
  }
}

}  // namespace aoide
