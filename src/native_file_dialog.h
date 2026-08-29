#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QWidget>
#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QList>
#include <QVariant>
#endif

namespace aoide {

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

/// Qt `QFileDialog` filter line from a display name and extensions without the
/// dot. Formats stay in `files.cpp`; only the dialog syntax lives here.
inline QString qtFileFilter(const QString& name, const QStringList& extensions) {
  QStringList globs;
  globs.reserve(extensions.size());
  for (const QString& ext : extensions) {
    globs.push_back(QStringLiteral("*.") + ext);
  }
  return name + QStringLiteral(" (") + globs.join(QLatin1Char(' ')) + QLatin1Char(')');
}

/// Portal FileChooser globs are case-sensitive. Real files are named
/// `Track.MP3`, so each letter becomes a class; digits and punctuation stay.
inline QString caseInsensitiveGlob(const QString& glob) {
  QString out;
  out.reserve(glob.size() * 4);
  for (const QChar ch : glob) {
    if (ch.isLetter()) {
      out += QLatin1Char('[');
      out += ch.toLower();
      out += ch.toUpper();
      out += QLatin1Char(']');
    } else {
      out += ch;
    }
  }
  return out;
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

#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
/// Portal FileChooser `filters` option: `a(sa(us))`. Type 0 is a glob.
struct PortalFilterPattern {
  uint type = 0;
  QString pattern;
};

struct PortalFilterGroup {
  QString name;
  QList<PortalFilterPattern> patterns;
};

using PortalFilterList = QList<PortalFilterGroup>;

inline QDBusArgument& operator<<(QDBusArgument& arg, const PortalFilterPattern& pattern) {
  arg.beginStructure();
  arg << pattern.type << pattern.pattern;
  arg.endStructure();
  return arg;
}

inline const QDBusArgument& operator>>(const QDBusArgument& arg, PortalFilterPattern& pattern) {
  arg.beginStructure();
  arg >> pattern.type >> pattern.pattern;
  arg.endStructure();
  return arg;
}

inline QDBusArgument& operator<<(QDBusArgument& arg, const PortalFilterGroup& group) {
  arg.beginStructure();
  arg << group.name << group.patterns;
  arg.endStructure();
  return arg;
}

inline const QDBusArgument& operator>>(const QDBusArgument& arg, PortalFilterGroup& group) {
  arg.beginStructure();
  arg >> group.name >> group.patterns;
  arg.endStructure();
  return arg;
}

inline void registerPortalFilterTypes() {
  static const int registered = [] {
    qDBusRegisterMetaType<PortalFilterPattern>();
    qDBusRegisterMetaType<QList<PortalFilterPattern>>();
    qDBusRegisterMetaType<PortalFilterGroup>();
    qDBusRegisterMetaType<PortalFilterList>();
    return 0;
  }();
  Q_UNUSED(registered);
}

/// Empty when `qtFilter` is empty. Otherwise the portal `filters` value.
inline QVariant portalFiltersOption(const QString& qtFilter) {
  if (qtFilter.isEmpty()) return {};
  const QVector<FileFilterGroup> groups = parseQtFileFilter(qtFilter);
  if (groups.isEmpty()) return {};
  registerPortalFilterTypes();
  PortalFilterList filters;
  filters.reserve(groups.size());
  for (const FileFilterGroup& group : groups) {
    PortalFilterGroup out;
    out.name = group.name;
    for (const QString& glob : group.globs) {
      PortalFilterPattern pattern;
      pattern.type = 0;
      pattern.pattern = caseInsensitiveGlob(glob);
      out.patterns.push_back(pattern);
    }
    filters.push_back(out);
  }
  return QVariant::fromValue(filters);
}
#endif

/// OS file chooser: xdg-desktop-portal on Linux (Dolphin/Nautilus), native
/// QFileDialog on Windows/macOS. Falls back to kdialog, then the Qt widget dialog.
QStringList pickFiles(const FilePick& pick);

inline QString pickFile(const FilePick& pick) {
  const QStringList paths = pickFiles(pick);
  return paths.isEmpty() ? QString() : paths.front();
}

}  // namespace aoide

#if defined(Q_OS_LINUX) && defined(AOIDE_HAVE_DBUS)
Q_DECLARE_METATYPE(aoide::PortalFilterPattern)
Q_DECLARE_METATYPE(QList<aoide::PortalFilterPattern>)
Q_DECLARE_METATYPE(aoide::PortalFilterGroup)
Q_DECLARE_METATYPE(aoide::PortalFilterList)
#endif
