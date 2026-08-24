#include "chrome_layout.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QString>
#include <cstdio>

namespace {

int gFails = 0;

void require(bool cond, const char* file, int line, const char* expr) {
  if (!cond) {
    std::fprintf(stderr, "FAIL %s:%d %s\n", file, line, expr);
    ++gFails;
  }
}

#define REQUIRE(cond) require(bool(cond), __FILE__, __LINE__, #cond)

QString loadFamily(const QString& path) {
  const int id = QFontDatabase::addApplicationFont(path);
  const QStringList families = QFontDatabase::applicationFontFamilies(id);
  REQUIRE(!families.isEmpty());
  return families.isEmpty() ? QString() : families.front();
}

QFont lcdProbe(const QString& family, int px) {
  QFont f(family);
  f.setPixelSize(px);
  f.setWeight(QFont::Medium);
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  return f;
}

QFont chromeProbe(const QString& family, int px, qreal trackingEm) {
  QFont f(family);
  f.setPixelSize(px);
  f.setWeight(QFont::Bold);
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  return f;
}

int headingInkPixels(const QFont& font, const QString& text, qreal boxW, bool clip) {
  QImage img(220, 40, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);
  QPainter p(&img);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setFont(font);
  p.setPen(Qt::white);
  int flags = Qt::AlignHCenter | Qt::AlignVCenter;
  if (!clip) flags |= Qt::TextDontClip;
  p.drawText(QRectF(10, 4, boxW, 20), flags, text);
  p.end();
  int n = 0;
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (qAlpha(img.pixel(x, y)) > 10) ++n;
    }
  }
  return n;
}

int lastInkRow(const QFont& font, qreal boxH, bool clip) {
  const QString sample = QStringLiteral("2:41");
  QImage img(220, 90, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);
  QPainter p(&img);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setFont(font);
  p.setPen(Qt::white);
  int flags = Qt::AlignLeft | Qt::AlignTop;
  if (!clip) flags |= Qt::TextDontClip;
  p.drawText(QRectF(0, 0, 220, clip ? boxH : 90), flags, sample);
  p.end();
  int last = -1;
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (qAlpha(img.pixel(x, y)) > 10) {
        last = y;
        break;
      }
    }
  }
  return last;
}

QString skinsFile(const char* relative) {
#ifdef TRAMP_SKINS_DIR
  return QDir(QStringLiteral(TRAMP_SKINS_DIR)).filePath(QString::fromUtf8(relative));
#else
  return QString::fromUtf8(relative);
#endif
}

}  // namespace

int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);

  const QString spaceMono = loadFamily(skinsFile("gamma/fonts/lcd.ttf"));
  const QString trampMono = loadFamily(tramp::assetPath("fonts/TrampMono-Medium.ttf"));
  REQUIRE(!spaceMono.isEmpty());
  REQUIRE(!trampMono.isEmpty());

  {
    const QFont unfitted = lcdProbe(spaceMono, tramp::kElapsedTimePx);
    const int clipped = lastInkRow(unfitted, tramp::kElapsedTimeBoxH, true);
    const int unclipped = lastInkRow(unfitted, tramp::kElapsedTimeBoxH, false);
    REQUIRE(unclipped > clipped);
  }

  {
    const QFont probe = lcdProbe(spaceMono, tramp::kElapsedTimePx);
    const int px = tramp::pixelSizeFittingLineHeight(probe, tramp::kElapsedTimePx,
                                                     tramp::kElapsedTimeBoxH);
    const QFont fitted = lcdProbe(spaceMono, px);
    const int clipped = lastInkRow(fitted, tramp::kElapsedTimeBoxH, true);
    const int unclipped = lastInkRow(fitted, tramp::kElapsedTimeBoxH, false);
    REQUIRE(clipped == unclipped);
    REQUIRE(unclipped < tramp::kElapsedTimeBoxH);
    REQUIRE(px <= tramp::kElapsedTimePx);
    REQUIRE(px < tramp::kElapsedTimePx);
    const qreal inkH =
        QFontMetricsF(fitted).tightBoundingRect(QStringLiteral("2:41")).height();
    REQUIRE(inkH >= 30);
  }

  {
    const QFont probe = lcdProbe(trampMono, tramp::kElapsedTimePx);
    const int px = tramp::pixelSizeFittingLineHeight(probe, tramp::kElapsedTimePx,
                                                     tramp::kElapsedTimeBoxH);
    REQUIRE(px == tramp::kElapsedTimePx);
  }

  {
    const QString heading = QStringLiteral("NO SAVED PLAYLISTS");
    const qreal boxW = tramp::playlistEmptyWellTextWidth(tramp::kPlaylistCollectionMinWidth);
    REQUIRE(boxW > 0);

    auto checkFace = [&](const QString& family) {
      const QFont designed = chromeProbe(family, 12, 0.18);
      REQUIRE(QFontMetricsF(designed).horizontalAdvance(heading) > boxW);
      const int clipped = headingInkPixels(designed, heading, boxW, true);
      const int unclipped = headingInkPixels(designed, heading, boxW, false);
      REQUIRE(unclipped > clipped);

      QFont fitted = designed;
      tramp::fitFontToWidth(fitted, heading, boxW);
      REQUIRE(QFontMetricsF(fitted).horizontalAdvance(heading) <= boxW);
      REQUIRE(headingInkPixels(fitted, heading, boxW, true) ==
              headingInkPixels(fitted, heading, boxW, false));
      REQUIRE(fitted.pixelSize() >= 8);
    };

    checkFace(loadFamily(skinsFile("shield/fonts/chrome.ttf")));
    checkFace(loadFamily(skinsFile("thunder/fonts/chrome.ttf")));
  }

  {
    REQUIRE(QFileInfo::exists(tramp::assetPath("fonts/Anton-Regular.ttf")));
    tramp::loadTrampFonts();
    const QString brand = tramp::brandFamily();
    REQUIRE(brand == QStringLiteral("Anton"));
    REQUIRE(QFontInfo(QFont(brand)).family() == QStringLiteral("Anton"));
    const QString chrome = tramp::chromeFamily();
    tramp::setLookFamilies(tramp::lcdFamily(), tramp::lcdFamily());
    REQUIRE(tramp::chromeFamily() == tramp::lcdFamily());
    REQUIRE(tramp::brandFamily() == brand);
    tramp::setLookFamilies({}, {});
    REQUIRE(tramp::chromeFamily() == chrome);
  }

  if (gFails) {
    std::fprintf(stderr, "%d failure(s)\n", gFails);
    return 1;
  }
  return 0;
}
