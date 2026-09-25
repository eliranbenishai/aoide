#include "aoide_fonts.h"
#include "chrome_bodies.h"
#include "chrome_hits.h"
#include "chrome_layout.h"
#include "chrome_menu.h"
#include "mockup_draw.h"
#include "playlist_group_colors.h"
#include "session_view.h"

#include <QImage>
#include <QPainter>
#include <QTest>

namespace {

QImage paintPlaylist(const aoide::SessionView& view, QSize size = aoide::kPlaylistDefault) {
  QImage image(size, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::black);
  QPainter painter(&image);
  const aoide::LookPaintScope look(view.look);
  aoide::paintWindowBody(painter, aoide::WindowId::playlist, size, nullptr, view);
  return image;
}

QRect colorBounds(const QImage& image, const QRect& within, const QColor& color) {
  QRect found;
  for (int y = within.top(); y <= within.bottom(); ++y) {
    for (int x = within.left(); x <= within.right(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      // The glass wash shifts a solid mark by up to nine channels. A wider
      // tolerance also mistakes the ordinary name ink for the gray group.
      if (qAbs(pixel.red() - color.red()) < 10 &&
          qAbs(pixel.green() - color.green()) < 10 &&
          qAbs(pixel.blue() - color.blue()) < 10) {
        found = found.united(QRect(x, y, 1, 1));
      }
    }
  }
  return found;
}

}  // namespace

class PlaylistGroupsChromeTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() { aoide::loadAoideFonts(); }
  void groupButtonSharesTheExistingToolbarAtEveryCollectionWidth();
  void favoritesStaysPaintedAndClickableAboveScrolledPlaylists();
  void groupMarksKeepTheirOrderBesideCountsWhenNamesAreLong();
  void favoriteAndGroupChangesInvalidateOnlyRelevantPaint();
  void automaticPlaylistWithdrawsMutatingControls();
  void collectionEditsFollowTheVisibleSelectedReference();
  void menuSwatchesReserveSpaceBeyondTheCheckGutter();
};

void PlaylistGroupsChromeTest::groupButtonSharesTheExistingToolbarAtEveryCollectionWidth() {
  using K = aoide::ChromeHit::Kind;
  for (const int width : {180, 240, 350}) {
    aoide::SessionView view;
    view.collectionWidth = width;
    const QRectF inner = aoide::playlistCollectionInner(
        aoide::playlistCollectionColumn(aoide::panelBody(aoide::kPlaylistDefault), width));
    const auto toolbar = aoide::layoutPlaylistCollectionButtons(inner);
    const std::pair<QRectF, K> controls[] = {
        {toolbar.add, K::plAddCollection}, {toolbar.create, K::plCreate},
        {toolbar.rename, K::plRename}, {toolbar.remove, K::plRemoveCollection},
        {toolbar.groups, K::plGroups}};
    QCOMPARE(toolbar.groups.height(), qreal(24));
    QVERIFY(toolbar.groups.width() >= 66);
    for (const auto& [rect, kind] : controls) {
      QCOMPARE(rect.top(), toolbar.groups.top());
      QVERIFY(inner.contains(rect));
      const QRect target = rect.toAlignedRect();
      for (const QPoint& point : {target.center(), target.topLeft(), target.bottomRight()}) {
        const auto hit = aoide::hitTest(aoide::WindowId::playlist, aoide::kPlaylistDefault,
                                       point, view);
        QCOMPARE(hit.kind, kind);
        QCOMPARE(hit.rect, target);
      }
      for (const auto& [other, otherKind] : controls) {
        if (kind != otherKind) QVERIFY(!target.intersects(other.toAlignedRect()));
      }
    }
  }
}

void PlaylistGroupsChromeTest::favoritesStaysPaintedAndClickableAboveScrolledPlaylists() {
  aoide::SessionView view;
  view.collection.append({QStringLiteral("Favorites"), 3, false, false, {}, true});
  for (int i = 0; i < 40; ++i) {
    view.collection.append({QStringLiteral("Playlist %1").arg(i), i + 1});
  }
  const QRectF well = aoide::playlistCollectionWell(aoide::panelBody(aoide::kPlaylistDefault),
                                                   view.collectionWidth);
  const QRectF rows = aoide::playlistCollectionRowsRect(well, view.collection.size());
  const QRect first(rows.left(), rows.top() + 4, rows.width(), 25);
  const QImage initial = paintPlaylist(view);
  view.collectionScroll = 7;
  const QImage scrolled = paintPlaylist(view);
  QCOMPARE(initial.copy(first), scrolled.copy(first));
  QVERIFY(initial.copy(first.translated(0, 26)) != scrolled.copy(first.translated(0, 26)));
  const auto pinned = aoide::hitTest(aoide::WindowId::playlist, aoide::kPlaylistDefault,
                                    first.center(), view);
  QCOMPARE(pinned.kind, aoide::ChromeHit::Kind::plCollectionRow);
  QCOMPARE(pinned.index, 0);
  QCOMPARE(aoide::hitTest(aoide::WindowId::playlist, aoide::kPlaylistDefault,
                          first.center() + QPoint(0, 26), view).index, 8);
  view.collection.front().favorites = false;
  QCOMPARE(aoide::hitTest(aoide::WindowId::playlist, aoide::kPlaylistDefault,
                          first.center(), view).index, 7);
}

void PlaylistGroupsChromeTest::groupMarksKeepTheirOrderBesideCountsWhenNamesAreLong() {
  aoide::SessionView view;
  view.collection.append({QStringLiteral("A playlist name long enough to run beyond every mark"),
                           123, false, false, {0, 1, 2, 3, 4, 5, 6}});
  const QRectF well = aoide::playlistCollectionWell(aoide::panelBody(aoide::kPlaylistDefault),
                                                   view.collectionWidth);
  const QRect row(well.left(), well.top() + 4, well.width(), 26);
  const QImage image = paintPlaylist(view);
  int previousRight = int(well.center().x());
  for (int id = 0; id < 7; ++id) {
    const QRect mark = colorBounds(image, row, aoide::playlistGroupColor(id));
    QVERIFY2(!mark.isEmpty(), qPrintable(QStringLiteral("Missing group mark %1").arg(id)));
    QVERIFY2(mark.left() > previousRight,
             qPrintable(QStringLiteral("Group %1 begins at %2, previous mark ends at %3")
                            .arg(id).arg(mark.left()).arg(previousRight)));
    QVERIFY(mark.right() < well.right() - 30);
    previousRight = mark.right();
  }
  // Recolouring the surrounding chrome must not change a group's identity.
  view.look.phos = QColor("#eeeeee");
  const QImage recolored = paintPlaylist(view);
  QVERIFY(!colorBounds(recolored, row, aoide::playlistGroupColor(0)).isEmpty());
  QVERIFY(!colorBounds(recolored, row, aoide::playlistGroupColor(6)).isEmpty());
}

void PlaylistGroupsChromeTest::favoriteAndGroupChangesInvalidateOnlyRelevantPaint() {
  const aoide::SessionView before = aoide::goldenDemoView();
  auto verify = [&](const aoide::SessionView& after) {
    QVERIFY(!aoide::paintsSame(aoide::WindowId::playlist, before, after));
    QVERIFY(aoide::paintsSame(aoide::WindowId::main, before, after));
    QVERIFY(aoide::paintsSame(aoide::WindowId::equalizer, before, after));
    QVERIFY(paintPlaylist(before) != paintPlaylist(after));
  };
  auto changed = before;
  changed.collection.front().groupIds = {1, 4};
  verify(changed);
  changed = before;
  changed.collection.front().favorites = true;
  verify(changed);
  changed = before;
  changed.tracks.front().favorite = true;
  verify(changed);
  changed = before;
  changed.playlistIsFavorites = true;
  verify(changed);
  changed = before;
  changed.playlistGroupFilterLabel = QStringLiteral("At home");
  verify(changed);
  changed = before;
  changed.collectionCanEdit = false;
  verify(changed);
}

void PlaylistGroupsChromeTest::automaticPlaylistWithdrawsMutatingControls() {
  using K = aoide::ChromeHit::Kind;
  aoide::SessionView view;
  view.playlistIsFavorites = true;
  view.collectionCanEdit = false;
  view.playlistAltered = true;
  view.collection.append({QStringLiteral("Favorites"), 3, true, false, {}, true});
  for (K kind : {K::plRename, K::plRemoveCollection, K::plSave, K::plAdd, K::plRemove,
                 K::plSort, K::plRefresh}) {
    QVERIFY(!aoide::chromeHitEnabled({kind, -1, {}}, view));
  }
  for (K kind : {K::plGroups, K::plCreate, K::plAddCollection, K::plPlay, K::plTrackRow}) {
    QVERIFY(aoide::chromeHitEnabled({kind, -1, {}}, view));
  }
}

void PlaylistGroupsChromeTest::collectionEditsFollowTheVisibleSelectedReference() {
  using K = aoide::ChromeHit::Kind;
  aoide::SessionView view;
  view.playlistIsFavorites = true;
  view.collection.append({QStringLiteral("Favorites"), 3, true, false, {}, true});
  view.collection.append({QStringLiteral("Evening"), 12});
  view.collectionSelected = QStringLiteral("/music/evening.m3u");
  view.collectionCanEdit = true;
  // The current automatic playlist does not stop editing a different saved
  // reference selected by the collection's context menu.
  for (K kind : {K::plRename, K::plRemoveCollection}) {
    QVERIFY(aoide::chromeHitEnabled({kind, -1, {}}, view));
  }
  view.playlistIsFavorites = false;
  view.collectionCanEdit = false;
  view.collection.removeLast();
  // A filter-hidden reference remains remembered, but cannot be edited by a
  // toolbar whose visible list offers no indication of that target.
  for (K kind : {K::plRename, K::plRemoveCollection}) {
    QVERIFY(!aoide::chromeHitEnabled({kind, -1, {}}, view));
  }
}

void PlaylistGroupsChromeTest::menuSwatchesReserveSpaceBeyondTheCheckGutter() {
  auto row = aoide::ChromeMenuItem::check(QStringLiteral("Road trips"), true);
  const auto metrics = aoide::chromeMenuMetrics(1);
  const QSize plain = aoide::chromeMenuSize({row}, 100, metrics);
  row.swatch = aoide::playlistGroupColor(1);
  const QSize colored = aoide::chromeMenuSize({row}, 100, metrics);
  QCOMPARE(colored.width() - plain.width(), 19);
  QCOMPARE(colored.height(), plain.height());
  QCOMPARE(aoide::chromeMenuRowAt({row}, metrics.padY + 2, metrics), 0);
}

QTEST_MAIN(PlaylistGroupsChromeTest)
#include "playlist_groups_chrome_test.moc"
