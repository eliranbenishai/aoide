#include "session.h"

#include "host_window.h"
#include "m3u.h"
#include "playlist_group_colors.h"
#include "playlist_groups_window.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aoide {

QVector<SavedPlaylist> AoideSession::visibleCollection() const {
  QVector<SavedPlaylist> rows{collection_.favoritesEntry()};
  for (const auto& entry : collection_.entries()) {
    const int filter = settings_.playlistGroupFilter;
    if (filter == -1 || (filter == -2 && entry.groupIds.isEmpty()) ||
        entry.groupIds.contains(filter)) rows.append(entry);
  }
  return rows;
}

void AoideSession::setPlaylistGroupFilter(int groupId) {
  if (groupId < -2 || groupId >= 7) return;
  settings_.playlistGroupFilter = groupId;
  collectionScroll_ = 0;
  schedulePersist();
  refreshChrome();
}

void AoideSession::assignPlaylistGroups(int row, const QSet<int>& groupIds) {
  const auto rows = visibleCollection();
  if (row < 0 || row >= rows.size() || isFavoritesPlaylist(rows[row].path)) return;
  if (collection_.setGroups(rows[row].path, groupIds)) {
    persistCollectionCache();
    refreshChrome();
  }
}

void AoideSession::syncFavoritesPlaylist() {
  if (!isFavoritesPlaylist(playlist_.sourcePath())) return;
  const auto previous = playlist_.tracks();
  QSet<QString> selected;
  for (int index : playlist_.selectedIndices()) selected.insert(previous[index].path);
  const auto favorites = collection_.favoriteTracks();
  playlist_.loadTracks(favorites, favoritesPlaylistPath());
  QSet<int> nextSelection;
  for (int i = 0; i < favorites.size(); ++i) {
    if (selected.contains(favorites[i].path)) nextSelection.insert(i);
  }
  playlist_.selectIndices(nextSelection);
  schedulePathVerify();
}

void AoideSession::toggleFavoriteTrack(int index) {
  const auto tracks = playlist_.tracks();
  if (index < 0 || index >= tracks.size()) return;
  collection_.setFavorite(tracks[index], !collection_.isFavorite(tracks[index].path));
  syncFavoritesPlaylist();
  persistCollectionCache();
  refreshChrome();
}

void AoideSession::presentPlaylistContextMenu(ChromeHit hit, QPoint logical) {
  const QRect anchor(logical, QSize(1, 1));
  if (hit.kind == ChromeHit::Kind::plTrackRow) {
    const auto tracks = playlist_.tracks();
    if (hit.index < 0 || hit.index >= tracks.size()) return;
    if (!playlist_.selectedIndices().contains(hit.index)) playlist_.select(hit.index);
    const bool favorite = collection_.isFavorite(tracks[hit.index].path);
    const QVector<ChromeMenuItem> items{ChromeMenuItem::action(favorite
        ? QStringLiteral("Remove from Favorites") : QStringLiteral("Add to Favorites"))};
    if (execAnchoredMenu(items, windowFor(WindowId::playlist), anchor,
                         PopupAnchor::belowLeft) == 0) toggleFavoriteTrack(hit.index);
    return;
  }
  if (hit.kind != ChromeHit::Kind::plCollectionRow) return;
  const auto rows = visibleCollection();
  if (hit.index < 0 || hit.index >= rows.size()) return;
  const QString path = rows[hit.index].path;
  if (isFavoritesPlaylist(path)) return;
  collection_.select(path);
  for (;;) {
    SavedPlaylist entry;
    if (!collection_.resolveForLoad(path, &entry)) return;
    QVector<ChromeMenuItem> items{ChromeMenuItem::action(QStringLiteral("Clear all groups")),
                                ChromeMenuItem::separator()};
    const auto groups = collection_.groups();
    for (const auto& group : groups) {
      auto item = ChromeMenuItem::check(group.name, entry.groupIds.contains(group.id));
      item.swatch = playlistGroupColor(group.id);
      items.append(item);
    }
    const int chosen = execAnchoredMenu(items, windowFor(WindowId::playlist), anchor,
                                        PopupAnchor::belowLeft);
    if (chosen == kChromeMenuNone) return;
    if (chosen == 0) entry.groupIds.clear();
    else if (chosen >= 2 && chosen - 2 < groups.size()) {
      const int groupId = groups[chosen - 2].id;
      if (entry.groupIds.contains(groupId)) entry.groupIds.remove(groupId);
      else entry.groupIds.insert(groupId);
    } else return;
    collection_.setGroups(path, entry.groupIds);
    persistCollectionCache();
    refreshChrome();
    if (chosen == 0) return;
  }
}

void AoideSession::presentPlaylistGroups(const ChromeHit& hit) {
  QVector<ChromeMenuItem> items{
      ChromeMenuItem::check(QStringLiteral("All playlists"), settings_.playlistGroupFilter == -1),
      ChromeMenuItem::check(QStringLiteral("Unassigned"), settings_.playlistGroupFilter == -2),
      ChromeMenuItem::separator()};
  const auto groups = collection_.groups();
  for (const auto& group : groups) {
    auto item = ChromeMenuItem::check(group.name, settings_.playlistGroupFilter == group.id);
    item.swatch = playlistGroupColor(group.id);
    items.append(item);
  }
  items.append(ChromeMenuItem::separator());
  const int manage = int(items.size());
  items.append(ChromeMenuItem::action(QStringLiteral("Manage groups…")));
  const int chosen = execAnchoredMenu(items, windowFor(WindowId::playlist), hit.rect,
                                      PopupAnchor::aboveLeft);
  if (chosen == 0) setPlaylistGroupFilter(-1);
  else if (chosen == 1) setPlaylistGroupFilter(-2);
  else if (chosen >= 3 && chosen - 3 < groups.size()) setPlaylistGroupFilter(groups[chosen - 3].id);
  else if (chosen == manage) presentManageGroups();
}

void AoideSession::presentManageGroups() {
  const auto groups = collection_.groups();
  PlaylistGroupsWindow dialog(groups, view().look, zoomPercent(), dialogParent(WindowId::playlist));
  if (dialog.exec() != QDialog::Accepted) return;
  const auto names = dialog.groupNames();
  for (int i = 0; i < groups.size(); ++i) collection_.renameGroup(groups[i].id, names[i]);
  persistCollectionCache();
  refreshChrome();
}

std::optional<QString> AoideSession::readPlaylistText(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
  const auto decoded = M3uCodec().decode(file.readAll(), path);
  if (!isPlaylistText(decoded.text)) return std::nullopt;
  if (!decoded.needsReview) return decoded.text;

  QDialog dialog(dialogParent(WindowId::playlist));
  dialog.setWindowTitle(QStringLiteral("Check playlist text"));
  auto* layout = new QVBoxLayout(&dialog);
  auto* explanation = new QLabel(QStringLiteral(
      "%1 uses older text encoding. Choose the version that shows the names correctly. "
      "The original playlist file stays unchanged.").arg(QFileInfo(path).fileName()), &dialog);
  explanation->setWordWrap(true);
  layout->addWidget(explanation);
  auto* choices = new QComboBox(&dialog);
  choices->setAccessibleName(QStringLiteral("Read text as"));
  layout->addWidget(new QLabel(QStringLiteral("Read text as"), &dialog));
  layout->addWidget(choices);
  auto* preview = new QPlainTextEdit(&dialog);
  preview->setReadOnly(true);
  preview->setAccessibleName(QStringLiteral("Playlist text preview"));
  layout->addWidget(preview);
  for (const auto& alternative : decoded.alternatives) choices->addItem(alternative.encoding);
  const auto update = [choices, preview, &decoded]() {
    const int i = choices->currentIndex();
    if (i >= 0 && i < decoded.alternatives.size())
      preview->setPlainText(decoded.alternatives[i].text);
  };
  connect(choices, &QComboBox::currentIndexChanged, &dialog, update);
  choices->setCurrentText(decoded.encoding);
  update();
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Use this text"));
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  dialog.resize(520, 380);
  if (dialog.exec() != QDialog::Accepted) return std::nullopt;
  return decoded.alternatives[choices->currentIndex()].text;
}

}  // namespace aoide
