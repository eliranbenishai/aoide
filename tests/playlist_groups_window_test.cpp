#include "aoide_fonts.h"
#include "playlist_group_colors.h"
#include "playlist_groups_window.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QLineEdit>
#include <QScreen>
#include <QSignalSpy>
#include <QTest>

namespace {

QVector<aoide::PlaylistGroup> groups() {
  return {{0, QStringLiteral("On repeat")}, {1, QStringLiteral("Road trips")},
          {2, QStringLiteral("Weekend")}, {3, QStringLiteral("At home")},
          {4, QStringLiteral("Focus")}, {5, QStringLiteral("After dark")},
          {6, QStringLiteral("To explore")}};
}

QLineEdit* field(aoide::PlaylistGroupsWindow& window, int id) {
  return window.findChild<QLineEdit*>(QStringLiteral("groupName_%1").arg(id));
}

QAbstractButton* button(aoide::PlaylistGroupsWindow& window, const char* name) {
  return window.findChild<QAbstractButton*>(QString::fromLatin1(name));
}

int matchingPixels(const QImage& image, const QColor& color) {
  int count = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (pixel.alpha() > 240 && qAbs(pixel.red() - color.red()) < 10 &&
          qAbs(pixel.green() - color.green()) < 10 &&
          qAbs(pixel.blue() - color.blue()) < 10) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

class PlaylistGroupsWindowTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() { aoide::loadAoideFonts(); }
  void aoideWindowKeepsEveryGroupUsableAtEachZoom_data();
  void aoideWindowKeepsEveryGroupUsableAtEachZoom();
  void parentedWindowKeepsSaveReachableWithinTheWorkArea();
  void windowUsesTheCurrentSkinAndPreservesGroupColors();
  void saveReturnsTrimmedNames();
  void enterSavesAndTabVisitsTheGroupNamesInOrder();
  void emptyNamesCannotBeSaved_data();
  void emptyNamesCannotBeSaved();
  void namesStayWithinThePersistedLengthLimit();
  void dismissalDoesNotAcceptEdits_data();
  void dismissalDoesNotAcceptEdits();
};

void PlaylistGroupsWindowTest::aoideWindowKeepsEveryGroupUsableAtEachZoom_data() {
  QTest::addColumn<qreal>("zoom");
  for (const qreal zoom : {50., 62.5, 75., 100., 125., 150.}) {
    QTest::newRow(qPrintable(QString::number(zoom))) << zoom;
  }
}

void PlaylistGroupsWindowTest::aoideWindowKeepsEveryGroupUsableAtEachZoom() {
  QFETCH(qreal, zoom);
  const auto original = groups();
  aoide::PlaylistGroupsWindow window(original, aoide::ChromeTokens::builtin(), zoom);
  aoide::PlaylistGroupsWindow unscaled(original, aoide::ChromeTokens::builtin(), 100);
  QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(qAbs(window.width() - unscaled.width() * zoom / 100.) <= 1);
  window.show();
  QCoreApplication::processEvents();
  QCOMPARE(window.findChildren<QLineEdit*>().size(), 7);
  QRect previous;
  for (const auto& group : original) {
    auto* edit = field(window, group.id);
    QVERIFY(edit);
    QCOMPARE(edit->text(), group.name);
    QVERIFY(!edit->accessibleName().isEmpty());
    QVERIFY(edit->isVisible());
    const QRect bounds(edit->mapTo(&window, QPoint()), edit->size());
    QVERIFY(window.rect().contains(bounds));
    QVERIFY(!previous.intersects(bounds));
    QVERIFY(bounds.top() > previous.bottom());
    QVERIFY(edit->contentsRect().height() >= edit->fontMetrics().height());
    previous = bounds;
  }
  for (const char* name : {"saveGroups", "cancelGroups", "closeGroups"}) {
    auto* action = button(window, name);
    QVERIFY(action);
    QVERIFY(action->isVisible());
    QVERIFY(window.rect().contains(QRect(action->mapTo(&window, QPoint()), action->size())));
  }
  const QImage image = window.grab().toImage();
  for (const auto& group : original) {
    QVERIFY2(matchingPixels(image, aoide::playlistGroupColor(group.id)) >= 4,
             qPrintable(QStringLiteral("Missing visible color for group %1").arg(group.id)));
  }
  const QString dump = qEnvironmentVariable("AOIDE_GROUPS_WINDOW_DUMP");
  if (!dump.isEmpty()) {
    QVERIFY(QDir().mkpath(dump));
    QVERIFY(image.save(QDir(dump).filePath(QStringLiteral("groups-%1.png").arg(zoom))));
  }
}

void PlaylistGroupsWindowTest::saveReturnsTrimmedNames() {
  const auto original = groups();
  aoide::PlaylistGroupsWindow window(original, aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 4);
  auto* save = button(window, "saveGroups");
  QVERIFY(edit);
  QVERIFY(save);
  QSignalSpy accepted(&window, &QDialog::accepted);
  window.show();
  edit->setText(QStringLiteral("  Quiet evenings  "));
  QVERIFY(save->isEnabled());
  QTest::mouseClick(save, Qt::LeftButton);
  QCOMPARE(accepted.size(), 1);
  QCOMPARE(window.result(), int(QDialog::Accepted));
  QCOMPARE(window.groupNames().at(4), QStringLiteral("Quiet evenings"));
  QCOMPARE(window.groupNames().at(0), original.at(0).name);
}

void PlaylistGroupsWindowTest::parentedWindowKeepsSaveReachableWithinTheWorkArea() {
  QWidget parent;
  parent.resize(320, 180);
  parent.show();
  QCoreApplication::processEvents();
  QVERIFY(parent.screen());
  const QRect available = parent.screen()->availableGeometry();
  aoide::PlaylistGroupsWindow requested(groups(), aoide::ChromeTokens::builtin(), 150);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 150, &parent);
  window.show();
  QCoreApplication::processEvents();
  QCOMPARE(window.windowModality(), Qt::WindowModal);
  QVERIFY(window.width() <= requested.width());
  QVERIFY(window.width() <= available.width());
  QVERIFY(window.height() <= available.height());
  auto* save = button(window, "saveGroups");
  QVERIFY(save);
  QVERIFY(save->isVisible());
  QVERIFY(window.rect().contains(QRect(save->mapTo(&window, QPoint()), save->size())));
  // This verifies fitting and initial placement using the mapped coordinates;
  // offscreen results make no claim about desktop compositor move behavior.
  QVERIFY(available.contains(QRect(window.mapToGlobal(QPoint()), window.size())));
  QVERIFY(available.contains(QRect(save->mapToGlobal(QPoint()), save->size())));
}

void PlaylistGroupsWindowTest::windowUsesTheCurrentSkinAndPreservesGroupColors() {
  auto look = aoide::ChromeTokens::builtin();
  look.shellHi = QColor("#a5c789");
  look.shell = QColor("#597d35");
  look.shellMid = QColor("#405e20");
  look.shellLo = QColor("#283c14");
  look.shellDeep = QColor("#16240a");
  look.titleBar0 = QColor("#719c45");
  look.titleBar26 = QColor("#527c2d");
  look.titleBar62 = QColor("#385a1e");
  look.titleBar100 = QColor("#223b10");
  look.phos = QColor("#ff9de3");
  look.ink = QColor("#fff3df");
  look.well = QColor("#162b0c");
  aoide::PlaylistGroupsWindow builtin(groups(), aoide::ChromeTokens::builtin(), 100);
  aoide::PlaylistGroupsWindow skinned(groups(), look, 100);
  builtin.show();
  skinned.show();
  QCoreApplication::processEvents();
  const QImage before = builtin.grab().toImage();
  const QImage after = skinned.grab().toImage();
  QCOMPARE(before.size(), after.size());
  int changed = 0;
  for (int y = 0; y < before.height(); ++y) {
    for (int x = 0; x < before.width(); ++x) {
      if (before.pixel(x, y) != after.pixel(x, y)) ++changed;
    }
  }
  // A focus ring or insertion caret alone must not satisfy this check: the
  // window itself must inherit the skin, while group identity stays stable.
  QVERIFY(changed > before.width() * before.height() / 4);
  for (int id = 0; id < 7; ++id) {
    QVERIFY(matchingPixels(after, aoide::playlistGroupColor(id)) >= 4);
  }
}

void PlaylistGroupsWindowTest::enterSavesAndTabVisitsTheGroupNamesInOrder() {
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 75);
  window.show();
  window.activateWindow();
  auto* first = field(window, 0);
  QVERIFY(first);
  first->setFocus();
  QTRY_VERIFY(first->hasFocus());
  for (int id = 1; id < 7; ++id) {
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QTRY_COMPARE(QApplication::focusWidget(), field(window, id));
  }
  auto* last = field(window, 6);
  last->setText(QStringLiteral("  Music to discover  "));
  QSignalSpy accepted(&window, &QDialog::accepted);
  QTest::keyClick(last, Qt::Key_Return);
  QCOMPARE(accepted.size(), 1);
  QCOMPARE(window.result(), int(QDialog::Accepted));
  QCOMPARE(window.groupNames().at(6), QStringLiteral("Music to discover"));
}

void PlaylistGroupsWindowTest::emptyNamesCannotBeSaved_data() {
  QTest::addColumn<QString>("invalid");
  QTest::newRow("empty") << QString();
  QTest::newRow("spaces") << QStringLiteral("   ");
  QTest::newRow("unicode-whitespace") << QString(QChar(0x2003));
}

void PlaylistGroupsWindowTest::emptyNamesCannotBeSaved() {
  QFETCH(QString, invalid);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 3);
  auto* save = button(window, "saveGroups");
  QVERIFY(edit);
  QVERIFY(save);
  window.show();
  QSignalSpy accepted(&window, &QDialog::accepted);
  edit->setText(invalid);
  QVERIFY(!save->isEnabled());
  QTest::mouseClick(save, Qt::LeftButton);
  QTest::keyClick(edit, Qt::Key_Return);
  window.accept();
  QCOMPARE(accepted.size(), 0);
  QVERIFY(window.isVisible());
  edit->setText(QStringLiteral("At home again"));
  QVERIFY(save->isEnabled());
  window.accept();
  QCOMPARE(accepted.size(), 1);
}

void PlaylistGroupsWindowTest::namesStayWithinThePersistedLengthLimit() {
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 0);
  QVERIFY(edit);
  edit->setText(QString(60, QLatin1Char('a')));
  QCOMPARE(edit->text().size(), 60);
  window.accept();
  QCOMPARE(window.result(), int(QDialog::Accepted));
  QCOMPARE(window.groupNames().at(0).size(), 60);
  // Pasting a longer name must never produce a value persistence truncates
  // differently after the listener has accepted the window.
  edit->setText(QString(61, QLatin1Char('b')));
  QVERIFY(edit->text().size() <= 60);
}

void PlaylistGroupsWindowTest::dismissalDoesNotAcceptEdits_data() {
  QTest::addColumn<QString>("dismissal");
  QTest::newRow("cancel") << QStringLiteral("cancel");
  QTest::newRow("escape") << QStringLiteral("escape");
  QTest::newRow("title-close") << QStringLiteral("title-close");
  QTest::newRow("window-close") << QStringLiteral("window-close");
}

void PlaylistGroupsWindowTest::dismissalDoesNotAcceptEdits() {
  QFETCH(QString, dismissal);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 0);
  QVERIFY(edit);
  window.show();
  edit->setText(QStringLiteral("Uncommitted edit"));
  QSignalSpy accepted(&window, &QDialog::accepted);
  QSignalSpy rejected(&window, &QDialog::rejected);
  if (dismissal == QStringLiteral("cancel")) {
    auto* cancel = button(window, "cancelGroups");
    QVERIFY(cancel);
    QTest::mouseClick(cancel, Qt::LeftButton);
  } else if (dismissal == QStringLiteral("escape")) {
    QTest::keyClick(edit, Qt::Key_Escape);
  } else if (dismissal == QStringLiteral("title-close")) {
    auto* close = button(window, "closeGroups");
    QVERIFY(close);
    QTest::mouseClick(close, Qt::LeftButton);
  } else {
    QVERIFY(window.close());
  }
  QCOMPARE(accepted.size(), 0);
  QCOMPARE(rejected.size(), 1);
  QCOMPARE(window.result(), int(QDialog::Rejected));
  QVERIFY(!window.isVisible());
}

QTEST_MAIN(PlaylistGroupsWindowTest)
#include "playlist_groups_window_test.moc"
