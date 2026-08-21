#include "layout_sync.h"

#include <QTest>

using tramp::DockLayout;
using tramp::LayoutSync;
using tramp::WindowId;

namespace {

/// A desktop with no compositor behind it. Everything LayoutSync needs from the
/// outside world is one rectangle, so a monitor going away is a field write.
class FakeDesktop : public tramp::PanelSurfaces {
 public:
  explicit FakeDesktop(QRect host = {}) : host_(host) {}
  QRect hostRect() const override { return host_; }
  void setHostRect(QRect host) { host_ = host; }

 private:
  QRect host_;
};

}  // namespace

class LayoutSyncTest : public QObject {
  Q_OBJECT

 private slots:
  void nativeAndLogicalAreInversesAcrossTheZoomLadder();
  void logicalToNativeRoundsRatherThanTruncates();
  void nativeFrameRectZoomsThePanelAndItsOrigin();
  void nativeFrameRectTakesTheStoredPlaylistSizeAndTheShadedHeight();
  void setNativeFrameRoundTripsThroughTheFrame();
  void onlyThePlaylistTakesASizeFromAScreenRectangle();
  void clampPullsAPanelBackOntoTheDesktop();
  void clampLeavesEverythingAloneWhenThereIsNoHostYet();
  void clampAcceptsAScreenLeftOfThePrimary();
};

void LayoutSyncTest::nativeAndLogicalAreInversesAcrossTheZoomLadder() {
  for (int percent : {75, 100, 125, 150}) {
    LayoutSync layout({}, percent);
    QCOMPARE(layout.zoomPercent(), percent);
    const QPoint native(400, 240);
    QCOMPARE(layout.logicalToNative(layout.nativeToLogical(native)), native);
  }
}

void LayoutSyncTest::logicalToNativeRoundsRatherThanTruncates() {
  LayoutSync layout({}, 75);
  // 825 * 0.75 = 618.75. Truncating loses most of a pixel per panel, and the
  // cluster walks a pixel left every time a drag round-trips through here.
  QCOMPARE(layout.logicalToNative(QPointF(825, 348)), QPoint(619, 261));
}

void LayoutSyncTest::nativeFrameRectZoomsThePanelAndItsOrigin() {
  DockLayout dock;
  dock.main = {true, false, 100, 200, {}, {}};
  LayoutSync layout(dock, 100);
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(100, 200, 825, 348));

  LayoutSync zoomed(dock, 150);
  QCOMPARE(zoomed.nativeFrameRect(WindowId::main), QRect(150, 300, 1238, 522));
}

void LayoutSyncTest::nativeFrameRectTakesTheStoredPlaylistSizeAndTheShadedHeight() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, 600));

  // Windowshade is a layout fact, not a paint one: docking measures the
  // collapsed panel by its title bar.
  dock.playlist.shaded = true;
  LayoutSync shaded(dock, 100);
  QCOMPARE(shaded.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, tramp::kTitleBar));
}

void LayoutSyncTest::setNativeFrameRoundTripsThroughTheFrame() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 75);

  const QRect moved(300, 150, 900, 600);
  layout.setNativeFrame(WindowId::playlist, moved);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), moved);
  QCOMPARE(layout.layout().playlist.left, 400.0);
  QCOMPARE(layout.layout().playlist.top, 200.0);
}

void LayoutSyncTest::onlyThePlaylistTakesASizeFromAScreenRectangle() {
  DockLayout dock;
  LayoutSync layout(dock, 100);
  // Main and EQ never stretch, so a rectangle handed to them moves the origin
  // and nothing else — a clamp that shrank one would otherwise stick.
  layout.setNativeFrame(WindowId::main, QRect(40, 60, 200, 100));
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(40, 60, 825, 348));
  QVERIFY(!layout.layout().main.width.has_value());
}

void LayoutSyncTest::clampPullsAPanelBackOntoTheDesktop() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.about = {true, false, 1900, 1000, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(1440, 720, 480, 360));
}

void LayoutSyncTest::clampLeavesEverythingAloneWhenThereIsNoHostYet() {
  DockLayout dock;
  dock.about = {true, false, -4000, -4000, {}, {}};
  LayoutSync layout(dock, 100);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.layout().about.left, -4000.0);

  // An empty rectangle is not a desktop of zero size; it is not knowing yet.
  FakeDesktop none(QRect(0, 0, 0, 0));
  layout.setSurfaces(&none);
  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.layout().about.left, -4000.0);
}

void LayoutSyncTest::clampAcceptsAScreenLeftOfThePrimary() {
  // A monitor placed left of or above the primary gives the virtual desktop a
  // negative origin, and the clamp has to hold the panel inside that rather
  // than against zero.
  FakeDesktop desktop(QRect(-1920, -200, 3840, 1280));
  DockLayout dock;
  dock.about = {true, false, -3000, -900, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(-1920, -200, 480, 360));
}

QTEST_APPLESS_MAIN(LayoutSyncTest)
#include "layout_sync_test.moc"
