#include "host_window.h"
#include "tramp_fonts.h"
#include "window_spec.h"

#include <QApplication>
#include <vector>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("tramp-qt-tracer"));
  app.setQuitOnLastWindowClosed(false);
  tramp::loadTrampFonts();

  std::vector<HostWindow*> windows;
  windows.reserve(5);
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    auto* window = new HostWindow(spec);
    windows.push_back(window);
    window->show();
  }

  const int rc = app.exec();
  for (HostWindow* window : windows) {
    delete window;
  }
  return rc;
}
