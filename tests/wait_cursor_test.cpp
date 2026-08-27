#include "wait_cursor.h"

#include <QApplication>
#include <QGuiApplication>
#include <QTest>
#include <QTimer>
#include <QWidget>

class WaitCursorTest : public QObject {
  Q_OBJECT

 private slots:
  void scopeSetsTheWidgetCursor();
  void restoreClearsTheWidgetCursor();
  void queuedWorkHoldsTheCursorUntilTheNextTick();
  void scopeDoesNotRunQueuedWork();
};

void WaitCursorTest::scopeSetsTheWidgetCursor() {
  QWidget w;
  w.resize(40, 40);
  w.setCursor(Qt::PointingHandCursor);
  w.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
  {
    aoide::WaitCursorScope wait;
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
    aoide::WaitCursorScope wait;
    QCOMPARE(w.cursor().shape(), Qt::WaitCursor);
  }
  QVERIFY(!QGuiApplication::overrideCursor());
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
}

void WaitCursorTest::queuedWorkHoldsTheCursorUntilTheNextTick() {
  QWidget w;
  w.resize(40, 40);
  w.setCursor(Qt::PointingHandCursor);
  w.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  bool ran = false;
  aoide::withWaitCursor(&w, [&]() {
    ran = true;
    QCOMPARE(w.cursor().shape(), Qt::WaitCursor);
  });
  QVERIFY(!ran);
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
  QTRY_VERIFY(ran);
  QCOMPARE(w.cursor().shape(), Qt::PointingHandCursor);
}

// A scope is entered part-way through a session method. While it pumped the
// event loop, a timer or a queued slot could run against state that method was
// only half way through changing — a persist of a playlist about to be
// replaced, a probe answer landing in a list the caller still held a copy of.
void WaitCursorTest::scopeDoesNotRunQueuedWork() {
  QWidget w;
  w.resize(40, 40);
  w.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  bool queued = false;
  QTimer::singleShot(0, &w, [&]() { queued = true; });
  {
    aoide::WaitCursorScope wait;
    QVERIFY2(!queued, "entering the wait cursor must not run queued work");
  }
  QVERIFY2(!queued, "leaving the wait cursor must not run queued work either");
  QTRY_VERIFY(queued);
}

QTEST_MAIN(WaitCursorTest)
#include "wait_cursor_test.moc"
