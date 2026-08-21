#include "session_view.h"

namespace tramp {

SessionView goldenDemoView() {
  SessionView v;
  v.goldenDemo = true;

  v.positionMs = 161000;
  v.durationMs = 347000;
  v.title = QStringLiteral("3. Velvet Static — Neon Boulevard (Extended Mix)");
  v.subtitle = QStringLiteral("COPPER RAIN EP · TRACK 3 OF 12");
  v.bitrate = QStringLiteral("192 kbps");
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
      {QStringLiteral("ANALOGUE GHOSTS"), 24, false, false},
      {QStringLiteral("COPPER RAIN EP"), 13, true, false},
      {QStringLiteral("NIGHTBUS CHOIR — LIVE"), 8, false, false},
  };
  // Thirteen rows totalling 65:34, which the default panel shows exactly.
  v.tracks = {
      {QStringLiteral("Cassette Mirage"), QStringLiteral("Low Orbit Lullaby"), QStringLiteral("4:12"), false, false},
      {QStringLiteral("The Brass Cassini"), QStringLiteral("Slow Dial"), QStringLiteral("3:38"), false, false},
      {QStringLiteral("Velvet Static"), QStringLiteral("Neon Boulevard (Extended Mix)"), QStringLiteral("5:47"), true, true},
      {QStringLiteral("Halogen Youth"), QStringLiteral("Parking Garage Sunset"), QStringLiteral("4:03"), false, false},
      {QStringLiteral("Moth & Marrow"), QStringLiteral("Analogue Ghosts"), QStringLiteral("6:21"), false, false},
      {QStringLiteral("Ruby Transit"), QStringLiteral("Bakelite Heart"), QStringLiteral("3:55"), false, false},
      {QStringLiteral("Slow Signal"), QStringLiteral("Copper Rain"), QStringLiteral("4:44"), false, false},
      {QStringLiteral("Aurora Kiosk"), QStringLiteral("Departure Lounge B"), QStringLiteral("5:09"), false, false},
      {QStringLiteral("Pale Antenna"), QStringLiteral("Tramp Theme (Demo)"), QStringLiteral("2:58"), false, false},
      {QStringLiteral("Nightbus Choir"), QStringLiteral("Fluorescent Hymn"), QStringLiteral("6:02"), false, false},
      {QStringLiteral("Second Cassette"), QStringLiteral("Static Blonde"), QStringLiteral("3:27"), false, false},
      {QStringLiteral("Velvet Static"), QStringLiteral("Neon Boulevard (Reprise)"), QStringLiteral("2:02"), false, false},
      {QStringLiteral("Long Wave Motel"), QStringLiteral("Untitled Sketch"), QStringLiteral("13:16"), false, false},
  };
  v.playingIndex = 2;
  v.playlistName = QStringLiteral("Copper Rain — Night Set.m3u8");
  v.playlistTotalMs = 3934000;
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

}  // namespace tramp
