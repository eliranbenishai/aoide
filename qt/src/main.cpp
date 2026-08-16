#include "chrome_paint.h"
#include "host_window.h"
#include "mockup_draw.h"
#include "title_chrome.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
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
  HostWindow* eqWindow = nullptr;
  HostWindow* plWindow = nullptr;
  HostWindow* settingsWindow = nullptr;
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    auto* window = new HostWindow(spec);
    windows.push_back(window);
    switch (spec.id) {
      case tramp::WindowId::main:
        mainWindow = window;
        break;
      case tramp::WindowId::equalizer:
        eqWindow = window;
        break;
      case tramp::WindowId::playlist:
        plWindow = window;
        break;
      case tramp::WindowId::settings:
        settingsWindow = window;
        break;
      case tramp::WindowId::about:
        break;
    }
  }

  int zoom = tramp::kDefaultZoomPercent;
  auto applyZoom = [&](int next) {
    zoom = next;
    for (HostWindow* window : windows) {
      window->setZoomPercent(zoom);
    }
  };
  auto syncToggles = [&]() {
    tramp::BodyChrome chrome;
    chrome.eqOn = eqWindow->isVisible();
    chrome.plOn = plWindow->isVisible();
    mainWindow->setBodyChrome(chrome);
  };

  QObject::connect(mainWindow, &HostWindow::zoomOutRequested, mainWindow, [&]() {
    applyZoom(tramp::prevZoomPercent(zoom));
  });
  QObject::connect(mainWindow, &HostWindow::zoomInRequested, mainWindow, [&]() {
    applyZoom(tramp::nextZoomPercent(zoom));
  });
  QObject::connect(mainWindow, &HostWindow::toggleEqualizer, mainWindow, [&]() {
    eqWindow->setVisible(!eqWindow->isVisible());
    syncToggles();
  });
  QObject::connect(mainWindow, &HostWindow::togglePlaylist, mainWindow, [&]() {
    plWindow->setVisible(!plWindow->isVisible());
    syncToggles();
  });
  QObject::connect(eqWindow, &HostWindow::extraHidden, mainWindow, [&]() { syncToggles(); });
  QObject::connect(plWindow, &HostWindow::extraHidden, mainWindow, [&]() { syncToggles(); });
  QObject::connect(mainWindow, &HostWindow::openSettings, mainWindow, [&]() {
    settingsWindow->show();
    settingsWindow->raise();
  });

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
