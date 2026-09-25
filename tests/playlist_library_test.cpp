#include "collection.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class PlaylistLibraryTest : public QObject {
  Q_OBJECT

 private slots:
  void groupAssignmentsAndNamesSurviveRestart();
  void favoritesRemainUniqueAndSurviveSourceRemovalAndRestart();
  void favoriteMetadataContinuesUpdatingAfterSourceRemoval();
  void reservedNamePreservesExistingSavedPlaylists();
  void failedCollectionWritePreservesSavedFavoritesAndReportsFailure();
  void savingCurrentFavoritesDiscardsOnlyAReplacedAlteredSnapshot();
};

void PlaylistLibraryTest::savingCurrentFavoritesDiscardsOnlyAReplacedAlteredSnapshot() {
  QTemporaryDir temp;
  aoide::Track old;
  old.path = temp.filePath(QStringLiteral("old.mp3"));
  aoide::SupportStore store(temp.filePath(QStringLiteral("successful")));
  QVERIFY(store.writeAltered({{old}, temp.filePath(QStringLiteral("old.m3u"))}));
  aoide::PersistHealth health;
  aoide::writeSessionPersist(store, health, {}, {}, {}, aoide::favoritesPlaylistPath(), nullptr);
  QVERIFY(health.lastPlaylistOk);
  QCOMPARE(store.readLastPlaylistPath(), aoide::favoritesPlaylistPath());
  QVERIFY(store.readAltered().isEmpty());

  aoide::SupportStore blocked(temp.filePath(QStringLiteral("blocked")));
  QVERIFY(blocked.writeAltered({{old}, temp.filePath(QStringLiteral("old.m3u"))}));
  QVERIFY(QDir().mkpath(QDir(blocked.dir()).filePath(QStringLiteral("session.json"))));
  aoide::writeSessionPersist(blocked, health, {}, {}, {}, aoide::favoritesPlaylistPath(), nullptr);
  QVERIFY(!health.lastPlaylistOk);
  QCOMPARE(blocked.readAltered().tracks.size(), 1);

  aoide::SupportStore refused(temp.filePath(QStringLiteral("refused-cleanup")));
  QVERIFY(QDir().mkpath(QDir(refused.dir()).filePath(QStringLiteral("altered_playlist.json"))));
  aoide::writeSessionPersist(refused, health, {}, {}, {}, aoide::favoritesPlaylistPath(), nullptr);
  QVERIFY(health.lastPlaylistOk);
  QVERIFY(!health.alteredOk);
}

void PlaylistLibraryTest::groupAssignmentsAndNamesSurviveRestart() {
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  aoide::SupportStore store(temp.path());
  const QString night = temp.filePath(QStringLiteral("night.m3u"));
  const QString morning = temp.filePath(QStringLiteral("morning.m3u"));
  aoide::PlaylistCollection collection;
  collection.addWritten(night, {});
  collection.addWritten(morning, {});
  QVERIFY(collection.setGroups(night, {4, 5}));
  QVERIFY(collection.setGroups(morning, {3, 4}));
  QVERIFY(collection.renameGroup(4, QStringLiteral("Writing")));
  QVERIFY(collection.saveIndex(store));

  aoide::PlaylistCollection restored;
  restored.load(store);
  QCOMPARE(restored.groups().size(), 7);
  QCOMPARE(restored.groups()[4].name, QStringLiteral("Writing"));
  QCOMPARE(restored.groups()[5].name, QStringLiteral("After dark"));
  aoide::SavedPlaylist saved;
  QVERIFY(restored.resolveForLoad(night, &saved));
  QCOMPARE(saved.groupIds, QSet<int>({4, 5}));
  int writingPlaylists = 0;
  for (const auto& playlist : restored.entries()) {
    if (playlist.groupIds.contains(4)) ++writingPlaylists;
  }
  QCOMPARE(writingPlaylists, 2);
  QVERIFY(!restored.setGroups(night, {7}));
  QVERIFY(!restored.renameGroup(4, QStringLiteral("   ")));
  QVERIFY(restored.setGroups(night, {}));
  QVERIFY(restored.saveIndex(store));
  collection.load(store);
  QVERIFY(collection.resolveForLoad(night, &saved));
  QVERIFY(saved.groupIds.isEmpty());
}

void PlaylistLibraryTest::favoritesRemainUniqueAndSurviveSourceRemovalAndRestart() {
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  aoide::SupportStore store(temp.path());
  const QString first = temp.filePath(QStringLiteral("first.m3u"));
  const QString second = temp.filePath(QStringLiteral("second.m3u"));
  aoide::Track track;
  track.path = temp.filePath(QStringLiteral("album/../track.flac"));
  track.title = QStringLiteral("When the city sleeps");
  track.artist = QStringLiteral("Night Chorus");
  track.album = QStringLiteral("Late hours");
  track.albumArtist = QStringLiteral("Night Chorus Ensemble");
  track.year = 2024;
  track.genre = QStringLiteral("Jazz");
  track.trackNumber = QStringLiteral("3/9");
  track.discNumber = QStringLiteral("1/2");
  track.composer = QStringLiteral("A. Writer");
  track.durationMs = 123000;
  aoide::PlaylistCollection collection;
  collection.addWritten(first, {track});
  aoide::Track same = track;
  same.path = temp.filePath(QStringLiteral("track.flac"));
  collection.addWritten(second, {same});
  QVERIFY(collection.setFavorite(track, true));
  QVERIFY(!collection.setFavorite(same, true));
  QCOMPARE(collection.favoriteTracks().size(), 1);
  QVERIFY(collection.isFavorite(same.path));
  QVERIFY(collection.contains(aoide::favoritesPlaylistPath()));
  QCOMPARE(collection.favoritesEntry().displayName(), QStringLiteral("Favorites"));
  QCOMPARE(collection.favoritesEntry().trackCount, 1);
  collection.select(aoide::favoritesPlaylistPath());
  QCOMPARE(collection.selectedPath(), QStringLiteral("aoide:favorites"));
  QVERIFY(!collection.setGroups(aoide::favoritesPlaylistPath(), {0}));
  collection.remove(aoide::favoritesPlaylistPath());
  QVERIFY(collection.contains(aoide::favoritesPlaylistPath()));

  collection.remove(first);
  collection.remove(second);
  QVERIFY(collection.saveIndex(store));
  QVERIFY(collection.saveTrackSets(store));
  aoide::PlaylistCollection restored;
  restored.load(store);
  QVERIFY(restored.entries().isEmpty());
  const QVector<aoide::Track> favorites = restored.tracksFor(aoide::favoritesPlaylistPath());
  QCOMPARE(favorites.size(), 1);
  track.path = same.path;
  // Missing-file state is evaluated separately from the saved favorite.
  track.disabled = favorites[0].disabled;
  QVERIFY(favorites[0] == track);
  QCOMPARE(restored.favoritesEntry().totalDurationMs, 123000);
  aoide::SavedPlaylist entry;
  QVERIFY(restored.resolveForLoad(aoide::favoritesPlaylistPath(), &entry));
  QCOMPARE(entry.path, QStringLiteral("aoide:favorites"));
  QVERIFY(restored.setFavorite(same, false));
  QVERIFY(restored.saveIndex(store));
  collection.load(store);
  QVERIFY(collection.favoriteTracks().isEmpty());
}

void PlaylistLibraryTest::favoriteMetadataContinuesUpdatingAfterSourceRemoval() {
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  aoide::SupportStore store(temp.path());
  const QString source = temp.filePath(QStringLiteral("mix.m3u"));
  aoide::Track track;
  track.path = temp.filePath(QStringLiteral("track.flac"));
  track.title = QStringLiteral("Old title");
  track.artist = QStringLiteral("Known artist");
  track.durationMs = 1000;
  aoide::PlaylistCollection collection;
  collection.setExists([](const QString&) { return true; });
  collection.addWritten(source, {track});
  QVERIFY(collection.setFavorite(track, true));
  collection.remove(source);
  QVERIFY(collection.saveTrackSets(store));
  collection.mergeTrackDuration(track.path, 44000);
  aoide::TrackMetadata incoming;
  incoming.title = QStringLiteral("Corrected title");
  incoming.album = QStringLiteral("Discovered album");
  collection.mergeTrackTags(track.path, incoming, true);
  QVERIFY(collection.saveIndex(store));
  QVERIFY(collection.saveTrackSets(store));

  aoide::PlaylistCollection restored;
  restored.load(store);
  const auto favorites = restored.favoriteTracks();
  QCOMPARE(favorites.size(), 1);
  QCOMPARE(favorites[0].title, QStringLiteral("Corrected title"));
  QCOMPARE(favorites[0].artist, QStringLiteral("Known artist"));
  QCOMPARE(favorites[0].album, QStringLiteral("Discovered album"));
  QCOMPARE(favorites[0].durationMs.value_or(0), 44000);
  QCOMPARE(restored.readFigures().tracks, 1);
  QCOMPARE(restored.readFigures().totalDurationMs, 44000);
  // Background metadata fills gaps but cannot replace known favorite tags.
  incoming.title = QStringLiteral("Less authoritative title");
  restored.mergeTrackTags(track.path, incoming);
  QCOMPARE(restored.favoriteTracks()[0].title, QStringLiteral("Corrected title"));
}

void PlaylistLibraryTest::reservedNamePreservesExistingSavedPlaylists() {
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  aoide::SupportStore store(temp.path());
  const QString source = temp.filePath(QStringLiteral("Favorites.m3u"));
  const QByteArray original("#EXTM3U\n#EXTINF:12,Artist - Song\ntrack.mp3\n");
  QFile file(source);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(original), original.size());
  file.close();
  aoide::SavedPlaylist legacy;
  legacy.path = source;
  legacy.name = QStringLiteral("  fAvOrItEs  ");
  legacy.trackCount = 1;
  legacy.groupIds = {0, 4};
  aoide::SavedPlaylist existing;
  existing.path = temp.filePath(QStringLiteral("other.m3u"));
  existing.name = QStringLiteral("Favorites (saved)");
  QVERIFY(store.writeCollectionIndex({legacy, existing}));
  aoide::CollectionTrackSets tracks;
  tracks.byEntry.insert(source, {temp.filePath(QStringLiteral("track.mp3"))});
  QVERIFY(store.writeTrackSets(tracks));

  aoide::PlaylistCollection collection;
  collection.load(store);
  aoide::SavedPlaylist migrated;
  QVERIFY(collection.resolveForLoad(source, &migrated));
  QCOMPARE(migrated.displayName(), QStringLiteral("Favorites (saved 2)"));
  QCOMPARE(migrated.groupIds, QSet<int>({0, 4}));
  QCOMPARE(collection.tracksFor(source).size(), 1);
  QVERIFY(collection.favoriteTracks().isEmpty());
  QVERIFY(aoide::isReservedPlaylistName(QStringLiteral("  ＦＡＶＯＲＩＴＥＳ  ")));
  collection.rename(source, QStringLiteral("ＦＡＶＯＲＩＴＥＳ"));
  collection.rename(source, {});  // Falling back to Favorites.m3u is reserved too.
  QVERIFY(collection.resolveForLoad(source, &migrated));
  QCOMPARE(migrated.displayName(), QStringLiteral("Favorites (saved 2)"));
  QVERIFY(collection.saveIndex(store));
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), original);
  file.close();

  // A newly added Favorites.m3u keeps its file and tracks, with a safe label.
  aoide::PlaylistCollection added;
  added.add(source);
  QCOMPARE(added.entries().size(), 1);
  QCOMPARE(added.entries()[0].displayName(), QStringLiteral("Favorites (saved)"));
  QCOMPARE(added.tracksFor(source).size(), 1);
  QCOMPARE(aoide::normalizePlaylistPath(aoide::favoritesPlaylistPath()),
           QStringLiteral("aoide:favorites"));
}

void PlaylistLibraryTest::failedCollectionWritePreservesSavedFavoritesAndReportsFailure() {
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  aoide::SupportStore store(temp.path());
  aoide::PlaylistCollection collection;
  aoide::Track first;
  first.path = temp.filePath(QStringLiteral("first.mp3"));
  QVERIFY(collection.setFavorite(first, true));
  QVERIFY(collection.saveIndex(store));
  aoide::Track second;
  second.path = temp.filePath(QStringLiteral("second.mp3"));
  QVERIFY(collection.setFavorite(second, true));
  const QString favoritesFile = temp.filePath(QStringLiteral("favorites.json"));
#ifdef Q_OS_WIN
  QVERIFY(QFile::setPermissions(favoritesFile, QFileDevice::ReadOwner));
#else
  QVERIFY(QFile::setPermissions(temp.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
#endif
  const bool saved = collection.saveIndex(store);
#ifdef Q_OS_WIN
  QVERIFY(QFile::setPermissions(favoritesFile, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
#else
  QVERIFY(QFile::setPermissions(temp.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                             QFileDevice::ExeOwner));
#endif
  QVERIFY(!saved);
  aoide::PlaylistCollection restored;
  restored.load(store);
  QCOMPARE(restored.favoriteTracks().size(), 1);
  QVERIFY(restored.isFavorite(first.path));
  QVERIFY(!restored.isFavorite(second.path));
  QVERIFY(collection.saveIndex(store));
  restored.load(store);
  QCOMPARE(restored.favoriteTracks().size(), 2);
}

QTEST_GUILESS_MAIN(PlaylistLibraryTest)
#include "playlist_library_test.moc"
