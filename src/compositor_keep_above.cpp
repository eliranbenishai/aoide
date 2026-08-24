#include "compositor_keep_above.h"

#include <QDir>
#include <QGuiApplication>
#include <QWindow>

#if defined(Q_OS_LINUX) && defined(TRAMP_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#endif

namespace tramp {
namespace {

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

#if defined(Q_OS_LINUX) && defined(TRAMP_HAVE_DBUS)

void applyKWinKeepAbove(QWindow* window, bool on) {
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.isConnected()) return;
  QDBusConnectionInterface* iface = bus.interface();
  if (!iface || !iface->isServiceRegistered(QStringLiteral("org.kde.KWin"))) return;

  const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  const QString path = kwinKeepAboveScriptPath(runtime);
  if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).path())) return;
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
  const QString caption = window && !window->title().isEmpty() ? window->title() : QStringLiteral("Tramp");
  const QString desktop = QGuiApplication::desktopFileName();
  file.write(kwinKeepAboveScript(QCoreApplication::applicationPid(), caption, desktop, on).toUtf8());
  file.close();

  QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                           QStringLiteral("org.kde.kwin.Scripting"), bus);
  if (!scripting.isValid()) return;
  scripting.call(QStringLiteral("unloadScript"), QStringLiteral("tramp-keep-above"));
  const QDBusReply<int> id =
      scripting.call(QStringLiteral("loadScript"), path, QStringLiteral("tramp-keep-above"));
  if (!id.isValid() || id.value() < 0) return;
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
}

#endif

}  // namespace

QString kwinKeepAboveScriptPath(const QString& runtimeDir) {
  if (runtimeDir.isEmpty()) return {};
  // The subdirectory is the whole point, not tidiness. KWin opens this path in
  // its own process; a Flatpak's $XDG_RUNTIME_DIR is a private mount, so a file
  // written at its root is invisible to the host and loadScript fails on a path
  // that looks right. `--filesystem=xdg-run/tramp:create` shares this directory
  // at one absolute path on both sides, so the file KWin reads is the file the
  // app just wrote.
  return QDir(runtimeDir).filePath(QStringLiteral("tramp/keep-above.js"));
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
             "  if (w.pid === pid || cap === caption || desk === desktop || cls === \"tramp\") {\n"
             "    w.keepAbove = want;\n"
             "  }\n"
             "}\n")
      .arg(pid)
      .arg(jsString(caption), jsString(desktopFile), on ? QStringLiteral("true") : QStringLiteral("false"));
}

void applyCompositorKeepAbove(QWindow* window, bool on) {
  if (window) {
    window->setFlag(Qt::WindowStaysOnTopHint, on);
    if (on) window->raise();
  }
#if defined(Q_OS_LINUX) && defined(TRAMP_HAVE_DBUS)
  applyKWinKeepAbove(window, on);
#endif
}

}  // namespace tramp
