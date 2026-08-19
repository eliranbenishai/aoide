#include "support_dir.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace tramp {
namespace {

QString linuxDataHome(const QMap<QString, QString>& environment) {
  const QString xdg = environment.value(QStringLiteral("XDG_DATA_HOME"));
  if (!xdg.isEmpty() && QFileInfo(xdg).isAbsolute()) {
    return xdg;
  }
  const QString home = environment.value(QStringLiteral("HOME"));
  if (!home.isEmpty()) {
    return QDir(home).filePath(QStringLiteral(".local/share"));
  }
  return QDir(QDir::currentPath()).filePath(QStringLiteral(".local/share"));
}

}  // namespace

QString resolveLinuxSupportPath(const QMap<QString, QString>& environment,
                                const std::function<bool(const QString&)>& exists) {
  const QString dataHome = linuxDataHome(environment);
  const QString pinned = QDir(dataHome).filePath(QString::fromLatin1(kApplicationId));
  if (exists(pinned)) {
    return pinned;
  }
  for (const char* name : kLegacyLinuxSupportDirNames) {
    const QString legacy = QDir(dataHome).filePath(QString::fromLatin1(name));
    if (exists(legacy)) {
      return legacy;
    }
  }
  return pinned;
}

QString trampSupportDirectory() {
#ifdef Q_OS_LINUX
  QMap<QString, QString> env;
  const auto system = QProcessEnvironment::systemEnvironment();
  const QStringList keys = system.keys();
  for (const QString& key : keys) {
    env.insert(key, system.value(key));
  }
  const QString path = resolveLinuxSupportPath(env, [](const QString& p) {
    return QFileInfo(p).isDir();
  });
#else
  const QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif
  QDir().mkpath(path);
  return path;
}

}  // namespace tramp
