#include "chrome_paint.h"
#include "host_window.h"
#include "mockup_draw.h"
#include "session.h"
#include "session_view.h"
#include "skip_taskbar.h"
#include "title_chrome.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QShortcut>
#include <QTimer>
#include <QWindow>
#include <clocale>
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
  const tramp::SessionView golden = tramp::SessionView::golden();
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto title = tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);
    tramp::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo, golden);
    p.end();
    const QString path = QDir(dirPath).filePath(dumpName(spec.id) + QStringLiteral(".png"));
    if (!img.save(path)) return 1;
  }
  return 0;
}

QStringList launchFiles(const QStringList& args) {
  QStringList files;
  bool skipNext = false;
  for (int i = 1; i < args.size(); ++i) {
    if (skipNext) {
      skipNext = false;
      continue;
    }
    const QString& a = args.at(i);
    if (a == QLatin1String("--dump-chrome")) {
      skipNext = true;
      continue;
    }
    if (a.startsWith(QLatin1Char('-'))) continue;
    files << a;
  }
  return files;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  std::setlocale(LC_NUMERIC, "C");
  app.setApplicationName(QStringLiteral("Tramp"));
  app.setApplicationVersion(QStringLiteral("0.1.0"));
  app.setOrganizationName(QStringLiteral("Proxima Magnifica"));
  app.setDesktopFileName(QStringLiteral("com.tramp.tramp"));
  app.setQuitOnLastWindowClosed(false);

  const QStringList args = app.arguments();
  const int dumpAt = args.indexOf(QStringLiteral("--dump-chrome"));
  if (dumpAt >= 0) {
    const QString dir =
        dumpAt + 1 < args.size() ? args.at(dumpAt + 1) : QStringLiteral(".");
    return dumpChrome(dir);
  }

  tramp::loadTrampFonts();
  tramp::TrampSession session;

  std::vector<HostWindow*> windows;
  windows.reserve(5);
  HostWindow* mainWindow = nullptr;
  HostWindow* eqWindow = nullptr;
  HostWindow* plWindow = nullptr;
  HostWindow* settingsWindow = nullptr;
  HostWindow* aboutWindow = nullptr;
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
        aboutWindow = window;
        break;
    }
  }
  session.setWindows(mainWindow, eqWindow, plWindow, settingsWindow, aboutWindow);
  mainWindow->setQuitConfirmer([&]() {
    if (!session.confirmQuit()) return true;
    const auto answer = QMessageBox::question(mainWindow, QStringLiteral("Quit Tramp"),
                                              QStringLiteral("Quit Tramp?"));
    return answer == QMessageBox::Yes;
  });

  auto applyZoom = [&](int next) {
    session.setZoomPercent(next);
    for (HostWindow* window : windows) window->setZoomPercent(next);
  };

  auto refresh = [&]() {
    const tramp::SessionView view = session.view();
    for (HostWindow* window : windows) window->setSessionView(view);
  };

  QObject::connect(&session, &tramp::TrampSession::chromeChanged, mainWindow, refresh);
  QObject::connect(&session, &tramp::TrampSession::zoomChanged, mainWindow, [&](int z) {
    for (HostWindow* window : windows) window->setZoomPercent(z);
  });
  QObject::connect(&session, &tramp::TrampSession::requestShow, mainWindow, [&](tramp::WindowId id) {
    HostWindow* w = nullptr;
    switch (id) {
      case tramp::WindowId::equalizer:
        w = eqWindow;
        break;
      case tramp::WindowId::playlist:
        w = plWindow;
        break;
      case tramp::WindowId::settings:
        w = settingsWindow;
        break;
      case tramp::WindowId::about:
        w = aboutWindow;
        break;
      case tramp::WindowId::main:
        w = mainWindow;
        break;
    }
    if (w) {
      w->show();
      w->raise();
    }
    refresh();
  });
  QObject::connect(&session, &tramp::TrampSession::requestHide, mainWindow, [&](tramp::WindowId id) {
    if (id == tramp::WindowId::equalizer) eqWindow->hide();
    if (id == tramp::WindowId::playlist) plWindow->hide();
    if (id == tramp::WindowId::settings) settingsWindow->hide();
    if (id == tramp::WindowId::about) aboutWindow->hide();
    refresh();
  });
  QObject::connect(&session, &tramp::TrampSession::requestRaise, mainWindow, [&](tramp::WindowId id) {
    HostWindow* w = nullptr;
    switch (id) {
      case tramp::WindowId::equalizer:
        w = eqWindow;
        break;
      case tramp::WindowId::playlist:
        w = plWindow;
        break;
      case tramp::WindowId::settings:
        w = settingsWindow;
        break;
      case tramp::WindowId::about:
        w = aboutWindow;
        break;
      case tramp::WindowId::main:
        w = mainWindow;
        break;
    }
    if (w) w->raise();
  });

  for (HostWindow* window : windows) {
    QObject::connect(window, &HostWindow::zoomOutRequested, mainWindow, [&]() {
      applyZoom(tramp::prevZoomPercent(session.zoomPercent()));
    });
    QObject::connect(window, &HostWindow::zoomInRequested, mainWindow, [&]() {
      applyZoom(tramp::nextZoomPercent(session.zoomPercent()));
    });
    QObject::connect(window, &HostWindow::chromePressed, mainWindow,
                     [&, window](tramp::ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
                       session.handleHit(window->id(), hit, mods, logical);
                     });
    QObject::connect(window, &HostWindow::chromeDragged, mainWindow,
                     [&, window](tramp::ChromeHit hit, QPoint logical) {
                       session.handleDrag(window->id(), hit, logical);
                     });
    QObject::connect(window, &HostWindow::chromeReleased, mainWindow,
                     [&, window]() { session.handleRelease(window->id()); });
    QObject::connect(window, &HostWindow::wheelScrolled, mainWindow,
                     [&, window](int d) { session.handleWheel(window->id(), d); });
    QObject::connect(window, &HostWindow::nativeMoved, mainWindow,
                     [&, window](QPoint pos) { session.windowMoved(window->id(), pos, false); });
    QObject::connect(window, &HostWindow::nativeResized, mainWindow,
                     [&, window](QSize size) {
                       if (window->id() == tramp::WindowId::playlist) session.playlistResized(size);
                     });
    QObject::connect(window, &HostWindow::filesDropped, mainWindow, [&](const QStringList& paths) {
      session.applyDroppedPaths(paths, false);
    });
    QObject::connect(window, &HostWindow::extraHidden, mainWindow, [&, window]() {
      session.extraClosed(window->id());
      refresh();
    });
    QObject::connect(window, &HostWindow::shadedChanged, mainWindow,
                     [&, window](bool shaded) { session.setShaded(window->id(), shaded); });
    QObject::connect(window, &HostWindow::trackActivated, mainWindow,
                     [&](int index) { session.playTrackAt(index); });
  }

  QObject::connect(mainWindow, &HostWindow::aboutToQuit, mainWindow, [&]() { session.persistNow(); });
  QObject::connect(mainWindow, &HostWindow::mainMinimized, mainWindow,
                   [&](bool minimized) { session.mainMinimized(minimized); });
  QObject::connect(mainWindow, &HostWindow::mainActivated, mainWindow,
                   [&]() { session.mainActivated(); });

  auto addAppShortcut = [&](const QKeySequence& seq, auto fn) {
    auto* sc = new QShortcut(seq, mainWindow);
    sc->setContext(Qt::ApplicationShortcut);
    QObject::connect(sc, &QShortcut::activated, mainWindow, fn);
  };
  addAppShortcut(QKeySequence(Qt::Key_Space), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::play, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence::SelectAll, [&]() { session.selectAllTracks(); });
  addAppShortcut(QKeySequence::Delete, [&]() { session.removeSelectedTracks(); });
  addAppShortcut(QKeySequence(Qt::Key_Backspace), [&]() { session.removeSelectedTracks(); });
  addAppShortcut(QKeySequence(Qt::Key_MediaPlay), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::play, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaStop), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::stop, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaNext), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::next, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaPrevious), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::prev, -1, {}},
                      Qt::NoModifier, {});
  });

  session.bootstrap(launchFiles(args));
  applyZoom(session.zoomPercent());
  refresh();

  mainWindow->show();
  QWindow* mainHandle = mainWindow->windowHandle();
  for (HostWindow* window : windows) {
    if (window == mainWindow) continue;
    window->winId();
    if (QWindow* native = window->windowHandle()) {
      native->setTransientParent(mainHandle);
    }
  }
  if (session.view().eqOn) eqWindow->show();
  if (session.view().plOn) plWindow->show();
  if (session.windowShouldShow(tramp::WindowId::settings)) settingsWindow->show();
  if (session.windowShouldShow(tramp::WindowId::about)) aboutWindow->show();

  if (qEnvironmentVariable("TRAMP_AUTO_QUIT") == QLatin1String("1")) {
    session.persistNow();
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
  }

  const int rc = app.exec();
  session.detachWindows();
  for (HostWindow* window : windows) delete window;
  return rc;
}
