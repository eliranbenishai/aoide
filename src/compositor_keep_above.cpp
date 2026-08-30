#include "compositor_keep_above.h"

#include <QDir>
#include <QGuiApplication>
#include <QWindow>

#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QStandardPaths>
#endif

namespace aoide {
namespace {

bool platformIsWayland() {
  const QString name = QGuiApplication::platformName();
  return name == QLatin1String("wayland") || name.startsWith(QLatin1String("wayland-"));
}

QString jsString(QStringView text) {
  QString out;
  out.reserve(static_cast<int>(text.size()) + 2);
  out += QLatin1Char('"');
  for (QChar c : text) {
    if (c == QLatin1Char('\\') || c == QLatin1Char('"')) out += QLatin1Char('\\');
    out += c;
  }
  out += QLatin1Char('"');
  return out;
}

#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)

const QString kKwinKeepAbovePlugin = QStringLiteral("aoide-keep-above");

// Set on aboutToQuit so a pending 0ms/150ms retry cannot reload the plugin
// after we have already unloaded it on the way out.
bool g_keepAboveReleased = false;

bool kwinServiceRegistered(const QDBusConnection& bus) {
  QDBusConnectionInterface* iface = bus.interface();
  return iface && iface->isServiceRegistered(QStringLiteral("org.kde.KWin"));
}

void unloadKeepAbovePlugin(const QDBusConnection& bus) {
  QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                           QStringLiteral("org.kde.kwin.Scripting"), bus);
  if (scripting.isValid()) {
    scripting.call(QStringLiteral("unloadScript"), kKwinKeepAbovePlugin);
  }
}

void removeKeepAboveScriptFile() {
  const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  const QString path = kwinKeepAboveScriptPath(runtime);
  if (!path.isEmpty()) QFile::remove(path);
}

void hookReleaseOnQuit() {
  static bool hooked = false;
  if (hooked) return;
  hooked = true;
  if (QCoreApplication* app = QCoreApplication::instance()) {
    QObject::connect(app, &QCoreApplication::aboutToQuit, app, []() {
      releaseCompositorKeepAbove();
    });
  }
}

void applyKWinKeepAbove(QWindow* window, bool on) {
  hookReleaseOnQuit();
  if (g_keepAboveReleased) return;
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.isConnected() || !kwinServiceRegistered(bus)) return;

  const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  const QString path = kwinKeepAboveScriptPath(runtime);
  if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).path())) return;
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
  const QString caption =
      window && !window->title().isEmpty() ? window->title() : QStringLiteral("Aoide");
  const QString desktop = QGuiApplication::desktopFileName();
  file.write(kwinKeepAboveScript(QCoreApplication::applicationPid(), caption, desktop, on).toUtf8());
  file.close();

  QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                           QStringLiteral("org.kde.kwin.Scripting"), bus);
  if (!scripting.isValid()) {
    QFile::remove(path);
    return;
  }
  scripting.call(QStringLiteral("unloadScript"), kKwinKeepAbovePlugin);
  const QDBusReply<int> id =
      scripting.call(QStringLiteral("loadScript"), path, kKwinKeepAbovePlugin);
  if (!id.isValid() || id.value() < 0) {
    QFile::remove(path);
    return;
  }
  QDBusInterface script(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/Scripting/Script%1").arg(id.value()),
                        QStringLiteral("org.kde.kwin.Script"), bus);
  if (script.isValid()) {
    // loadScript above allocates an id without opening the file, so it returns
    // success for a path KWin cannot read. This reply is the only place that
    // surfaces, and discarding it is what let the Flatpak write to a private
    // runtime dir and believe the window had been raised.
    const QDBusMessage ran = script.call(QStringLiteral("run"));
    if (ran.type() == QDBusMessage::ErrorMessage) {
      qWarning("keep-above: KWin refused %s: %s", qUtf8Printable(path),
               qUtf8Printable(ran.errorMessage()));
    }
  } else {
    scripting.call(QStringLiteral("start"));
  }
  // One-shot property write. A resident plugin is what leaked
  // isScriptLoaded=true after Aoide exited, with keep-above.js still naming
  // the dead pid. `want` is already applied (including false) before this
  // unload. The 0ms/150ms retries reload from scratch because a new
  // xdg_toplevel is only matchable after map — they are not a reason to
  // leave the plugin loaded between shots.
  scripting.call(QStringLiteral("unloadScript"), kKwinKeepAbovePlugin);
  QFile::remove(path);
}

#endif

}  // namespace

QString kwinKeepAboveScriptPath(const QString& runtimeDir) {
  if (runtimeDir.isEmpty()) return {};
  // The subdirectory is the whole point, not tidiness. KWin opens this path in
  // its own process; a Flatpak's $XDG_RUNTIME_DIR is a private mount, so a file
  // written at its root is invisible to the host and loadScript fails on a path
  // that looks right. `--filesystem=xdg-run/aoide:create` shares this directory
  // at one absolute path on both sides, so the file KWin reads is the file the
  // app just wrote.
  return QDir(runtimeDir).filePath(QStringLiteral("aoide/keep-above.js"));
}

QString kwinKeepAboveScript(qint64 pid, QStringView caption, QStringView desktopFile, bool on) {
  return QStringLiteral(
             "function list() {\n"
             "  if (workspace.windowList) return workspace.windowList();\n"
             "  if (workspace.stackingOrder) return workspace.stackingOrder;\n"
             "  if (workspace.clientList) return workspace.clientList();\n"
             "  return [];\n"
             "}\n"
             "const pid = %1;\n"
             "const caption = %2;\n"
             "const desktop = %3;\n"
             "const want = %4;\n"
             "const wins = list();\n"
             "for (let i = 0; i < wins.length; ++i) {\n"
             "  const w = wins[i];\n"
             "  const desk = String(w.desktopFileName || \"\");\n"
             "  const cap = String(w.caption || \"\");\n"
             "  const cls = String(w.resourceClass || \"\").toLowerCase();\n"
             "  if (w.pid === pid || cap === caption || desk === desktop || cls === \"aoide\") {\n"
             "    w.keepAbove = want;\n"
             "  }\n"
             "}\n")
      .arg(pid)
      .arg(jsString(caption), jsString(desktopFile), on ? QStringLiteral("true") : QStringLiteral("false"));
}

bool compositorKeepAboveAvailableOn(QStringView platformName, bool kwinReachable) {
  if (platformName == QLatin1String("windows") || platformName == QLatin1String("cocoa") ||
      platformName == QLatin1String("xcb")) {
    return true;
  }
  if (platformName == QLatin1String("wayland") ||
      platformName.startsWith(QLatin1String("wayland-"))) {
    return kwinReachable;
  }
  return false;
}

bool compositorKeepAboveAvailable() {
  bool kwin = false;
#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
  if (platformIsWayland()) {
    static const bool cached = []() {
      const QDBusConnection bus = QDBusConnection::sessionBus();
      return bus.isConnected() && kwinServiceRegistered(bus);
    }();
    kwin = cached;
  }
#else
  // Test binaries (and any build without QtDBus) cannot talk to KWin.
  // On Wayland that is the only mechanism, so the row must hide.
#endif
  return compositorKeepAboveAvailableOn(QGuiApplication::platformName(), kwin);
}

void releaseCompositorKeepAbove() {
#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
  g_keepAboveReleased = true;
  const QDBusConnection bus = QDBusConnection::sessionBus();
  if (bus.isConnected()) unloadKeepAbovePlugin(bus);
  removeKeepAboveScriptFile();
#endif
}

void applyCompositorKeepAbove(QWindow* window, bool on) {
  // setFlag goes through QWindow::setParent and remaps the native window.
  // On Wayland that recreates a virtual-desktop-sized punched toplevel;
  // xdg-shell also has no keep-above, so the flag cannot stack us. KWin's
  // keepAbove write below is the whole mechanism there.
  if (window && !platformIsWayland()) {
    window->setFlag(Qt::WindowStaysOnTopHint, on);
    // offscreen (and other non-stacking QPAs) warn and do nothing on raise.
    if (on && compositorKeepAboveAvailable()) window->raise();
  }
#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
  applyKWinKeepAbove(window, on);
#endif
}

}  // namespace aoide
