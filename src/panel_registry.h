#pragma once

#include "docking.h"
#include "settings.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QSize>
#include <QString>
#include <QStringList>
#include <array>
#include <cstddef>
#include <optional>

class HostWindow;

namespace tramp {

/// One panel's fixed facts: everything about it that the rest of the app used
/// to rediscover in a `switch (WindowId)` of its own. What it is called on its
/// title bar, in the settings file, in a chrome dump and on the command line;
/// how big its canvas is; whether it docks, and where it parks when it would
/// open on top of main; and which `WindowFrame` inside a settings record and a
/// live layout is its.
///
/// Nothing here changes while the app runs — the position, size and visibility
/// a listener can move all live in the panel's `WindowFrame`, which the layout
/// owns. This is only the table that says which frame that is.
struct PanelSpec {
  WindowId id = WindowId::main;
  /// What the panel's own title bar says.
  QString title;
  /// What the settings file calls the panel. A file-format name: changing one
  /// orphans the layout every listener has saved.
  QString persistKey;
  /// The file stem `--dump-chrome` writes the panel's picture under, and the
  /// name the fidelity baseline holds it by.
  QString dumpName;
  /// What the benches print, and what `--bench-drag` accepts. The first is the
  /// one printed.
  QStringList commandNames;
  /// The logical canvas before zoom, and before windowshade.
  QSize logicalSize;
  /// Whether the listener can resize the panel, which is what makes the width
  /// and height on its frame mean anything. Only the playlist's do.
  bool resizable = false;
  /// Whether the panel belongs to the docked cluster: it snaps to its
  /// neighbours, keeps dock edges, and travels when main is dragged. Settings
  /// and About float free of all of that.
  bool docks = false;
  /// Which side of main the panel is pushed out to when it would otherwise
  /// open stacked on top of it. Only read for a panel that [docks].
  DockSide parkSide = DockSide::right;
  /// Which column of the first-run arrangement the panel is seeded into.
  int seedColumn = 0;
  /// This panel's frame inside a settings record...
  WindowFrame TrampSettings::*settingsFrame = &TrampSettings::main;
  /// ...and inside a live layout.
  WindowFrame DockLayout::*layoutFrame = &DockLayout::main;
};

/// Where [id] sits in every table keyed by a panel.
inline constexpr std::size_t panelIndex(WindowId id) { return static_cast<std::size_t>(id); }

/// Every panel, in `WindowId` order. The one table a sixth panel is added to.
inline const std::array<PanelSpec, kPanelCount>& panelSpecs() {
  static const std::array<PanelSpec, kPanelCount> specs = {{
      PanelSpec{
          WindowId::main,
          QStringLiteral("Tramp"),
          QStringLiteral("main"),
          QStringLiteral("main_player_window"),
          {QStringLiteral("main")},
          kMainPlayer,
          false,
          true,
          DockSide::right,
          0,
          &TrampSettings::main,
          &DockLayout::main,
      },
      PanelSpec{
          WindowId::equalizer,
          QStringLiteral("Equalizer"),
          QStringLiteral("equalizer"),
          QStringLiteral("equalizer_window"),
          {QStringLiteral("eq"), QStringLiteral("equalizer")},
          kEqualizer,
          false,
          true,
          DockSide::bottom,
          0,
          &TrampSettings::equalizer,
          &DockLayout::equalizer,
      },
      PanelSpec{
          WindowId::playlist,
          QStringLiteral("Playlist"),
          QStringLiteral("playlist"),
          QStringLiteral("playlist_window"),
          {QStringLiteral("playlist"), QStringLiteral("pl")},
          kPlaylistDefault,
          true,
          true,
          DockSide::right,
          1,
          &TrampSettings::playlist,
          &DockLayout::playlist,
      },
      PanelSpec{
          WindowId::settings,
          QStringLiteral("Settings"),
          QStringLiteral("settings"),
          QStringLiteral("settings_window"),
          {QStringLiteral("settings")},
          kSettings,
          false,
          false,
          DockSide::right,
          1,
          &TrampSettings::settings,
          &DockLayout::settings,
      },
      PanelSpec{
          WindowId::about,
          QStringLiteral("About"),
          QStringLiteral("about"),
          QStringLiteral("about_window"),
          {QStringLiteral("about")},
          kAbout,
          false,
          false,
          DockSide::right,
          0,
          &TrampSettings::about,
          &DockLayout::about,
      },
  }};
  return specs;
}

inline const PanelSpec& panelSpec(WindowId id) { return panelSpecs()[panelIndex(id)]; }

/// The panel a name on the command line asks for, or nothing when it names no
/// panel.
inline std::optional<WindowId> panelForName(const QString& name) {
  const QString key = name.trimmed().toLower();
  for (const PanelSpec& panel : panelSpecs()) {
    if (panel.commandNames.contains(key) || panel.persistKey == key) return panel.id;
  }
  return std::nullopt;
}

/// The panel a settings file names, or nothing when it names none.
inline std::optional<WindowId> panelForPersistKey(const QString& key) {
  for (const PanelSpec& panel : panelSpecs()) {
    if (panel.persistKey == key) return panel.id;
  }
  return std::nullopt;
}

/// The live window behind each panel. A table rather than five named pointers,
/// so the app wiring and the session both reach a panel's window by id instead
/// of switching on it.
class PanelWindows {
 public:
  HostWindow* operator[](WindowId id) const { return windows_[panelIndex(id)]; }
  void set(WindowId id, HostWindow* window) { windows_[panelIndex(id)] = window; }
  void clear() { windows_.fill(nullptr); }

 private:
  std::array<HostWindow*, kPanelCount> windows_{};
};

/// Copy every panel's frame from a settings record into a live layout, or the
/// other way: persist writes the layout back. The table is the list of frames,
/// so a sixth panel is one more pointer pair, not another assignment.
inline void copyPanelFrames(DockLayout& dest, const TrampSettings& src) {
  for (const PanelSpec& panel : panelSpecs()) {
    dest.*panel.layoutFrame = src.*panel.settingsFrame;
  }
}

inline void copyPanelFrames(TrampSettings& dest, const DockLayout& src) {
  for (const PanelSpec& panel : panelSpecs()) {
    dest.*panel.settingsFrame = src.*panel.layoutFrame;
  }
}

}  // namespace tramp
