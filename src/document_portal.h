#pragma once

#include <QString>
#include <QStringList>

namespace tramp {

/// Paths that reach the app from outside it -- "Open with" from a file manager,
/// a drag from one, a portal file pick -- do not always name the file. Under a
/// sandbox they name an *export* of it under the document portal's mount, and an
/// export made for a launch is held only in the portal service's memory: it
/// outlives the process it was handed to, so it looks durable, and then dies
/// with the service at logout. Persisting one yields a playlist row that is
/// dead after a reboot although the file never moved.
///
/// The portal records the real path on the export as an xattr, which is the only
/// way back. These turn an export into the path worth writing down, and leave
/// every other path exactly as it came in.

/// True when `path` lies under the document portal's mount, `$XDG_RUNTIME_DIR/doc`.
bool isDocumentPortalPath(const QString& path, const QString& runtimeDir);

/// The path an export stands in for, from the portal's
/// `user.document-portal.host-path` xattr. Empty when there is no such xattr,
/// which is the answer for every path that is not an export.
QString documentPortalHostPath(const QString& path);

/// `path` rewritten to the path it stands in for, when it is an export whose
/// origin this process can actually reach. Unchanged otherwise -- including for
/// an export we cannot see behind, where it is the only handle available.
QString durablePath(const QString& path, const QString& runtimeDir);
QStringList durablePaths(const QStringList& paths, const QString& runtimeDir);

/// The same, against the running process's `$XDG_RUNTIME_DIR`.
QStringList durablePaths(const QStringList& paths);

}  // namespace tramp
