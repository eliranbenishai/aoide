#include "aoide_fonts.h"
#include "chrome_paint.h"
#include "playlist_group_colors.h"
#include "playlist_groups_window.h"
#include "window_spec.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QImage>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScreen>
#include <QSignalSpy>
#include <QTest>
#include <QWindow>

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

aoide::ChromeTokens greenSkin() {
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
  look.radii.surface = 0;
  look.radii.window = 0;
  return look;
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
  void editorIsAnOrdinaryIndependentWindow();
  void aoideWindowKeepsEveryGroupUsableAtEachZoom_data();
  void aoideWindowKeepsEveryGroupUsableAtEachZoom();
  void ownerPlacesAnEditorThatFitsTheWorkArea();
  void embeddedEditorFitsAndDragsWithinItsContainer();
  void cornersUseTheSameFrameAsOtherAoideWindows();
  void windowUsesTheCurrentSkinAndPreservesGroupColors();
  void validEditsImmediatelyPublishTrimmedNames();
  void enterEndsEditingWithoutClosingAndTabVisitsEachName();
  void emptyIntermediateNamesRestoreTheLastValidName_data();
  void emptyIntermediateNamesRestoreTheLastValidName();
  void namesStayWithinThePersistedLengthLimit();
  void closingKeepsPublishedChanges_data();
  void closingKeepsPublishedChanges();
  void appearanceChangesPreserveTypingAndSelection();
};

void PlaylistGroupsWindowTest::editorIsAnOrdinaryIndependentWindow() {
  QWidget owner;
  owner.show();
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 75, &owner);
  window.show();
  QCoreApplication::processEvents();
  QVERIFY(!qobject_cast<QDialog*>(&window));
  QCOMPARE(window.parentWidget(), nullptr);
  QCOMPARE(window.windowModality(), Qt::NonModal);
  QCOMPARE(window.windowType(), Qt::Window);
  QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(window.windowHandle());
  QCOMPARE(window.windowHandle()->parent(), nullptr);
  QCOMPARE(window.windowHandle()->transientParent(), nullptr);
  QVERIFY(!button(window, "saveGroups"));
  QVERIFY(!button(window, "cancelGroups"));
}

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
  auto* close = button(window, "closeGroups");
  QVERIFY(close);
  QVERIFY(close->isVisible());
  QVERIFY(window.rect().contains(QRect(close->mapTo(&window, QPoint()), close->size())));
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

void PlaylistGroupsWindowTest::ownerPlacesAnEditorThatFitsTheWorkArea() {
  QWidget owner;
  owner.resize(320, 180);
  owner.show();
  QCoreApplication::processEvents();
  QVERIFY(owner.screen());
  const QRect available = owner.screen()->availableGeometry();
  aoide::PlaylistGroupsWindow requested(groups(), aoide::ChromeTokens::builtin(), 150);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 150, &owner);
  window.show();
  QCoreApplication::processEvents();
  QVERIFY(window.width() <= requested.width());
  QVERIFY(window.width() <= available.width());
  QVERIFY(window.height() <= available.height());
  auto* last = field(window, 6);
  QVERIFY(last);
  QVERIFY(last->isVisible());
  QVERIFY(window.rect().contains(QRect(last->mapTo(&window, QPoint()), last->size())));
  // This verifies fitting and initial placement using mapped coordinates;
  // offscreen results make no claim about desktop compositor move behavior.
  QVERIFY(available.contains(QRect(window.mapToGlobal(QPoint()), window.size())));
  QVERIFY(available.contains(QRect(last->mapToGlobal(QPoint()), last->size())));
}

void PlaylistGroupsWindowTest::cornersUseTheSameFrameAsOtherAoideWindows() {
  const auto look = aoide::ChromeTokens::builtin();
  aoide::PlaylistGroupsWindow window(groups(), look, 100);
  window.show();
  QCoreApplication::processEvents();
  const QImage actual = window.grab().toImage();
  QImage frame(actual.size(), QImage::Format_ARGB32_Premultiplied);
  frame.setDevicePixelRatio(actual.devicePixelRatio());
  frame.fill(Qt::transparent);
  QPainter painter(&frame);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  const auto title = aoide::TitleChromeLayout::forWindow(aoide::WindowId::settings, window.size());
  aoide::paintWindowFrame(painter, window.size(), title, look);
  painter.end();
  const int corner = qRound(8 * actual.devicePixelRatio());
  const QRect corners[] = {{0, 0, corner, corner},
                           {actual.width() - corner, 0, corner, corner},
                           {0, actual.height() - corner, corner, corner},
                           {actual.width() - corner, actual.height() - corner, corner, corner}};
  for (const QRect& bounds : corners) QCOMPARE(actual.copy(bounds), frame.copy(bounds));
}

void PlaylistGroupsWindowTest::embeddedEditorFitsAndDragsWithinItsContainer() {
  QWidget container;
  container.resize(700, 500);
  QWidget owner(&container);
  owner.setGeometry(55, 65, 320, 180);
  container.show();
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 150, &owner);
  window.setParent(&container, Qt::Widget);
  window.setAppearance(aoide::ChromeTokens::builtin(), 150, &owner);
  window.show();
  QCoreApplication::processEvents();
  QVERIFY(!window.isWindow());
  QVERIFY(container.contentsRect().contains(window.geometry()));
  const auto dragTo = [&](QPoint destination) {
    const QPoint start(window.width() / 2, 10);
    const QPoint globalStart = window.mapToGlobal(start);
    const QPoint globalEnd = container.mapToGlobal(destination);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(start), QPointF(globalStart),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&window, &press);
    QMouseEvent move(QEvent::MouseMove, QPointF(window.mapFromGlobal(globalEnd)),
                      QPointF(globalEnd), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&window, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(window.mapFromGlobal(globalEnd)),
                         QPointF(globalEnd), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&window, &release);
  };
  dragTo(QPoint(-1000, -1000));
  QCOMPARE(window.pos(), container.contentsRect().topLeft());
  QVERIFY(container.contentsRect().contains(window.geometry()));
  const QPoint start = window.pos();
  dragTo(QPoint(2000, 2000));
  QVERIFY(window.pos() != start);
  QVERIFY(container.contentsRect().contains(window.geometry()));
  QCOMPARE(container.size(), QSize(700, 500));
}

void PlaylistGroupsWindowTest::windowUsesTheCurrentSkinAndPreservesGroupColors() {
  aoide::PlaylistGroupsWindow builtin(groups(), aoide::ChromeTokens::builtin(), 100);
  aoide::PlaylistGroupsWindow skinned(groups(), greenSkin(), 100);
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
  // A focus ring or insertion caret alone must not satisfy this check.
  QVERIFY(changed > before.width() * before.height() / 4);
  for (int id = 0; id < 7; ++id) {
    QVERIFY(matchingPixels(after, aoide::playlistGroupColor(id)) >= 4);
  }
}

void PlaylistGroupsWindowTest::validEditsImmediatelyPublishTrimmedNames() {
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 4);
  QVERIFY(edit);
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  window.show();
  edit->setText(QStringLiteral("  Quiet evenings  "));
  QCOMPARE(changes.size(), 1);
  QCOMPARE(changes.at(0).at(0).toInt(), 4);
  QCOMPARE(changes.at(0).at(1).toString(), QStringLiteral("Quiet evenings"));
  QVERIFY(window.isVisible());
  // Normalization alone does not cause duplicate writes to the collection.
  edit->setText(QStringLiteral("Quiet evenings"));
  QCOMPARE(changes.size(), 1);
  edit->setCursorPosition(edit->text().size());
  QTest::keyClicks(edit, "!");
  QCOMPARE(changes.size(), 2);
  QCOMPARE(changes.at(1).at(1).toString(), QStringLiteral("Quiet evenings!"));
  QCOMPARE(field(window, 0)->text(), QStringLiteral("On repeat"));
}

void PlaylistGroupsWindowTest::enterEndsEditingWithoutClosingAndTabVisitsEachName() {
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
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  last->setText(QStringLiteral("  Music to discover  "));
  QCOMPARE(changes.size(), 1);
  QTest::keyClick(last, Qt::Key_Return);
  QVERIFY(window.isVisible());
  QCOMPARE(last->text(), QStringLiteral("Music to discover"));
  QCOMPARE(changes.size(), 1);
}

void PlaylistGroupsWindowTest::emptyIntermediateNamesRestoreTheLastValidName_data() {
  QTest::addColumn<QString>("invalid");
  QTest::newRow("empty") << QString();
  QTest::newRow("spaces") << QStringLiteral("   ");
  QTest::newRow("unicode-whitespace") << QString(QChar(0x2003));
}

void PlaylistGroupsWindowTest::emptyIntermediateNamesRestoreTheLastValidName() {
  QFETCH(QString, invalid);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 3);
  QVERIFY(edit);
  window.show();
  window.activateWindow();
  edit->setFocus();
  QTRY_VERIFY(edit->hasFocus());
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  edit->setText(invalid);
  QCOMPARE(edit->text(), invalid);
  QCOMPARE(changes.size(), 0);
  QTest::keyClick(edit, Qt::Key_Return);
  QCOMPARE(edit->text(), QStringLiteral("At home"));
  QVERIFY(window.isVisible());
  QCOMPARE(changes.size(), 0);
  edit->setFocus();
  QTRY_VERIFY(edit->hasFocus());
  edit->setText(QStringLiteral("At home again"));
  QCOMPARE(changes.size(), 1);
  edit->setText(invalid);
  QTest::keyClick(edit, Qt::Key_Tab);
  QCOMPARE(edit->text(), QStringLiteral("At home again"));
  QCOMPARE(changes.size(), 1);
}

void PlaylistGroupsWindowTest::namesStayWithinThePersistedLengthLimit() {
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 0);
  QVERIFY(edit);
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  edit->setText(QString(60, QLatin1Char('a')));
  QCOMPARE(edit->text().size(), 60);
  QCOMPARE(changes.size(), 1);
  QCOMPARE(changes.last().at(1).toString().size(), 60);
  edit->setText(QString(61, QLatin1Char('b')));
  QVERIFY(edit->text().size() <= 60);
  QCOMPARE(changes.size(), 2);
  QVERIFY(changes.last().at(1).toString().size() <= 60);
}

void PlaylistGroupsWindowTest::closingKeepsPublishedChanges_data() {
  QTest::addColumn<QString>("dismissal");
  QTest::newRow("escape") << QStringLiteral("escape");
  QTest::newRow("title-close") << QStringLiteral("title-close");
  QTest::newRow("window-close") << QStringLiteral("window-close");
}

void PlaylistGroupsWindowTest::closingKeepsPublishedChanges() {
  QFETCH(QString, dismissal);
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 100);
  auto* edit = field(window, 0);
  QVERIFY(edit);
  window.show();
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  edit->setText(QStringLiteral("Already saved"));
  QCOMPARE(changes.size(), 1);
  edit->setText(QStringLiteral("  "));
  if (dismissal == QStringLiteral("escape")) {
    QTest::keyClick(edit, Qt::Key_Escape);
  } else if (dismissal == QStringLiteral("title-close")) {
    auto* close = button(window, "closeGroups");
    QVERIFY(close);
    QTest::mouseClick(close, Qt::LeftButton);
  } else {
    QVERIFY(window.close());
  }
  QVERIFY(!window.isVisible());
  QCOMPARE(changes.size(), 1);
  QCOMPARE(changes.at(0).at(1).toString(), QStringLiteral("Already saved"));
  window.show();
  QCOMPARE(edit->text(), QStringLiteral("Already saved"));
  QCOMPARE(changes.size(), 1);
}

void PlaylistGroupsWindowTest::appearanceChangesPreserveTypingAndSelection() {
  aoide::PlaylistGroupsWindow window(groups(), aoide::ChromeTokens::builtin(), 75);
  auto* edit = field(window, 2);
  QVERIFY(edit);
  window.show();
  window.activateWindow();
  edit->setFocus();
  QTRY_VERIFY(edit->hasFocus());
  edit->setText(QStringLiteral("  Weekend plans  "));
  edit->setSelection(2, 7);
  const int cursor = edit->cursorPosition();
  QSignalSpy changes(&window, &aoide::PlaylistGroupsWindow::groupNameChanged);
  const int previousWidth = window.width();
  const auto look = greenSkin();
  QCOMPARE(look.id, aoide::ChromeTokens::builtin().id);
  window.setAppearance(look, 125, nullptr);
  QCoreApplication::processEvents();
  QVERIFY(window.width() > previousWidth);
  QCOMPARE(field(window, 2), edit);
  QCOMPARE(edit->text(), QStringLiteral("  Weekend plans  "));
  QCOMPARE(edit->selectedText(), QStringLiteral("Weekend"));
  QCOMPARE(edit->selectionStart(), 2);
  QCOMPARE(edit->cursorPosition(), cursor);
  QVERIFY(edit->hasFocus());
  QCOMPARE(changes.size(), 0);
  const QImage image = window.grab().toImage();
  QVERIFY(matchingPixels(image, look.well) > 100);
  for (int id = 0; id < 7; ++id) QVERIFY(matchingPixels(image, aoide::playlistGroupColor(id)) >= 4);
  window.setAppearance(look, 125, nullptr);
  QCoreApplication::processEvents();
  QCOMPARE(edit->selectedText(), QStringLiteral("Weekend"));
  QCOMPARE(edit->cursorPosition(), cursor);
  QVERIFY(edit->hasFocus());
  QCOMPARE(changes.size(), 0);
}

QTEST_MAIN(PlaylistGroupsWindowTest)
#include "playlist_groups_window_test.moc"
