#include "native_file_dialog.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QWindow>

#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
#include "native_file_dialog_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#endif

namespace aoide {
namespace {

enum class ChooserStatus { picked, cancelled, unavailable };

struct ChooserResult {
  ChooserStatus status = ChooserStatus::unavailable;
  QStringList paths;
};

ChooserResult widgetPick(const FilePick& pick) {
  QStringList paths;
#if defined(Q_OS_LINUX)
  // Native QFileDialog on Wayland re-enters the portal and, parented to a
  // punched host child, often greys the app without mapping a window.
  QFileDialog dialog(nullptr, pick.title, pick.directory, pick.filter);
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
  dialog.setWindowModality(Qt::WindowModal);
  if (pick.parent) {
    dialog.move(pick.parent->mapToGlobal(QPoint(24, 24)));
  }
  switch (pick.kind) {
    case FilePickKind::openFiles:
      dialog.setFileMode(QFileDialog::ExistingFiles);
      break;
    case FilePickKind::openFile:
      dialog.setFileMode(QFileDialog::ExistingFile);
      break;
    case FilePickKind::saveFile:
      dialog.setAcceptMode(QFileDialog::AcceptSave);
      dialog.setFileMode(QFileDialog::AnyFile);
      if (!pick.suggestedName.isEmpty()) dialog.selectFile(pick.suggestedName);
      break;
    case FilePickKind::openDirectory:
      dialog.setFileMode(QFileDialog::Directory);
      dialog.setOption(QFileDialog::ShowDirsOnly, true);
      break;
  }
  if (dialog.exec() == QDialog::Accepted) paths = dialog.selectedFiles();
#else
  switch (pick.kind) {
    case FilePickKind::openFiles:
      paths = QFileDialog::getOpenFileNames(pick.parent, pick.title, pick.directory, pick.filter);
      break;
    case FilePickKind::openFile: {
      const QString path =
          QFileDialog::getOpenFileName(pick.parent, pick.title, pick.directory, pick.filter);
      if (!path.isEmpty()) paths = {path};
      break;
    }
    case FilePickKind::saveFile: {
      const QString start = pick.directory.isEmpty()
                                ? pick.suggestedName
                                : QDir(pick.directory).filePath(pick.suggestedName);
      const QString path =
          QFileDialog::getSaveFileName(pick.parent, pick.title, start, pick.filter);
      if (!path.isEmpty()) paths = {path};
      break;
    }
    case FilePickKind::openDirectory: {
      const QString path =
          QFileDialog::getExistingDirectory(pick.parent, pick.title, pick.directory);
      if (!path.isEmpty()) paths = {path};
      break;
    }
  }
#endif
  ChooserResult out;
  out.status = paths.isEmpty() ? ChooserStatus::cancelled : ChooserStatus::picked;
  out.paths = paths;
  return out;
}

#if defined(Q_OS_LINUX)

QString kdialogFilter(const QString& filter) {
  const auto groups = parseQtFileFilter(filter);
  if (groups.isEmpty()) return {};
  QStringList parts;
  for (const FileFilterGroup& group : groups) {
    if (group.globs.isEmpty()) {
      parts.push_back(group.name);
    } else {
      parts.push_back(group.globs.join(QLatin1Char(' ')) + QLatin1Char('|') + group.name);
    }
  }
  return parts.join(QLatin1Char('\n'));
}

ChooserResult kdialogPick(const FilePick& pick) {
  const QString exe = QStandardPaths::findExecutable(QStringLiteral("kdialog"));
  if (exe.isEmpty()) return {};
  QStringList args;
  switch (pick.kind) {
    case FilePickKind::openFiles:
      args << QStringLiteral("--getopenfilename") << QStringLiteral("--multiple")
           << QStringLiteral("--separate-output");
      break;
    case FilePickKind::openFile:
      args << QStringLiteral("--getopenfilename");
      break;
    case FilePickKind::saveFile:
      args << QStringLiteral("--getsavefilename");
      break;
    case FilePickKind::openDirectory:
      args << QStringLiteral("--getexistingdirectory");
      break;
  }
  QString start = pick.directory;
  if (start.isEmpty()) start = QDir::homePath();
  if (pick.kind == FilePickKind::saveFile && !pick.suggestedName.isEmpty()) {
    start = QDir(start).filePath(pick.suggestedName);
  }
  args << start;
  if (pick.kind != FilePickKind::openDirectory) {
    const QString filter = kdialogFilter(pick.filter);
    if (!filter.isEmpty()) args << filter;
  }
  if (!pick.title.isEmpty()) args << QStringLiteral("--title") << pick.title;

  QProcess proc;
  QEventLoop loop;
  QObject::connect(&proc, &QProcess::finished, &loop, &QEventLoop::quit);
  proc.start(exe, args);
  if (!proc.waitForStarted(3000)) return {};
  loop.exec();
  if (proc.exitStatus() != QProcess::NormalExit) return {};
  if (proc.exitCode() != 0) {
    ChooserResult cancelled;
    cancelled.status = ChooserStatus::cancelled;
    return cancelled;
  }
  const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
  if (out.isEmpty()) {
    ChooserResult cancelled;
    cancelled.status = ChooserStatus::cancelled;
    return cancelled;
  }
  ChooserResult ok;
  ok.status = ChooserStatus::picked;
  ok.paths = pick.kind == FilePickKind::openFiles
                 ? out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)
                 : QStringList{out};
  return ok;
}

#if defined(AOIDE_HAVE_DBUS)

QString portalParentWindow(QWidget* parent) {
  if (!parent) return {};
  QWidget* top = parent->window();
  if (!top) return {};
  if (!top->windowHandle()) top->winId();
  QWindow* win = top->windowHandle();
  if (!win) return {};
  if (QGuiApplication::platformName() == QLatin1String("xcb")) {
    return QStringLiteral("x11:%1").arg(static_cast<qulonglong>(win->winId()), 0, 16);
  }
  return {};
}

ChooserResult portalPick(const FilePick& pick) {
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.isConnected()) return {};

  const QString token =
      QStringLiteral("aoide%1").arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));
  QString sender = bus.baseService();
  if (sender.startsWith(QLatin1Char(':'))) sender.remove(0, 1);
  sender.replace(QLatin1Char('.'), QLatin1Char('_'));
  const QString requestPath =
      QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);

  PortalWaiter waiter;
  if (!bus.connect(QStringLiteral("org.freedesktop.portal.Desktop"), requestPath,
                   QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
                   &waiter, SLOT(onResponse(uint, QVariantMap)))) {
    return {};
  }

  const PortalFileChooserRequest req = portalFileChooserRequest(pick.kind);
  const QString parentWindow = portalParentWindow(pick.parent);
  QVariantMap options;
  options.insert(QStringLiteral("handle_token"), token);
  // Modal + empty parent on Wayland greys the host even when no picker maps.
  options.insert(QStringLiteral("modal"), !parentWindow.isEmpty());
  if (req.multiple) options.insert(QStringLiteral("multiple"), true);
  if (req.directory) options.insert(QStringLiteral("directory"), true);
  if (!pick.directory.isEmpty()) {
    QByteArray folder = QFile::encodeName(pick.directory);
    folder.append('\0');
    options.insert(QStringLiteral("current_folder"), folder);
  }
  if (pick.kind == FilePickKind::saveFile && !pick.suggestedName.isEmpty()) {
    options.insert(QStringLiteral("current_name"), pick.suggestedName);
  }
  const QVariant filters = portalFiltersOption(pick.filter);
  if (filters.isValid()) options.insert(QStringLiteral("filters"), filters);

  const QString method = req.method;

  QDBusMessage msg = QDBusMessage::createMethodCall(
      QStringLiteral("org.freedesktop.portal.Desktop"),
      QStringLiteral("/org/freedesktop/portal/desktop"),
      QStringLiteral("org.freedesktop.portal.FileChooser"), method);
  msg << parentWindow << pick.title << options;

  const QDBusMessage reply = bus.call(msg, QDBus::Block, 8000);
  if (reply.type() == QDBusMessage::ErrorMessage) {
    bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"), requestPath,
                   QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
                   &waiter, SLOT(onResponse(uint, QVariantMap)));
    return {};
  }

  waiter.loop.exec();
  bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"), requestPath,
                 QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
                 &waiter, SLOT(onResponse(uint, QVariantMap)));
  ChooserResult out;
  if (waiter.code != 0) {
    out.status = ChooserStatus::cancelled;
    return out;
  }
  out.paths = fileUrisToLocalPaths(waiter.results.value(QStringLiteral("uris")).toStringList());
  out.status = out.paths.isEmpty() ? ChooserStatus::cancelled : ChooserStatus::picked;
  return out;
}

#endif  // AOIDE_HAVE_DBUS
#endif  // Q_OS_LINUX

}  // namespace

QStringList pickFiles(const FilePick& pick) {
#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
  const ChooserResult portal = portalPick(pick);
  if (portal.status != ChooserStatus::unavailable) return portal.paths;
#endif
#if defined(Q_OS_LINUX)
  // Plasma kdialog also talks to the portal; on Wayland a failed portal then
  // blocks forever in kdialog. Skip it and show a real Qt window instead.
  if (QGuiApplication::platformName() != QLatin1String("wayland")) {
    const ChooserResult kde = kdialogPick(pick);
    if (kde.status != ChooserStatus::unavailable) return kde.paths;
  }
#endif
  return widgetPick(pick).paths;
}

}  // namespace aoide
