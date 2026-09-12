#include "native_file_dialog.h"
#include "playlist.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class NativeFileDialogTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void init();
  void cleanup();
  void saveStartsInHomeWhenLaunchedFromRoot();
  void saveUsesExplicitFolder();
  void saveUsesFolderChosenByListener();

 private:
  QString runSavePicker(const QString& directory = {}, const QString& destination = {});

  QTemporaryDir settings_;
  QString previousDirectory_;
  QString initialSelection_;
  bool timedOut_ = false;
};

void NativeFileDialogTest::initTestCase() {
  QVERIFY(settings_.isValid());
  // Keep the dialog's remembered location out of the listener's settings.
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_.path());
  QCoreApplication::setOrganizationName(QStringLiteral("AoideTests"));
  QCoreApplication::setApplicationName(QStringLiteral("SaveDialog"));
}

void NativeFileDialogTest::init() {
  previousDirectory_ = QDir::currentPath();
  QVERIFY(QDir::setCurrent(QDir::rootPath()));
  initialSelection_.clear();
  timedOut_ = false;
}

void NativeFileDialogTest::cleanup() {
  QVERIFY(QDir::setCurrent(previousDirectory_));
}

QString NativeFileDialogTest::runSavePicker(const QString& directory,
                                          const QString& destination) {
  aoide::FilePick pick;
  pick.kind = aoide::FilePickKind::saveFile;
  pick.title = QStringLiteral("Save playlist");
  pick.suggestedName = QStringLiteral("playlist.m3u");
  pick.directory = directory;

  QTimer inspect;
  connect(&inspect, &QTimer::timeout, [&] {
    for (QWidget* widget : QApplication::topLevelWidgets()) {
      auto* dialog = qobject_cast<QFileDialog*>(widget);
      if (!dialog || !dialog->isVisible()) continue;
      inspect.stop();
      initialSelection_ = dialog->selectedFiles().value(0);
      if (destination.isEmpty()) {
        dialog->reject();
      } else {
        dialog->setDirectory(QFileInfo(destination).absolutePath());
        auto* name = dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"));
        if (name) name->setText(QFileInfo(destination).fileName());
        QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
      }
    }
  });
  inspect.start(10);

  QTimer timeout;
  timeout.setSingleShot(true);
  connect(&timeout, &QTimer::timeout, [&] {
    timedOut_ = true;
    for (QWidget* widget : QApplication::topLevelWidgets()) {
      if (auto* dialog = qobject_cast<QFileDialog*>(widget)) dialog->reject();
    }
  });
  timeout.start(3000);
  return aoide::pickFile(pick);
}

void NativeFileDialogTest::saveStartsInHomeWhenLaunchedFromRoot() {
  // A filename without a folder used to resolve to /playlist.m3u when the
  // desktop launched Aoide from /. Inspect the real picker without writing
  // anything to the root or the listener's home folder. Cancel stays cancel.
  QVERIFY(runSavePicker().isEmpty());
  QVERIFY(!timedOut_);
  QCOMPARE(initialSelection_, QDir::home().filePath(QStringLiteral("playlist.m3u")));
}

void NativeFileDialogTest::saveUsesExplicitFolder() {
  QTemporaryDir folder;
  QVERIFY(folder.isValid());
  QVERIFY(runSavePicker(folder.path()).isEmpty());
  QVERIFY(!timedOut_);
  QCOMPARE(initialSelection_, folder.filePath(QStringLiteral("playlist.m3u")));
}

void NativeFileDialogTest::saveUsesFolderChosenByListener() {
  QTemporaryDir folder;
  QVERIFY(folder.isValid());
  const QString destination = folder.filePath(QStringLiteral("NAS favourites.m3u"));
  const QString picked = runSavePicker({}, destination);
  QVERIFY(!timedOut_);
  QCOMPARE(picked, destination);

  aoide::Track track;
  track.path = folder.filePath(QStringLiteral("mounted NAS/album/track.mp3"));
  QVERIFY(aoide::writeM3uFile(picked, {track}));
  QFile file(picked);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QVERIFY(file.readAll().contains(track.path.toUtf8()));
}

int main(int argc, char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#ifdef Q_OS_LINUX
  // Exercise Aoide's picker unattended; no host portal or kdialog process.
  qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/aoide-test-bus");
  qputenv("PATH", "");
#endif
  QApplication app(argc, argv);
  NativeFileDialogTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "native_file_dialog_test.moc"
