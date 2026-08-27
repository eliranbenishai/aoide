#include "skin_preview.h"

#include "chrome_paint.h"
#include "session_view.h"
#include "title_chrome.h"
#include "aoide_fonts.h"
#include "aoide_metrics.h"
#include "window_spec.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>

#include <exception>

namespace aoide {

bool writeSkinPreviewPng(const QString& id, const QVector<LookManifest>& installed,
                         const QString& path, QString* error) {
  auto fail = [&](const QString& message) {
    if (error) *error = message;
    return false;
  };
  LoadedSkinFonts fonts;
  const QString prevChrome = lookChromeOverride();
  const QString prevLcd = lookLcdOverride();
  auto restore = [&]() {
    setLookFamilies(prevChrome, prevLcd);
    fonts.unload();
  };
  try {
    ResolvedLook resolved = resolveLook(id, installed);
    fonts = loadSkinFonts(id, installed);
    resolved.chromeFamily = fonts.chromeFamily;
    resolved.lcdFamily = fonts.lcdFamily;
    const ChromeTokens tokens = ChromeTokens::from(resolved);
    setLookFamilies(resolved.chromeFamily, resolved.lcdFamily);

    static const QImage logo = loadAoideLogo();
    SessionView view = goldenDemoView();
    view.look = tokens;
    view.zoomPercent = 100;
    view.zoomInEnabled = true;
    view.zoomOutEnabled = true;

    QImage img(kMainPlayer, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    {
      QPainter p(&img);
      p.setRenderHint(QPainter::Antialiasing);
      p.setRenderHint(QPainter::TextAntialiasing);
      const auto title = TitleChromeLayout::forWindow(WindowId::main, kMainPlayer);
      LookPaintScope scope(tokens);
      paintMockupWindow(p, kMainPlayer, WindowId::main, title, &logo, view);
    }
    restore();

    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!img.save(path)) {
      return fail(QStringLiteral("failed to write %1").arg(path));
    }
    return true;
  } catch (const LookError& e) {
    restore();
    return fail(e.message());
  } catch (const std::exception& e) {
    restore();
    return fail(QString::fromUtf8(e.what()));
  }
}

}  // namespace aoide
