#include "chrome_paint.h"
#include "host_window.h"
#include "mockup_draw.h"
#include "title_chrome.h"
#include "tramp_fonts.h"
#include "window_spec.h"

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QWindow>
#include <vector>

namespace {

QString dumpName(tramp::WindowId id) {
  switch (id) {
    case tramp::WindowId::main:
      return QStringLiteral("main_player_window");
    case tramp::WindowId::equalizer:
      return QStringLiteral("equalizer_window");
    case tramp::WindowId::playlist:
      return QStringLiteral("playlist_window");
    case tramp::WindowId::settings:
      return QStringLiteral("settings_window");
    case tramp::WindowId::about:
      return QStringLiteral("about_window");
  }
  return QStringLiteral("window");
}

int dumpChrome(const QString& dirPath) {
  QDir().mkpath(dirPath);
  tramp::loadTrampFonts();
  QImage logo = tramp::loadTrampLogo();
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto title =
        tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);
    tramp::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo);
    p.end();
    const QString path =
        QDir(dirPath).filePath(dumpName(spec.id) + QStringLiteral(".png"));
    if (!img.save(path)) {
      return 1;
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("Tramp"));
  app.setDesktopFileName(QStringLiteral("tramp"));
  app.setQuitOnLastWindowClosed(false);

  const QStringList args = app.arguments();
  const int dumpAt = args.indexOf(QStringLiteral("--dump-chrome"));
  if (dumpAt >= 0) {
    const QString dir = dumpAt + 1 < args.size()
                            ? args.at(dumpAt + 1)
                            : QStringLiteral(".");
    return dumpChrome(dir);
  }

  tramp::loadTrampFonts();

  std::vector<HostWindow*> windows;
  windows.reserve(5);
  HostWindow* mainWindow = nullptr;
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    auto* window = new HostWindow(spec);
    windows.push_back(window);
    if (spec.id == tramp::WindowId::main) {
      mainWindow = window;
    }
  }

  // Show main first so extras can be xdg_toplevel children. Independent
  // toplevels all land on the KDE/GNOME taskbar; X11 skip atoms do not
  // exist on Wayland.
  mainWindow->show();
  QWindow* mainHandle = mainWindow->windowHandle();
  for (HostWindow* window : windows) {
    if (window == mainWindow) {
      continue;
    }
    window->winId();
    if (QWindow* native = window->windowHandle()) {
      native->setTransientParent(mainHandle);
    }
    window->show();
  }

  const int rc = app.exec();
  for (HostWindow* window : windows) {
    delete window;
  }
  return rc;
}
