#include "session_view.h"

#include "chrome_layout.h"

namespace aoide {
namespace {

/// Two token sets paint alike when the look they were resolved from is alike:
/// `ChromeTokens::from` derives all sixty-odd colours from palette and
/// materials, and painters also read the resolved radii, so comparing those
/// inputs settles every output.
bool sameLook(const ChromeTokens& a, const ChromeTokens& b) {
  const LookPalette& p = a.palette;
  const LookPalette& q = b.palette;
  const LookMaterials& m = a.materials;
  const LookMaterials& n = b.materials;
  const LookRadii& ra = a.radii;
  const LookRadii& rb = b.radii;
  return a.id == b.id && a.chromeFamily == b.chromeFamily && a.lcdFamily == b.lcdFamily &&
         p.shellHi == q.shellHi && p.shell == q.shell && p.shellMid == q.shellMid &&
         p.shellLo == q.shellLo && p.shellDeep == q.shellDeep && p.ink == q.ink &&
         p.inkDim == q.inkDim && p.inkFaint == q.inkFaint && p.phos == q.phos &&
         p.phosHot == q.phosHot && p.phosDim == q.phosDim && p.phosDeep == q.phosDeep &&
         p.accent == q.accent && p.accentDim == q.accentDim && p.well == q.well &&
         m.bevelLightOpacity == n.bevelLightOpacity &&
         m.bevelSoftOpacity == n.bevelSoftOpacity && m.spectrumStops == n.spectrumStops &&
         m.railStops == n.railStops && ra.window == rb.window && ra.surface == rb.surface &&
         ra.button == rb.button;
}

bool sameCurve(const EqualizerSettings& a, const EqualizerSettings& b) {
  return a.enabled == b.enabled && a.auto_ == b.auto_ && a.preamp == b.preamp &&
         a.gains == b.gains && a.presetName == b.presetName;
}

bool sameCatalog(const QVector<SkinCatalogEntry>& a, const QVector<SkinCatalogEntry>& b) {
  if (a.size() != b.size()) return false;
  for (int i = 0; i < a.size(); ++i) {
    if (a[i].id != b[i].id || a[i].name != b[i].name || a[i].author != b[i].author ||
        a[i].previewPath != b[i].previewPath || a[i].canRemove != b[i].canRemove) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool titleMarqueeRunning(const SessionView& view) {
  return view.scrollTitle && !view.goldenDemo && view.titleScrollMs > kMarqueeHoldMs;
}

bool paintsSame(WindowId id, const SessionView& a, const SessionView& b) {
  // Destructured rather than read field by field, because the compiler counts
  // the names: a field added to the snapshot arrives here as a build error
  // instead of as a panel quietly keeping a raster that has stopped being true.
  // Whoever adds one says which panels repaint for it — listing it under all
  // six is the answer that cannot be wrong.
  const auto& [goldenDemo, eqOn, plOn, skinsOn, trackInfoEnabled, showElapsed, positionMs, durationMs, title, subtitle,
               bitrate, sampleRate, channels, formatChip, volume, muted, forceMono, playing,
               paused, shuffle, repeat, zoomPercent, zoomInEnabled, zoomOutEnabled, spectrum,
               spectrumPeaks, eq, tracks, selectedIndices, playingIndex, trackScroll, collection,
               collectionSelected, collectionWidth, collectionCollapsed, playlistName,
               playlistAltered, playlistTotalMs, playlistTrackCount, playlistRefreshEnabled,
               playlistRefreshing, settingsTab, resumeLastSession, confirmBeforeQuit, scrollTitle,
               titleScrollMs, minimizeHidesSecondaries, dockSnap, aboutPlaylists, aboutTracks,
               aboutTimeMs, aboutSpins, aboutMeasured, look, skins, activeSkinId, skinsError,
               skinsScroll, spectrumUnmeasured, noAudioEngine, persistWriteFailed,
               audioDeviceLabel, audioExclusive] = a;

  // No painter reads these three. The playlist rows carry their own `selected`
  // flag, so `selectedIndices` is the session's copy; `aboutMeasured` is read
  // by nobody at all; and nothing ever writes `collectionSelected`. A panel
  // that starts painting one has to move it into that panel's group below.
  (void)selectedIndices;
  (void)aboutMeasured;
  (void)collectionSelected;

  // Main paints from the marquee clock, but only through `titleMarqueeRunning`
  // below. Comparing the raw value would cost main its raster on every snapshot
  // for as long as a track is loaded, which is the very cost this is removing:
  // past the hold the moving line is painted on the live pass, not into the
  // raster, so the frames are not main's to keep.
  (void)titleScrollMs;

  // The shell, the title bar and its buttons belong to all six, so a skin
  // change or a zoom step is the one thing that does re-rasterise everything.
  if (zoomPercent != b.zoomPercent || goldenDemo != b.goldenDemo || !sameLook(look, b.look)) {
    return false;
  }

  switch (id) {
    case WindowId::main:
      // The display well, the meta row, and the volume and transport clusters —
      // and the zoom buttons, which are here rather than in the shared block
      // above because main is the only panel whose title bar carries them:
      // `TitleChromeLayout::forWindow` gives the other five minimize and close
      // and nothing else, so a step going grey is invisible on them.
      // `mainEmptyTitle` is the empty-list swap: title can stay `No track`
      // while the list gains a row, and the chassis would otherwise keep
      // "Drop files to play".
      return zoomInEnabled == b.zoomInEnabled && zoomOutEnabled == b.zoomOutEnabled &&
             skinsOn == b.skinsOn && trackInfoEnabled == b.trackInfoEnabled &&
             showElapsed == b.showElapsed && positionMs == b.positionMs &&
             durationMs == b.durationMs && title == b.title &&
             mainEmptyTitle(a) == mainEmptyTitle(b) && subtitle == b.subtitle &&
             bitrate == b.bitrate && sampleRate == b.sampleRate && channels == b.channels &&
             formatChip == b.formatChip && volume == b.volume && muted == b.muted &&
             forceMono == b.forceMono && playing == b.playing && paused == b.paused &&
             shuffle == b.shuffle && repeat == b.repeat && eqOn == b.eqOn && plOn == b.plOn &&
             scrollTitle == b.scrollTitle && spectrum == b.spectrum &&
             spectrumPeaks == b.spectrumPeaks &&
             spectrumUnmeasured == b.spectrumUnmeasured &&
             noAudioEngine == b.noAudioEngine &&
             titleMarqueeRunning(a) == titleMarqueeRunning(b);
    case WindowId::equalizer:
      return sameCurve(eq, b.eq);
    case WindowId::playlist:
      // The collection column is compared row by row rather than gated on a
      // revision counter. A counter over the collection's mutations misses the
      // three things this column actually paints from — a rename, a change of
      // selection, and the validation pass that disables a row whose file has
      // gone — while firing for a duration total the column never shows. The
      // rows are one per playlist file, so comparing them outright is cheap;
      // it is `tracks` below that is long, and no counter speaks for that.
      return collection == b.collection && collectionWidth == b.collectionWidth &&
             collectionCollapsed == b.collectionCollapsed && tracks == b.tracks &&
             trackScroll == b.trackScroll && playingIndex == b.playingIndex &&
             playlistName == b.playlistName && playlistAltered == b.playlistAltered &&
             playlistTotalMs == b.playlistTotalMs &&
             playlistTrackCount == b.playlistTrackCount &&
             playlistRefreshEnabled == b.playlistRefreshEnabled &&
             playlistRefreshing == b.playlistRefreshing && playing == b.playing;
    case WindowId::settings:
      // `scrollTitle` is here as well as on main: the marquee runs on one panel
      // and its switch is painted on the other.
      return settingsTab == b.settingsTab && resumeLastSession == b.resumeLastSession &&
             confirmBeforeQuit == b.confirmBeforeQuit && scrollTitle == b.scrollTitle &&
             minimizeHidesSecondaries == b.minimizeHidesSecondaries && dockSnap == b.dockSnap &&
             persistWriteFailed == b.persistWriteFailed &&
             audioDeviceLabel == b.audioDeviceLabel && audioExclusive == b.audioExclusive;
    case WindowId::skins:
      return activeSkinId == b.activeSkinId && skinsScroll == b.skinsScroll &&
             skinsError == b.skinsError && sameCatalog(skins, b.skins);
    case WindowId::about:
      return aboutPlaylists == b.aboutPlaylists && aboutTracks == b.aboutTracks &&
             aboutTimeMs == b.aboutTimeMs && aboutSpins == b.aboutSpins;
  }
  return false;
}

SessionView goldenDemoView() {
  SessionView v;
  v.goldenDemo = true;

  // Real tracks, from the Chronoton catalogue. The screenshots this view feeds
  // are the ones on aoide.music and in the AppStream metainfo, and a store
  // listing showing invented band names is a listing showing music nobody can
  // buy. Durations are the released ones, so the footer totals below add up.
  v.positionMs = 112000;
  v.durationMs = 248000;
  v.title = QStringLiteral("1. Chronoton — Travelling Far In Short Strides");
  v.subtitle = QStringLiteral("PARTICLES EP · TRACK 1 OF 5");
  // Not "320 kbps", and the chip below is not "FLAC": aKeptRasterIsNeverOneThatWentStale
  // in host_window_move_test proves a changed field re-rasterises main by
  // assigning those two literals, and a demo already holding one makes that
  // mutation a no-op and fails the test.
  v.bitrate = QStringLiteral("256 kbps");
  v.sampleRate = QStringLiteral("44.1 kHz");
  v.channels = QStringLiteral("STEREO");
  v.formatChip = QStringLiteral("MP3");
  v.volume = 0.66;
  v.playing = true;
  v.shuffle = true;
  v.repeat = RepeatMode::all;
  v.spectrum = {0.26, 0.52, 0.71, 0.88, 0.64, 0.47, 0.58, 0.39, 0.31, 0.44,
                0.35, 0.24, 0.29, 0.19, 0.22, 0.14, 0.17, 0.10, 0.12, 0.07};
  v.spectrumPeaks = {0.44, 0.70, 0.88, 0.96, 0.80, 0.66, 0.74, 0.57, 0.52, 0.61,
                     0.55, 0.42, 0.47, 0.36, 0.40, 0.30, 0.33, 0.24, 0.27, 0.19};

  v.eq.enabled = true;
  v.eq.auto_ = false;
  v.eq.presetName = QStringLiteral("Late night");
  v.eq.preamp = 3.8;
  v.eq.gains = {6.2, 4.6, 1.0, -1.9, -0.5, 2.2, 3.4, 1.4, 0.0, 5.0};

  v.collectionWidth = 240;
  v.collection = {
      {QStringLiteral("CHRONOTON"), 13, true, false},
      {QStringLiteral("PARTICLES"), 5, false, false},
      {QStringLiteral("CHILDHOOD CHORDS"), 5, false, false},
  };
  // Thirteen rows totalling 46:58, which the default panel shows exactly: a row
  // the bottom edge halves belongs in the clamped dump, not this one. Order is
  // the Particles EP, then Childhood Chords, then the singles.
  v.tracks = {
      {QStringLiteral("Chronoton"), QStringLiteral("Travelling Far In Short Strides"), QStringLiteral("4:08"), true, true},
      {QStringLiteral("Chronoton"), QStringLiteral("All My Secrets Are Petals For You"), QStringLiteral("4:14"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("The Evolution Of Weakness"), QStringLiteral("3:34"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Puzzle Catcher - Ingame"), QStringLiteral("3:40"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Puzzle Catcher - Menu"), QStringLiteral("3:07"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("What's the Plan, Superman?"), QStringLiteral("3:00"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Stranded Between States of Matter (feat Wilsiac)"), QStringLiteral("3:44"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("The Whispers That Move Mountains"), QStringLiteral("2:56"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Light Years Away by ZAYAZ (Chronoton Cover)"), QStringLiteral("3:10"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("A Kingdom for Your Thoughts"), QStringLiteral("3:52"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Escaping Eridani"), QStringLiteral("3:57"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("Only In The Void We Find Perfection"), QStringLiteral("4:13"), false, false},
      {QStringLiteral("Chronoton"), QStringLiteral("In This Light You Look Like Me"), QStringLiteral("3:23"), false, false},
  };
  v.playingIndex = 0;
  v.trackInfoEnabled = true;
  v.playlistName = QStringLiteral("Chronoton.m3u8");
  v.playlistTotalMs = 2818000;
  v.playlistTrackCount = v.tracks.size();
  v.playlistRefreshEnabled = true;

  v.confirmBeforeQuit = true;
  v.minimizeHidesSecondaries = false;

  v.aboutPlaylists = 12;
  v.aboutTracks = 1284;
  v.aboutTimeMs = 338400000;
  v.aboutSpins = 4096;
  return v;
}

}  // namespace aoide
