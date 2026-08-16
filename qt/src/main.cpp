#include "host_window.h"
#include "tramp_fonts.h"
#include "window_spec.h"

#include <QApplication>
#include <QWindow>
#include <vector>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("Tramp"));
  app.setDesktopFileName(QStringLiteral("tramp"));
  app.setQuitOnLastWindowClosed(false);
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
