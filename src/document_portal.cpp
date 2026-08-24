#include "document_portal.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#ifdef Q_OS_LINUX
#include <climits>
#include <sys/types.h>
#include <sys/xattr.h>
#endif

namespace tramp {

bool isDocumentPortalPath(const QString& path, const QString& runtimeDir) {
  if (path.isEmpty() || runtimeDir.isEmpty()) return false;
  const QString mount = QDir::cleanPath(runtimeDir) + QStringLiteral("/doc/");
  return QDir::cleanPath(path).startsWith(mount);
}

QString documentPortalHostPath(const QString& path) {
#ifdef Q_OS_LINUX
  if (path.isEmpty()) return {};
  const QByteArray local = QFile::encodeName(path);
  char buf[PATH_MAX + 1];
  const ssize_t len =
      getxattr(local.constData(), "user.document-portal.host-path", buf, sizeof(buf) - 1);
  if (len <= 0) return {};
  return QFile::decodeName(QByteArray(buf, int(len)));
#else
  Q_UNUSED(path);
  return {};
#endif
}

QString durablePath(const QString& path, const QString& runtimeDir) {
  if (!isDocumentPortalPath(path, runtimeDir)) return path;
  const QString host = documentPortalHostPath(path);
  // The existence check is what makes this safe to do unconditionally. With
  // --filesystem=host the origin is readable and worth storing; narrow the
  // sandbox and it is not, and then the export -- which dies at logout -- still
  // beats a path that is dead on arrival.
  if (host.isEmpty() || !QFileInfo::exists(host)) return path;
  return host;
}

QStringList durablePaths(const QStringList& paths, const QString& runtimeDir) {
  QStringList out;
  out.reserve(paths.size());
  for (const QString& p : paths) out.push_back(durablePath(p, runtimeDir));
  return out;
}

QStringList durablePaths(const QStringList& paths) {
  return durablePaths(paths, QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));
}

}  // namespace tramp
