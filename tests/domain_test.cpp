#include "collection.h"
#include "docking.h"
#include "equalizer.h"
#include "m3u.h"
#include "playback.h"
#include "player_engine.h"
#include "playlist.h"
#include "popup_anchor.h"
#include "spectrum.h"
#include "support_dir.h"
#include "track.h"
#include "transport.h"
#include "wav_reader.h"

#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QtEndian>
#include <QVariant>
#include <QVector>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

int gFails = 0;

void require(bool cond, const char* file, int line, const char* expr) {
  if (!cond) {
    std::fprintf(stderr, "FAIL %s:%d %s\n", file, line, expr);
    ++gFails;
  }
}

#define REQUIRE(cond) require(bool(cond), __FILE__, __LINE__, #cond)
#define REQUIRE_EQ(a, b)                                                                 \
  do {                                                                                   \
    const auto _va = (a);                                                                \
    const auto _vb = (b);                                                                \
    if (_va != _vb) {                                                                    \
      std::fprintf(stderr, "FAIL %s:%d %s != %s\n  left:  %s\n  right: %s\n", __FILE__,  \
                   __LINE__, #a, #b, qPrintable(QVariant::fromValue(_va).toString()),     \
                   qPrintable(QVariant::fromValue(_vb).toString()));                      \
      ++gFails;                                                                          \
    }                                                                                    \
  } while (0)

}  // namespace

using tramp::AudioLevels;
using tramp::EqualizerSettings;
using tramp::M3uCodec;
using tramp::NullEngine;
using tramp::PlaybackController;
using tramp::PlaylistController;
using tramp::PcmBuffer;
using tramp::RepeatMode;
using tramp::Spectrogram;
using tramp::SpectrumAnalyzer;
using tramp::SpectrumHold;
using tramp::Track;
using tramp::WavReader;
using tramp::buildEqualizerAf;
using tramp::nextIndex;
using tramp::previousIndex;
using tramp::resolveLinuxSupportPath;
using tramp::spectrumFrame;

int main() {
  {
    const QString raw = QStringLiteral(
        "#EXTM3U\n"
        "#EXTINF:221,Wire Garden - Static Hymn\n"
        "tracks/static.mp3\n"
        "# comment\n"
        "other.flac\n");
    const QString playlist = QDir(QStringLiteral("music/lists")).filePath(QStringLiteral("go.m3u"));
    const auto tracks = M3uCodec().parse(raw, playlist);
    REQUIRE(tracks.size() == 2);
    REQUIRE(tracks[0].path == QDir::cleanPath(QStringLiteral("music/lists/tracks/static.mp3")));
    REQUIRE(tracks[0].title == QStringLiteral("Static Hymn"));
    REQUIRE(tracks[0].artist == QStringLiteral("Wire Garden"));
    REQUIRE(tracks[0].durationMs == 221000);
    REQUIRE(tracks[1].path == QDir::cleanPath(QStringLiteral("music/lists/other.flac")));
  }

  {
    const QString albumDir =
        QDir::cleanPath(QStringLiteral("/mnt/share/Enigma/1990 - MCMXC a.D"));
    const QString playlistFile =
        QDir(albumDir).filePath(QStringLiteral("Enigma - M C M X C a. D.m3u"));
    const QString realTrack =
        QDir(albumDir).filePath(QStringLiteral("01 - The Voice Of Enigma.flac"));

    {
      const QString raw = QStringLiteral("#EXTM3U\n#EXTINF:141,Enigma - The Voice Of Enigma\n") +
                          QStringLiteral("\\\\eliranas\\NAS\\Media\\Music\\Enigma\\1990 - MCMXC "
                                         "a.D\\01 - The Voice Of Enigma.flac\n");
      M3uCodec codec([&](const QString& p) { return p == realTrack; });
      const auto tracks = codec.parse(raw, playlistFile);
      REQUIRE(tracks.size() == 1);
      REQUIRE(tracks[0].path == realTrack);
    }

    {
      const QString discTrack =
          QDir(QDir(albumDir).filePath(QStringLiteral("Disc 2")))
              .filePath(QStringLiteral("03 - Callas Went Away.flac"));
      const QString raw =
          QStringLiteral("#EXTM3U\n\\\\eliranas\\NAS\\Music\\Enigma\\1990 - MCMXC a.D\\Disc "
                         "2\\03 - Callas Went Away.flac\n");
      M3uCodec codec([&](const QString& p) { return p == discTrack; });
      const auto tracks = codec.parse(raw, playlistFile);
      REQUIRE(tracks.size() == 1);
      REQUIRE(tracks[0].path == discTrack);
    }

    {
      const QString stale = QStringLiteral(
          "/run/user/1000/kio-fuse-XkqMpT/Enigma/1990 - MCMXC a.D/01 - The Voice Of Enigma.flac");
      const QString raw = QStringLiteral("#EXTM3U\n") + stale + QLatin1Char('\n');
      M3uCodec codec([&](const QString& p) { return p == realTrack; });
      const auto tracks = codec.parse(raw, playlistFile);
      REQUIRE(tracks.size() == 1);
      REQUIRE(tracks[0].path == realTrack);
    }

    {
      const QString elsewhere =
          QStringLiteral("/music/singles/01 - The Voice Of Enigma.flac");
      const QString raw = QStringLiteral("#EXTM3U\n") + elsewhere + QLatin1Char('\n');
      M3uCodec codec([&](const QString& p) { return p == elsewhere || p == realTrack; });
      const auto tracks = codec.parse(raw, playlistFile);
      REQUIRE(tracks.size() == 1);
      REQUIRE(tracks[0].path == QDir::cleanPath(elsewhere));
    }

    {
      const QString raw = QStringLiteral("#EXTM3U\n#EXTINF:141,Enigma - The Voice Of Enigma\n") +
                          QStringLiteral("\\\\eliranas\\NAS\\Media\\gone.flac\n");
      M3uCodec codec([](const QString&) { return false; });
      const auto tracks = codec.parse(raw, playlistFile);
      REQUIRE(tracks.size() == 1);
      REQUIRE(tracks[0].path.contains(QStringLiteral("gone.flac")));
      REQUIRE(tracks[0].title == QStringLiteral("The Voice Of Enigma"));
    }
  }

  {
    Track t;
    t.path = QDir::cleanPath(QDir::current().filePath(QStringLiteral("a.mp3")));
    t.title = QStringLiteral("A");
    t.artist = QStringLiteral("X");
    t.durationMs = 10000;
    const QString out = M3uCodec().encode({t});
    REQUIRE(out.split(QLatin1Char('\n')).first() == QStringLiteral("#EXTM3U"));
    REQUIRE(out.contains(QStringLiteral("#EXTINF:10,X - A")));
    REQUIRE(out.contains(t.path));
  }

  {
    REQUIRE(buildEqualizerAf(EqualizerSettings::flat()).isEmpty());
    EqualizerSettings disabled;
    disabled.enabled = false;
    disabled.preamp = 3;
    disabled.gains[5] = 12;
    REQUIRE(buildEqualizerAf(disabled).isEmpty());

    EqualizerSettings enabledFlat = EqualizerSettings::flat();
    enabledFlat.enabled = true;
    const QString af = buildEqualizerAf(enabledFlat);
    REQUIRE(af.startsWith(QStringLiteral("lavfi=[volume=0dB,")));
    REQUIRE(af.contains(QStringLiteral("equalizer=f=60:t=o:w=1:g=0")));
    REQUIRE(af.contains(QStringLiteral("equalizer=f=16000:t=o:w=1:g=0")));
    REQUIRE(af.count(QStringLiteral("equalizer=")) == 10);

    EqualizerSettings pre;
    pre.enabled = true;
    pre.preamp = 3;
    REQUIRE(buildEqualizerAf(pre).startsWith(QStringLiteral("lavfi=[volume=3dB,")));

    EqualizerSettings band;
    band.enabled = true;
    band.gains[4] = 12;
    REQUIRE(buildEqualizerAf(band).contains(QStringLiteral("equalizer=f=1000:t=o:w=1:g=12")));

    EqualizerSettings mixed;
    mixed.enabled = true;
    mixed.preamp = -2.5;
    mixed.gains[0] = 5;
    mixed.gains[9] = -3;
    const QString mixedAf = buildEqualizerAf(mixed);
    REQUIRE(mixedAf.startsWith(QStringLiteral("lavfi=[volume=-2.5dB,")));
    REQUIRE(mixedAf.contains(QStringLiteral("equalizer=f=60:t=o:w=1:g=5")));
    REQUIRE(mixedAf.contains(QStringLiteral("equalizer=f=16000:t=o:w=1:g=-3")));
  }

  {
    const QString home = QStringLiteral("/home/listener");
    const QString share = home + QStringLiteral("/.local/share");
    const QString pinned = share + QStringLiteral("/com.tramp.tramp");
    const QString legacy = share + QStringLiteral("/tramp");
    auto only = [](const QStringList& existing) {
      return [existing](const QString& path) { return existing.contains(path); };
    };
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home},
                                     {QStringLiteral("XDG_DATA_HOME"), share}},
                                    only({pinned})) == pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({pinned})) == pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home},
                                     {QStringLiteral("XDG_DATA_HOME"), QStringLiteral("relative/share")}},
                                    only({pinned})) == pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home},
                                     {QStringLiteral("XDG_DATA_HOME"), QStringLiteral("/data/xdg")}},
                                    only({QStringLiteral("/data/xdg/com.tramp.tramp")})) ==
            QStringLiteral("/data/xdg/com.tramp.tramp"));
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({legacy})) == legacy);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({pinned, legacy})) ==
            pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({})) == pinned);
  }

  {
    REQUIRE(!nextIndex(2, 3, false, RepeatMode::off, {}).has_value());
    REQUIRE(nextIndex(2, 3, false, RepeatMode::all, {}).value() == 0);
    REQUIRE(nextIndex(0, 3, false, RepeatMode::off, {}).value() == 1);
    REQUIRE(!previousIndex(0, 3, false, RepeatMode::off, {}).has_value());
    REQUIRE(previousIndex(0, 3, false, RepeatMode::all, {}).value() == 2);
    const QVector<int> order{2, 0, 1};
    REQUIRE(nextIndex(2, 3, true, RepeatMode::off, order).value() == 0);
    REQUIRE(!nextIndex(1, 3, true, RepeatMode::off, order).has_value());
    REQUIRE(nextIndex(1, 3, true, RepeatMode::all, order).value() == 2);
    REQUIRE(previousIndex(2, 3, true, RepeatMode::all, order).value() == 1);
  }

  {
    PlaylistController pl;
    Track a;
    a.path = QStringLiteral("/a.mp3");
    Track b;
    b.path = QStringLiteral("/b.mp3");
    pl.addTracks({a});
    REQUIRE(pl.altered());
    pl.select(0);
    REQUIRE(pl.altered());
    const QString tmp = QDir::temp().filePath(QStringLiteral("tramp-domain-test.m3u"));
    REQUIRE(pl.savePlaylistFile(tmp));
    REQUIRE(!pl.altered());
    pl.addTracks({b});
    REQUIRE(pl.altered());
    pl.setTracks({a, b}, tmp);
    REQUIRE(!pl.altered());
    pl.restoreAlteredTracks({a}, tmp);
    REQUIRE(pl.altered());
    QFile::remove(tmp);
  }

  {
    REQUIRE(tramp::formatClock(161000) == QStringLiteral("2:41"));
    REQUIRE(tramp::formatClock(347000) == QStringLiteral("5:47"));
    REQUIRE(tramp::formatTotalTime((3 * 24 + 22) * 3600LL * 1000 + 40 * 60 * 1000) ==
            QStringLiteral("3 d 22 h"));
    REQUIRE(tramp::groupedInt(1284) == QStringLiteral("1,284"));
    REQUIRE(tramp::groupedInt(4096) == QStringLiteral("4,096"));
  }

  auto appendU16 = [](QByteArray& b, quint16 v) {
    char d[2];
    qToLittleEndian(v, d);
    b.append(d, 2);
  };
  auto appendU32 = [](QByteArray& b, quint32 v) {
    char d[4];
    qToLittleEndian(v, d);
    b.append(d, 4);
  };
  auto makePcm16Wav = [&](const QVector<qint16>& interleaved, int channels, int sampleRate) {
    const quint32 dataBytes = quint32(interleaved.size() * 2);
    QByteArray out;
    out.append("RIFF", 4);
    appendU32(out, 36 + dataBytes);
    out.append("WAVE", 4);
    out.append("fmt ", 4);
    appendU32(out, 16);
    appendU16(out, 1);
    appendU16(out, quint16(channels));
    appendU32(out, quint32(sampleRate));
    appendU32(out, quint32(sampleRate * channels * 2));
    appendU16(out, quint16(channels * 2));
    appendU16(out, 16);
    out.append("data", 4);
    appendU32(out, dataBytes);
    for (qint16 s : interleaved) appendU16(out, quint16(s));
    return out;
  };
  auto argmax = [](const std::array<double, AudioLevels::kBandCount>& bands) {
    int best = 0;
    for (int i = 1; i < AudioLevels::kBandCount; ++i) {
      if (bands[size_t(i)] > bands[size_t(best)]) best = i;
    }
    return best;
  };

  {
    const QByteArray wav = makePcm16Wav({0, 16384, -16384}, 1, 44100);
    const PcmBuffer pcm = WavReader().read(wav);
    REQUIRE(pcm.sampleRateHz == 44100);
    REQUIRE(pcm.samples.size() == 3);
    REQUIRE(qAbs(pcm.samples[0]) < 1e-9);
    REQUIRE(qAbs(pcm.samples[1] - 0.5) < 1e-4);
    REQUIRE(qAbs(pcm.samples[2] + 0.5) < 1e-4);
  }

  {
    const QByteArray wav = makePcm16Wav({32767, 0}, 2, 48000);
    const PcmBuffer pcm = WavReader().read(wav);
    REQUIRE(pcm.sampleRateHz == 48000);
    REQUIRE(pcm.samples.size() == 1);
    REQUIRE(qAbs(pcm.samples[0] - 0.5) < 1e-3);
  }

  {
    constexpr int sampleRate = 44100;
    QVector<double> samples(4096, 0.0);
    samples[512] = 1.0;
    const Spectrogram spec = SpectrumAnalyzer().analyzeMonoPcm(samples, sampleRate);
    REQUIRE(!spec.frames.isEmpty());
    const AudioLevels frame = spec.levelsAt(0);
    REQUIRE(!frame.synthetic);
    int lit = 0;
    for (double b : frame.bands) {
      if (b > 0.05) ++lit;
    }
    REQUIRE(lit >= 8);
  }

  {
    constexpr int sampleRate = 44100;
    QVector<double> samples(sampleRate);
    for (int i = 0; i < samples.size(); ++i) {
      samples[i] = 0.5 * std::sin(2.0 * 3.14159265358979323846 * 1000.0 * double(i) / sampleRate);
    }
    const Spectrogram spec = SpectrumAnalyzer().analyzeMonoPcm(samples, sampleRate);
    const AudioLevels frame = spec.levelsAt(500);
    REQUIRE(!frame.synthetic);
    const int peakIndex = argmax(frame.bands);
    const double peakHz = SpectrumAnalyzer::bandCenterHz(peakIndex, sampleRate);
    REQUIRE(peakHz >= 500.0);
    REQUIRE(peakHz <= 2000.0);
    REQUIRE(frame.bands[size_t(peakIndex)] > 0.3);
  }

  {
    // Every log bar must own a real FFT range: a sine at the bar's center
    // frequency has to light that bar. 1024-point FFTs at 44.1/48 kHz collapse
    // the first two 40 Hz-up log edges onto the same bin, so bars 0–1 stayed
    // dark and bar 2 ate all the bass.
    constexpr double kPi = 3.14159265358979323846;
    const int rates[] = {44100, 48000};
    for (int sampleRate : rates) {
      QVector<double> samples(sampleRate);
      for (int band = 0; band < AudioLevels::kBandCount; ++band) {
        const double hz = SpectrumAnalyzer::bandCenterHz(band, sampleRate);
        for (int i = 0; i < samples.size(); ++i) {
          samples[i] = 0.5 * std::sin(2.0 * kPi * hz * double(i) / double(sampleRate));
        }
        const AudioLevels frame =
            SpectrumAnalyzer().analyzeMonoPcm(samples, sampleRate).levelsAt(200);
        const int peakIndex = argmax(frame.bands);
        REQUIRE(std::abs(peakIndex - band) <= 1);
        REQUIRE(frame.bands[size_t(band)] > 0.3);
      }
    }
  }

  {
    const Spectrogram spec = SpectrumAnalyzer().analyzeMonoPcm(QVector<double>(2048, 0.0), 44100);
    const AudioLevels frame = spec.levelsAt(0);
    REQUIRE(!frame.synthetic);
    bool quiet = true;
    for (double b : frame.bands) {
      if (b >= 0.01) quiet = false;
    }
    REQUIRE(quiet);
  }

  {
    Spectrogram spec;
    spec.framesPerSecond = 10;
    spec.frames = {std::array<double, AudioLevels::kBandCount>{},
                   std::array<double, AudioLevels::kBandCount>{}};
    spec.frames[0].fill(0.1);
    spec.frames[1].fill(0.9);
    REQUIRE(qAbs(spec.levelsAt(0).bands[0] - 0.1) < 1e-12);
    REQUIRE(qAbs(spec.levelsAt(100).bands[0] - 0.9) < 1e-12);
  }

  {
    constexpr int sampleRate = 44100;
    QVector<double> samples(8192);
    for (int i = 0; i < samples.size(); ++i) {
      samples[i] = 0.4 * std::sin(2.0 * 3.14159265358979323846 * 440.0 * double(i) / sampleRate);
    }
    SpectrumAnalyzer analyzer([samples](const QString&) {
      return PcmBuffer{samples, sampleRate};
    });
    const Spectrogram spec = analyzer.load(QStringLiteral("fixture://tone.wav"));
    const AudioLevels paused = spectrumFrame(spec, false, 100);
    REQUIRE(!paused.synthetic);
    bool pausedQuiet = true;
    for (double b : paused.bands) {
      if (b != 0.0) pausedQuiet = false;
    }
    REQUIRE(pausedQuiet);
    const AudioLevels live = spectrumFrame(spec, true, 100);
    REQUIRE(!live.synthetic);
    bool lit = false;
    for (double b : live.bands) {
      if (b > 0.05) lit = true;
    }
    REQUIRE(lit);
  }

  {
    SpectrumAnalyzer analyzer([](const QString&) -> PcmBuffer {
      throw std::runtime_error("decode failed");
    });
    const Spectrogram spec = analyzer.load(QStringLiteral("missing.wav"));
    const AudioLevels frame = spec.levelsAt(0);
    REQUIRE(!frame.synthetic);
    bool quiet = true;
    for (double b : frame.bands) {
      if (b != 0.0) quiet = false;
    }
    REQUIRE(quiet);
  }

  {
    SpectrumHold hold;
    AudioLevels frame;
    frame.bands[0] = 1.0;
    hold.apply(frame);
    REQUIRE(qAbs(hold.bars[0] - 1.0) < 1e-12);
    REQUIRE(qAbs(hold.peaks[0] - 1.0) < 1e-12);
    hold.apply(AudioLevels::silent());
    REQUIRE(qAbs(hold.bars[0] - 0.86) < 1e-12);
    REQUIRE(qAbs(hold.peaks[0] - 0.97) < 1e-12);
  }

  {
    const QRect cog(100, 200, 26, 26);
    const QSize menu(180, 120);
    REQUIRE_EQ(tramp::popupMenuPos(cog, menu, tramp::PopupAnchor::belowLeft), QPoint(100, 226));
    REQUIRE_EQ(tramp::popupMenuPos(cog, menu, tramp::PopupAnchor::aboveLeft), QPoint(100, 80));
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 348, {}, {}};
    layout.playlist = {true, false, 0, 696, {}, {}};
    layout.dockEdges = {{tramp::WindowId::main, tramp::WindowId::equalizer, tramp::DockSide::bottom}};
    tramp::DockingCoordinator dock(layout);
    dock.move(tramp::WindowId::main, QPointF(40, 20), false, false);
    REQUIRE_EQ(dock.layout().main.left, 40.0);
    REQUIRE_EQ(dock.layout().main.top, 20.0);
    REQUIRE_EQ(dock.layout().equalizer.left, 0.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 348.0);
    REQUIRE_EQ(dock.layout().playlist.left, 0.0);
    REQUIRE_EQ(dock.layout().playlist.top, 696.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 100, 100, {}, {}};
    layout.equalizer = {true, false, 100, 448, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::main, QPointF(130, 110), false, false);
    REQUIRE_EQ(dock.layout().equalizer.left, 100.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 448.0);
  }

  {
    REQUIRE_EQ(tramp::collectionHighlightPath(QStringLiteral("/music/set.m3u"), QString()),
               QDir::cleanPath(QStringLiteral("/music/set.m3u")));
    REQUIRE_EQ(tramp::collectionHighlightPath(QString(), QStringLiteral("/music/other.m3u")),
               QStringLiteral("/music/other.m3u"));
    REQUIRE_EQ(tramp::collectionHighlightPath(QString(), QString()), QString());
  }

  {
    // MpvEngine only reports pause via a later property event. Pause must not
    // wait for that callback or the button looks stuck.
    class QuietEngine : public NullEngine {
     public:
      void play() override { played = true; }
      void pause() override { paused = true; }
      bool played = false;
      bool paused = false;
    };
    PlaylistController playlist;
    Track track;
    track.path = QStringLiteral("/tmp/quiet.mp3");
    track.title = QStringLiteral("Quiet");
    playlist.setTracks({track});
    QuietEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    REQUIRE(playback.playing());
    REQUIRE(engine.played);
    playback.playPause();
    REQUIRE(!playback.playing());
    REQUIRE(playback.paused());
    REQUIRE(engine.paused);
  }

  {
    // loadfile reports pause=yes until the file is ready. That echo must not
    // undo the optimistic play/pause the chrome already showed.
    class LagEngine : public NullEngine {
     public:
      void play() override {}
      void pause() override {}
      void fire(bool playing) {
        if (onPlaying) onPlaying(playing);
      }
    };
    PlaylistController playlist;
    Track track;
    track.path = QStringLiteral("/tmp/lag.mp3");
    playlist.setTracks({track});
    LagEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    REQUIRE(playback.playing());
    engine.fire(false);
    REQUIRE(playback.playing());
    playback.playPause();
    REQUIRE(!playback.playing());
    engine.fire(true);
    REQUIRE(!playback.playing());
    REQUIRE(playback.paused());
  }

  {
    REQUIRE(tramp::samePlaylistFile(QStringLiteral("/music/set.m3u"),
                                    QStringLiteral("/music/./set.m3u")));
    REQUIRE(!tramp::samePlaylistFile(QStringLiteral("/music/a.m3u"),
                                     QStringLiteral("/music/b.m3u")));
    REQUIRE(!tramp::samePlaylistFile(QString(), QStringLiteral("/music/a.m3u")));
  }

  {
    class WatchEngine : public NullEngine {
     public:
      void play() override { stopped = false; }
      void stop() override {
        stopped = true;
        NullEngine::stop();
      }
      bool stopped = false;
    };
    PlaylistController playlist;
    Track a;
    a.path = QStringLiteral("/tmp/keep-playing.mp3");
    Track b;
    b.path = QStringLiteral("/tmp/other.mp3");
    playlist.setTracks({a, b}, QStringLiteral("/tmp/current.m3u"));
    WatchEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    REQUIRE(playback.playing());
    playlist.select(1);
    REQUIRE(playback.playing());
    REQUIRE(!engine.stopped);
    Track c;
    c.path = QStringLiteral("/tmp/from-another-list.mp3");
    playlist.setTracks({c}, QStringLiteral("/tmp/other.m3u"));
    REQUIRE(playback.playing());
    REQUIRE(!engine.stopped);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 100, 80, {}, {}};
    layout.equalizer = {true, false, 100, 80, {}, {}};
    layout.playlist = {true, false, 102, 82, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.nudgeOffMainIfStacked(tramp::WindowId::equalizer);
    dock.nudgeOffMainIfStacked(tramp::WindowId::playlist);
    REQUIRE_EQ(dock.layout().equalizer.left, 100.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 80.0 + 348.0);
    REQUIRE_EQ(dock.layout().playlist.left, 100.0 + 825.0);
    REQUIRE_EQ(dock.layout().playlist.top, 80.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 40, 20, {}, {}};
    layout.equalizer = {true, false, 40, 368, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.nudgeOffMainIfStacked(tramp::WindowId::equalizer);
    REQUIRE_EQ(dock.layout().equalizer.left, 40.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 368.0);
  }

  {
    tramp::DockLayout layout;
    layout.main.visible = false;
    layout.equalizer.visible = false;
    layout.playlist.visible = false;
    tramp::DockingCoordinator dock(layout);
    dock.setVisible(tramp::WindowId::main, false);
    REQUIRE(dock.layout().main.visible == false);
    dock.ensureMainVisible();
    REQUIRE(dock.layout().main.visible);
    REQUIRE(dock.layout().equalizer.visible);
    REQUIRE(dock.layout().playlist.visible);
    dock.setVisible(tramp::WindowId::main, false);
    REQUIRE(dock.layout().main.visible);
    dock.setVisible(tramp::WindowId::equalizer, false);
    dock.ensureMainVisible();
    REQUIRE(!dock.layout().equalizer.visible);
  }

  if (gFails != 0) {
    std::fprintf(stderr, "%d assertion(s) failed\n", gFails);
    return 1;
  }
  std::puts("domain_test: ok");
  return 0;
}
