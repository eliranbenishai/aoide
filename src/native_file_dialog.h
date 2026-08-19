#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QWidget>

namespace tramp {

enum class FilePickKind { openFile, openFiles, saveFile, openDirectory };

struct FilePick {
  QWidget* parent = nullptr;
  QString title;
  QString directory;
  QString filter;
  QString suggestedName;
  FilePickKind kind = FilePickKind::openFile;
};

struct FileFilterGroup {
  QString name;
  QStringList globs;
};

/// Cursor AppImages (and similar hosts) leak Qt 4/5 plugin paths into the
/// environment. Those hide our Qt 6 portal/theme plugins and force the widget
/// file dialog.
inline bool qtPluginPathNeedsSanitize(const QByteArray& path) {
  if (path.isEmpty()) return false;
  const QByteArray lower = path.toLower();
  return lower.contains(".mount_cursor") || lower.contains("/qt4/plugins") ||
         lower.contains("/qt5/plugins");
}

inline void sanitizeInheritedQtPluginPath() {
  if (qtPluginPathNeedsSanitize(qgetenv("QT_PLUGIN_PATH"))) {
    qunsetenv("QT_PLUGIN_PATH");
  }
}

inline QVector<FileFilterGroup> parseQtFileFilter(const QString& filter) {
  QVector<FileFilterGroup> out;
  const QStringList groups = filter.split(QStringLiteral(";;"), Qt::SkipEmptyParts);
  for (const QString& group : groups) {
    const int open = group.lastIndexOf(QLatin1Char('('));
    const int close = group.lastIndexOf(QLatin1Char(')'));
    FileFilterGroup parsed;
    if (open > 0 && close > open) {
      parsed.name = group.left(open).trimmed();
      const QString inside = group.mid(open + 1, close - open - 1);
      for (const QString& part : inside.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        parsed.globs.push_back(part.trimmed());
      }
    } else {
      parsed.name = group.trimmed();
    }
    if (!parsed.name.isEmpty()) out.push_back(parsed);
  }
  return out;
}

inline QStringList fileUrisToLocalPaths(const QStringList& uris) {
  QStringList paths;
  paths.reserve(uris.size());
  for (const QString& uri : uris) {
    const QString path = QUrl(uri).toLocalFile();
    if (!path.isEmpty()) paths.push_back(path);
  }
  return paths;
}

/// xdg-desktop-portal FileChooser has OpenFile / SaveFile / SaveFiles only.
/// Folder pick is OpenFile with options.directory = true (portal version 3).
struct PortalFileChooserRequest {
  QString method;
  bool directory = false;
  bool multiple = false;
};

inline PortalFileChooserRequest portalFileChooserRequest(FilePickKind kind) {
  PortalFileChooserRequest req;
  switch (kind) {
    case FilePickKind::openFiles:
      req.method = QStringLiteral("OpenFile");
      req.multiple = true;
      return req;
    case FilePickKind::openFile:
      req.method = QStringLiteral("OpenFile");
      return req;
    case FilePickKind::saveFile:
      req.method = QStringLiteral("SaveFile");
      return req;
    case FilePickKind::openDirectory:
      req.method = QStringLiteral("OpenFile");
      req.directory = true;
      return req;
  }
  req.method = QStringLiteral("OpenFile");
  return req;
}

/// OS file chooser: xdg-desktop-portal on Linux (Dolphin/Nautilus), native
/// QFileDialog on Windows/macOS. Falls back to kdialog, then the Qt widget dialog.
QStringList pickFiles(const FilePick& pick);

inline QString pickFile(const FilePick& pick) {
  const QStringList paths = pickFiles(pick);
  return paths.isEmpty() ? QString() : paths.front();
}

}  // namespace tramp
