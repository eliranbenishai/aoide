#include "skip_taskbar.h"

#include <QTest>
#include <QWindow>

class SkipTaskbarTest : public QObject {
  Q_OBJECT

 private slots:
  void extrasAreTransientsOfMain();
};

void SkipTaskbarTest::extrasAreTransientsOfMain() {
  QWindow main;
  main.setFlags(Qt::FramelessWindowHint | Qt::Window);
  QWindow extra;
  extra.setFlags(Qt::FramelessWindowHint | Qt::Dialog);
  tramp::attachExtraWindow(&extra, &main);
  QCOMPARE(extra.transientParent(), &main);
}

QTEST_MAIN(SkipTaskbarTest)
#include "skip_taskbar_test.moc"
