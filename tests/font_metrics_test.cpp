#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QDir>
#include <QFont>
#include <QFontDatabase>
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

  if (gFails) {
    std::fprintf(stderr, "%d failure(s)\n", gFails);
    return 1;
  }
  return 0;
}
