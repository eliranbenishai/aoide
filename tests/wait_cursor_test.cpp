#include "wait_cursor.h"

#include <QApplication>
#include <QGuiApplication>
#include <QTest>
#include <QWidget>

class WaitCursorTest : public QObject {
  Q_OBJECT

 private slots:
  void scopeSetsTheWidgetCursor();
  void restoreClearsTheWidgetCursor();
};

void WaitCursorTest::scopeSetsTheWidgetCursor() {
  QWidget w;
  w.resize(40, 40);
  w.setCursor(Qt::PointingHandCursor);
  w.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
  {
    tramp::WaitCursorScope wait;
    QCOMPARE(w.cursor().shape(), Qt::WaitCursor);
    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);
  }
}

void WaitCursorTest::restoreClearsTheWidgetCursor() {
  QWidget w;
  w.resize(40, 40);
  w.setCursor(Qt::PointingHandCursor);
  w.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  {
    tramp::WaitCursorScope wait;
    QCOMPARE(w.cursor().shape(), Qt::WaitCursor);
  }
  QVERIFY(!QGuiApplication::overrideCursor());
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
}

QTEST_MAIN(WaitCursorTest)
#include "wait_cursor_test.moc"
