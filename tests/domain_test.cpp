#include "chrome_hits.h"
#include "collection.h"
#include "duration_probe.h"
#include "native_file_dialog.h"
#include "persist.h"
#include "docking.h"
#include "equalizer.h"
#include "m3u.h"
#include "playback.h"
#include "player_engine.h"
#include "playlist.h"
#include "popup_anchor.h"
#include "session_view.h"
#include "spectrum.h"
#include "support_dir.h"
#include "wait_cursor.h"
#include "track.h"
#include "transport.h"
#include "wav_reader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTemporaryDir>
#include <QByteArray>
#include <QtEndian>
#include <QVariant>
#include <QVector>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    // Seek/volume wells are the whole track. A press must use the pointer, not the
    // well center — otherwise every click seeks to 50%.
    const QRect seekWell(100, 248, 200, 16);
    const QPoint click(150, 256);
    const QPoint engage = tramp::sliderPressPoint(seekWell, click);
    REQUIRE_EQ(engage, click);
    REQUIRE(std::abs(tramp::sliderFractionX(seekWell, engage.x()) - 0.25) < 1e-9);
    REQUIRE(std::abs(tramp::sliderFractionX(seekWell, seekWell.left() + seekWell.width() / 2) - 0.5) <
            1e-9);
    REQUIRE_EQ(qint64(tramp::sliderFractionX(seekWell, engage.x()) * 400000), qint64(100000));
  }

  {
    REQUIRE(!tramp::qtPluginPathNeedsSanitize(QByteArray()));
    REQUIRE(!tramp::qtPluginPathNeedsSanitize(QByteArray("/opt/qt/6.11.1/plugins")));
    REQUIRE(tramp::qtPluginPathNeedsSanitize(
        QByteArray("/tmp/.mount_cursorABC/usr/lib/qt5/plugins:/tmp/.mount_cursorABC/usr/lib64/qt5/"
                   "plugins")));
    REQUIRE(tramp::qtPluginPathNeedsSanitize(QByteArray("/usr/lib/qt4/plugins")));
    const auto audio = tramp::parseQtFileFilter(
        QStringLiteral("Audio (*.mp3 *.m4a *.aac *.flac *.wav *.ogg *.opus)"));
    REQUIRE(audio.size() == 1);
    REQUIRE_EQ(audio.front().name, QStringLiteral("Audio"));
    REQUIRE(audio.front().globs.contains(QStringLiteral("*.flac")));
    REQUIRE_EQ(tramp::fileUrisToLocalPaths(QStringList{QStringLiteral("file:///home/music/a.mp3")})
                   .front(),
               QStringLiteral("/home/music/a.mp3"));
    // xdg-desktop-portal FileChooser has OpenFile/SaveFile/SaveFiles only.
    // Folder pick is OpenFile + directory=true (portal v3), not OpenDirectory.
    const auto folder = tramp::portalFileChooserRequest(tramp::FilePickKind::openDirectory);
    REQUIRE_EQ(folder.method, QStringLiteral("OpenFile"));
    REQUIRE(folder.directory);
    REQUIRE(!folder.multiple);
    const auto files = tramp::portalFileChooserRequest(tramp::FilePickKind::openFiles);
    REQUIRE_EQ(files.method, QStringLiteral("OpenFile"));
    REQUIRE(files.multiple);
    REQUIRE(!files.directory);
    const auto save = tramp::portalFileChooserRequest(tramp::FilePickKind::saveFile);
    REQUIRE_EQ(save.method, QStringLiteral("SaveFile"));
    REQUIRE(!save.directory);
  }

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
    const QString pinned = share + QStringLiteral("/com.proximamagnifica.tramp");
    const QString ignored = share + QStringLiteral("/com.tramp.tramp");
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
                                    only({QStringLiteral("/data/xdg/com.proximamagnifica.tramp")})) ==
            QStringLiteral("/data/xdg/com.proximamagnifica.tramp"));
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({legacy})) == legacy);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({pinned, legacy})) ==
            pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({ignored})) == pinned);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({ignored, legacy})) ==
            legacy);
    REQUIRE(resolveLinuxSupportPath({{QStringLiteral("HOME"), home}}, only({pinned, ignored})) ==
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
    auto skipMiddle = [](int i) { return i != 1; };
    REQUIRE(tramp::nextPlayableIndex(0, 3, false, RepeatMode::off, {}, skipMiddle).value() == 2);
    REQUIRE(tramp::previousPlayableIndex(2, 3, false, RepeatMode::off, {}, skipMiddle).value() ==
            0);
    REQUIRE(!tramp::nextPlayableIndex(0, 2, false, RepeatMode::off, {}, [](int) { return false; })
                 .has_value());
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
    REQUIRE_EQ(dock.layout().equalizer.left, 40.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 368.0);
    REQUIRE_EQ(dock.layout().playlist.left, 40.0);
    REQUIRE_EQ(dock.layout().playlist.top, 716.0);
    REQUIRE_EQ(dock.layout().settings.left, 900.0);
    REQUIRE_EQ(dock.layout().settings.top, 60.0);
    REQUIRE_EQ(dock.layout().about.left, 900.0);
    REQUIRE_EQ(dock.layout().about.top, 500.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 100, 100, {}, {}};
    layout.equalizer = {true, false, 100, 448, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::main, QPointF(130, 110), false, false);
    REQUIRE_EQ(dock.layout().equalizer.left, 130.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 458.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 348, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.move(tramp::WindowId::equalizer, QPointF(40, 20), false, false);
    REQUIRE_EQ(dock.layout().main.left, 0.0);
    REQUIRE_EQ(dock.layout().main.top, 0.0);
    REQUIRE_EQ(dock.layout().equalizer.left, 40.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 20.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 358, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::equalizer, QPointF(0, 358), false, true);
    REQUIRE_EQ(dock.layout().equalizer.left, 0.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 348.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 358, {}, {}};
    layout.playlist = {true, false, 830, 0, 1073.0, 820.0};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::equalizer, QPointF(0, 358), false, true);
    REQUIRE_EQ(dock.layout().equalizer.top, 348.0);
    REQUIRE_EQ(dock.layout().equalizer.left, 5.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 8, 358, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::equalizer, QPointF(8, 358), false, true);
    REQUIRE_EQ(dock.layout().equalizer.left, 0.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 348.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.playlist = {true, false, 0, 358, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::playlist, QPointF(0, 358), false, true);
    REQUIRE_EQ(dock.layout().playlist.left, 0.0);
    REQUIRE_EQ(dock.layout().playlist.top, 348.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.playlist = {true, false, 835, 0, {}, {}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::playlist, QPointF(835, 0), false, true);
    REQUIRE_EQ(dock.layout().playlist.left, 825.0);
    REQUIRE_EQ(dock.layout().playlist.top, 0.0);
  }

  {
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 348, {}, {}};
    layout.playlist = {true, false, 835, 10, 1073.0, 500.0};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::playlist, QPointF(835, 10), false, true);
    REQUIRE_EQ(dock.layout().playlist.left, 825.0);
    REQUIRE_EQ(dock.layout().playlist.top, 0.0);
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
    // MpvEngine reports a loadfile failure synchronously from open(). playIndex
    // used to set playing_ = true immediately afterwards, so a file that never
    // opened still read as playing and the chrome showed the pause face.
    class RefusingEngine : public NullEngine {
     public:
      void open(const Track&) override {
        if (onError) onError(QStringLiteral("cannot open"));
      }
      void play() override { played = true; }
      bool played = false;
    };
    PlaylistController playlist;
    Track track;
    track.path = QStringLiteral("/tmp/gone.mp3");
    playlist.setTracks({track});
    RefusingEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    REQUIRE(!playback.playing());
    REQUIRE(!engine.played);
    REQUIRE_EQ(playback.failureMessage(), QStringLiteral("cannot open"));
  }

  {
    // A build with no audio backend must say so. Reporting playback of silence
    // is how a Windows package with no engine looked healthy.
    PlaylistController playlist;
    Track track;
    track.path = QStringLiteral("/tmp/quiet.flac");
    playlist.setTracks({track});
    tramp::MissingAudioEngine engine(QStringLiteral("no audio engine in this build"));
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    REQUIRE(!playback.playing());
    REQUIRE_EQ(playback.failureMessage(), QStringLiteral("no audio engine in this build"));
  }

  {
    // A data chunk size past INT_MAX used to become a negative int, slip through
    // the bounds check, and walk `offset` backwards into a read before the
    // buffer. mpv renders a whole track to PCM for the spectrum, so any track
    // past roughly 3.4 hours produces exactly this header.
    QByteArray wav(64, 0);
    std::memcpy(wav.data(), "RIFF", 4);
    std::memcpy(wav.data() + 8, "WAVE", 4);
    std::memcpy(wav.data() + 12, "fmt ", 4);
    qToLittleEndian(quint32(16), reinterpret_cast<uchar*>(wav.data() + 16));
    qToLittleEndian(quint16(1), reinterpret_cast<uchar*>(wav.data() + 20));
    qToLittleEndian(quint16(2), reinterpret_cast<uchar*>(wav.data() + 22));
    qToLittleEndian(quint32(44100), reinterpret_cast<uchar*>(wav.data() + 24));
    qToLittleEndian(quint16(16), reinterpret_cast<uchar*>(wav.data() + 34));
    std::memcpy(wav.data() + 36, "data", 4);
    qToLittleEndian(quint32(0x80000000u), reinterpret_cast<uchar*>(wav.data() + 40));
    bool refused = false;
    try {
      WavReader().read(wav);
    } catch (const std::exception&) {
      refused = true;
    }
    REQUIRE(refused);

    // A chunk size of zero must not spin forever either.
    QByteArray stuck = wav;
    qToLittleEndian(quint32(0), reinterpret_cast<uchar*>(stuck.data() + 16));
    bool refusedZero = false;
    try {
      WavReader().read(stuck);
    } catch (const std::exception&) {
      refusedZero = true;
    }
    REQUIRE(refusedZero);
  }

  {
    // A save that did not land must not report success, and must leave the
    // playlist altered so the discard prompt still fires. Clearing the flag on an
    // unchecked write is how unsaved edits disappeared without a word.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    PlaylistController playlist;
    Track a;
    a.path = QStringLiteral("/music/a.mp3");
    Track b;
    b.path = QStringLiteral("/music/b.mp3");
    playlist.setTracks({a, b});
    REQUIRE(!playlist.altered());
    playlist.removeAt(1);
    REQUIRE(playlist.altered());

    // Parent directory does not exist, so the write cannot land.
    const QString target =
        QDir(tmp.path()).filePath(QStringLiteral("no-such-dir/out.m3u"));
    REQUIRE(!playlist.savePlaylistFile(target, M3uCodec{}));
    REQUIRE(playlist.altered());
    REQUIRE(!QFile::exists(target));

    // And a save that does land clears it.
    const QString good = QDir(tmp.path()).filePath(QStringLiteral("out.m3u"));
    REQUIRE(playlist.savePlaylistFile(good, M3uCodec{}));
    REQUIRE(!playlist.altered());
    REQUIRE(QFile::exists(good));
  }

  {
    // Persistence must never destroy what it cannot replace. writeObject used to
    // truncate the target before writing, so an interrupted or failed write left
    // an empty file — which then read back as defaults and got persisted over
    // whatever had survived.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    tramp::SupportStore store(tmp.path());
    tramp::TrampSettings first;
    first.zoomPercent = 150;
    REQUIRE(store.writeSettings(first));
    REQUIRE_EQ(store.readSettings().zoomPercent, 150);
    REQUIRE(!QFile::exists(tmp.filePath(QStringLiteral("settings.json.tmp"))));

    // An unwritable directory must fail loudly and leave the old file intact.
    REQUIRE(QFile::setPermissions(tmp.path(), QFile::ReadOwner | QFile::ExeOwner));
    tramp::TrampSettings second;
    second.zoomPercent = 50;
    const bool wrote = store.writeSettings(second);
    REQUIRE(QFile::setPermissions(
        tmp.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    REQUIRE(!wrote);
    REQUIRE_EQ(store.readSettings().zoomPercent, 150);
  }

  {
    // A corrupt state file must not vanish into defaults. Keep the bytes aside so
    // the listener's collection can be recovered rather than overwritten.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString path = QDir(tmp.path()).filePath(QStringLiteral("settings.json"));
    QFile broken(path);
    REQUIRE(broken.open(QIODevice::WriteOnly));
    broken.write(QByteArray("{ this is not json"));
    broken.close();
    tramp::SupportStore store(tmp.path());
    REQUIRE_EQ(store.readSettings().zoomPercent, tramp::TrampSettings{}.zoomPercent);
    REQUIRE(QFile::exists(path + QStringLiteral(".corrupt")));
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
      void open(const Track& track) override { opened = track.path; }
      void play() override { stopped = false; }
      void stop() override {
        stopped = true;
        NullEngine::stop();
      }
      bool stopped = false;
      QString opened;
    };
    PlaylistController playlist;
    Track a;
    a.path = QStringLiteral("/tmp/keep-playing.mp3");
    a.title = QStringLiteral("Keep Playing");
    a.artist = QStringLiteral("Wire Garden");
    a.album = QStringLiteral("Copper Rain EP");
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
    c.title = QStringLiteral("From Another List");
    playlist.setTracks({c}, QStringLiteral("/tmp/other.m3u"));
    playback.onPlaylistChanged();
    REQUIRE(playback.playing());
    REQUIRE(!engine.stopped);
    REQUIRE_EQ(engine.opened, a.path);
    const auto nowPlaying = playback.currentTrack();
    REQUIRE(nowPlaying.has_value());
    REQUIRE_EQ(nowPlaying->path, a.path);
    REQUIRE_EQ(nowPlaying->title, a.title);
    REQUIRE_EQ(nowPlaying->artist, a.artist);
    REQUIRE(!playback.playingIndex().has_value());
    playback.playPause();
    REQUIRE(!playback.playing());
    REQUIRE(playback.paused());
    REQUIRE_EQ(playback.currentTrack()->path, a.path);
    playback.playIndex(0);
    REQUIRE_EQ(playback.currentTrack()->path, c.path);
  }

  {
    PlaylistController playlist;
    Track a;
    a.path = QStringLiteral("/tmp/first.mp3");
    Track b;
    b.path = QStringLiteral("/tmp/second.mp3");
    Track c;
    c.path = QStringLiteral("/tmp/third.mp3");
    playlist.setTracks({a, b, c});
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    playlist.removeAt(0);
    playback.onPlaylistChanged();
    REQUIRE(playback.playing());
    REQUIRE_EQ(playback.currentTrack()->path, b.path);
    REQUIRE_EQ(*playback.playingIndex(), 0);
  }

  {
    PlaylistController playlist;
    Track a;
    a.path = QStringLiteral("/tmp/one.mp3");
    Track b;
    b.path = QStringLiteral("/tmp/two.mp3");
    b.disabled = true;
    Track c;
    c.path = QStringLiteral("/tmp/three.mp3");
    playlist.setTracks({a, b, c});
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playIndex(0);
    playback.next();
    REQUIRE_EQ(*playback.playingIndex(), 2);
    playback.playIndex(1);
    REQUIRE_EQ(*playback.playingIndex(), 2);
    playback.previous();
    REQUIRE_EQ(*playback.playingIndex(), 0);
  }

  {
    Track t;
    t.path = QStringLiteral("/tmp/keep-playing.mp3");
    t.title = QStringLiteral("Keep Playing");
    t.artist = QStringLiteral("Wire Garden");
    t.album = QStringLiteral("Copper Rain EP");
    const auto shown = tramp::nowPlayingDisplay(t, std::nullopt, 1);
    REQUIRE_EQ(shown.title, QStringLiteral("Wire Garden — Keep Playing"));
    REQUIRE_EQ(shown.subtitle, QStringLiteral("COPPER RAIN EP"));
    REQUIRE_EQ(shown.formatChip, QStringLiteral("MP3"));
    REQUIRE(shown.title != QStringLiteral("No track"));
    const auto numbered = tramp::nowPlayingDisplay(t, 2, 12);
    REQUIRE_EQ(numbered.title, QStringLiteral("3. Wire Garden — Keep Playing"));
    REQUIRE_EQ(numbered.subtitle, QStringLiteral("COPPER RAIN EP · TRACK 3 OF 12"));
    const auto empty = tramp::nowPlayingDisplay(std::nullopt, std::nullopt, 0);
    REQUIRE_EQ(empty.title, QStringLiteral("No track"));
    REQUIRE_EQ(empty.formatChip, QStringLiteral("—"));
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

  {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString pl = tmp.filePath(QStringLiteral("mix.m3u"));
    QFile f(pl);
    REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(QByteArray("#EXTM3U\n#EXTINF:221,Wire - Hymn\ntrack.mp3\nother.flac\n"));
    f.close();

    tramp::PlaylistCollection col;
    col.add(pl);
    REQUIRE_EQ(col.readFigures().playlists, 1);
    REQUIRE_EQ(col.readFigures().tracks, 2);
    REQUIRE_EQ(col.readFigures().totalDurationMs, 221000);
    REQUIRE_EQ(col.entries().front().totalDurationMs, 221000);

    const QString other = QDir::cleanPath(QDir(QFileInfo(pl).absolutePath()).filePath(QStringLiteral("other.flac")));
    col.mergeTrackDuration(other, 45000);
    REQUIRE_EQ(col.readFigures().totalDurationMs, 266000);

    Track a;
    a.path = QDir::cleanPath(QDir(QFileInfo(pl).absolutePath()).filePath(QStringLiteral("track.mp3")));
    a.durationMs = 200000;
    Track b;
    b.path = other;
    b.durationMs = 50000;
    col.addWritten(pl, {a, b});
    REQUIRE_EQ(col.readFigures().totalDurationMs, 250000);

    Track bare;
    bare.path = other;
    QVector<Track> hydrated = {bare};
    col.hydrateDurations(hydrated);
    REQUIRE(hydrated[0].durationMs == 50000);

    col.mergeTrackTags(other, QStringLiteral("Other Side"), QStringLiteral("Wire Garden"),
                       QStringLiteral("Demos"));
    Track titledBare;
    titledBare.path = other;
    QVector<Track> titled = {titledBare};
    col.hydrateDurations(titled);
    REQUIRE_EQ(titled[0].title, QStringLiteral("Other Side"));
    REQUIRE_EQ(titled[0].artist, QStringLiteral("Wire Garden"));
    REQUIRE_EQ(titled[0].album, QStringLiteral("Demos"));

    tramp::SupportStore store(tmp.path());
    col.saveIndex(store);
    col.saveTrackSets(store);
    tramp::PlaylistCollection loaded;
    loaded.load(store);
    REQUIRE_EQ(loaded.readFigures().totalDurationMs, 250000);
    REQUIRE_EQ(loaded.readFigures().tracks, 2);
    Track reloadedBare;
    reloadedBare.path = other;
    QVector<Track> reloaded = {reloadedBare};
    loaded.hydrateDurations(reloaded);
    REQUIRE_EQ(reloaded[0].title, QStringLiteral("Other Side"));
    REQUIRE_EQ(reloaded[0].durationMs, 50000);

    const QVector<Track> fromCache = loaded.tracksFor(pl);
    REQUIRE_EQ(fromCache.size(), 2);
    REQUIRE_EQ(fromCache[1].title, QStringLiteral("Other Side"));
    REQUIRE_EQ(fromCache[1].durationMs, 50000);
  }

  {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString dir = tmp.path();
    const QString keep = QDir(dir).filePath(QStringLiteral("keep.wav"));
    const QString gone = QDir(dir).filePath(QStringLiteral("gone.wav"));
    QFile keepFile(keep);
    REQUIRE(keepFile.open(QIODevice::WriteOnly));
    keepFile.write("x");
    keepFile.close();
    const QString pl = QDir(dir).filePath(QStringLiteral("set.m3u"));
    QFile f(pl);
    REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(QStringLiteral("#EXTM3U\n%1\n%2\n").arg(keep, gone).toUtf8());
    f.close();

    tramp::PlaylistCollection col;
    Track kept;
    kept.path = keep;
    kept.title = QStringLiteral("Keep");
    kept.durationMs = 1000;
    Track missing;
    missing.path = gone;
    missing.title = QStringLiteral("Gone");
    missing.durationMs = 2000;
    col.addWritten(pl, {kept, missing});
    REQUIRE_EQ(col.tracksFor(pl).size(), 2);

    QFile::remove(pl);
    QFile rewritten(pl);
    REQUIRE(rewritten.open(QIODevice::WriteOnly | QIODevice::Text));
    rewritten.write("#EXTM3U\nonly-on-disk.mp3\n");
    rewritten.close();
    col.validateReferences();
    REQUIRE(col.disabledPaths().isEmpty());
    REQUIRE_EQ(col.tracksFor(pl).size(), 2);
    REQUIRE_EQ(col.tracksFor(pl)[0].title, QStringLiteral("Keep"));
    tramp::SavedPlaylist resolved;
    REQUIRE(col.resolveForLoad(pl, &resolved));

    QFile::remove(pl);
    col.validateReferences();
    REQUIRE(col.disabledPaths().contains(tramp::normalizePlaylistPath(pl)));
    REQUIRE(col.resolveForLoad(pl, &resolved));
    REQUIRE_EQ(col.tracksFor(pl).size(), 2);

    Track diskKeep = kept;
    Track diskGone = missing;
    const QVector<Track> purged = tramp::dropMissingTrackFiles({diskKeep, diskGone});
    REQUIRE_EQ(purged.size(), 1);
    REQUIRE_EQ(purged[0].path, keep);
  }

  {
    PlaylistController list;
    Track live;
    live.path = QStringLiteral("/tmp/live.mp3");
    live.durationMs = 10000;
    Track dead;
    dead.path = QStringLiteral("/tmp/dead.mp3");
    dead.durationMs = 20000;
    list.setTracks({live, dead}, QStringLiteral("/tmp/p.m3u"));
    REQUIRE(!list.altered());
    list.markMissingPaths({tramp::normalizePlaylistPath(dead.path)});
    REQUIRE(!list.altered());
    REQUIRE(list.tracks()[1].disabled);
    REQUIRE(!list.tracks()[0].disabled);
    REQUIRE_EQ(tramp::playableTrackCount(list.tracks()), 1);
    REQUIRE_EQ(tramp::playableTotalMs(list.tracks()), 10000);
  }

  {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString pl = tmp.filePath(QStringLiteral("unknown.m3u"));
    QFile f(pl);
    REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write(QByteArray("#EXTM3U\n#EXTINF:-1,Unknown\ntrack.mp3\n"));
    f.close();
    tramp::PlaylistCollection col;
    col.add(pl);
    REQUIRE_EQ(col.readFigures().totalDurationMs, 0);
  }

  {
    PlaylistController list;
    Track t;
    t.path = QDir::cleanPath(QDir::current().filePath(QStringLiteral("x.mp3")));
    list.setTracks({t}, QStringLiteral("/tmp/p.m3u"));
    REQUIRE(!list.altered());
    QMap<QString, qint64> durations;
    durations.insert(t.path, 123000);
    REQUIRE(list.applyDurations(durations));
    REQUIRE(!list.altered());
    REQUIRE(list.tracks()[0].durationMs == 123000);
  }

  {
    PlaylistController list;
    Track t;
    t.path = QDir::cleanPath(QDir::current().filePath(QStringLiteral("tagged.mp3")));
    list.setTracks({t}, QStringLiteral("/tmp/p.m3u"));
    REQUIRE(list.tracks()[0].displayTitle() == QFileInfo(t.path).fileName());
    REQUIRE(list.applyMetadata(t.path, QStringLiteral("Static Hymn"),
                               QStringLiteral("Wire Garden"), QStringLiteral("Demos"), 221000));
    REQUIRE(!list.altered());
    REQUIRE(list.tracks()[0].title == QStringLiteral("Static Hymn"));
    REQUIRE(list.tracks()[0].artist == QStringLiteral("Wire Garden"));
    REQUIRE(list.tracks()[0].album == QStringLiteral("Demos"));
    REQUIRE(list.tracks()[0].durationMs == 221000);
    REQUIRE(list.tracks()[0].displayTitle() == QStringLiteral("Static Hymn"));
    REQUIRE(!list.applyMetadata(t.path, QString(), QString(), QString(), 0));
    REQUIRE(list.tracks()[0].title == QStringLiteral("Static Hymn"));
  }

  {
    Track timed;
    timed.path = QStringLiteral("/tmp/tagged.mp3");
    timed.durationMs = 221000;
    REQUIRE(tramp::trackNeedsAudioProbe(timed));
    REQUIRE(tramp::pathsNeedingAudioProbe({timed}).contains(timed.path));
    timed.title = QStringLiteral("Static Hymn");
    REQUIRE(!tramp::trackNeedsAudioProbe(timed));
    REQUIRE(tramp::pathsNeedingAudioProbe({timed}).isEmpty());
  }

  {
    PlaylistController list;
    Track t;
    t.path = QStringLiteral("/tmp/keep-playing.mp3");
    t.durationMs = 221000;
    list.setTracks({t}, QStringLiteral("/tmp/current.m3u"));
    REQUIRE(list.tracks()[0].displayTitle() == QFileInfo(t.path).fileName());
    tramp::PlaylistCollection col;
    col.mergeTrackDuration(t.path, 221000);
    QVector<Track> copy = list.tracks();
    col.hydrateDurations(copy);
    list.applyMetadata(copy[0].path, copy[0].title, copy[0].artist, copy[0].album,
                       copy[0].durationMs.value_or(0));
    REQUIRE(list.tracks()[0].durationMs == 221000);
    REQUIRE(list.tracks()[0].displayTitle() == QFileInfo(t.path).fileName());
    REQUIRE(tramp::trackNeedsAudioProbe(list.tracks()[0]));
    col.mergeTrackTags(t.path, QStringLiteral("Keep Playing"), QStringLiteral("Wire Garden"),
                       QString());
    copy = list.tracks();
    col.hydrateDurations(copy);
    list.applyMetadata(copy[0].path, copy[0].title, copy[0].artist, copy[0].album,
                       copy[0].durationMs.value_or(0));
    REQUIRE_EQ(list.tracks()[0].displayTitle(), QStringLiteral("Keep Playing"));
    REQUIRE(!tramp::trackNeedsAudioProbe(list.tracks()[0]));
  }

  {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const int sampleRate = 8000;
    const int samples = 8000;
    QByteArray wav;
    const int dataBytes = samples * 2;
    wav.resize(44 + dataBytes);
    wav.fill(0);
    std::memcpy(wav.data(), "RIFF", 4);
    const quint32 riffSize = quint32(36 + dataBytes);
    qToLittleEndian(riffSize, reinterpret_cast<uchar*>(wav.data() + 4));
    std::memcpy(wav.data() + 8, "WAVE", 4);
    std::memcpy(wav.data() + 12, "fmt ", 4);
    qToLittleEndian(quint32(16), reinterpret_cast<uchar*>(wav.data() + 16));
    qToLittleEndian(quint16(1), reinterpret_cast<uchar*>(wav.data() + 20));
    qToLittleEndian(quint16(1), reinterpret_cast<uchar*>(wav.data() + 22));
    qToLittleEndian(quint32(sampleRate), reinterpret_cast<uchar*>(wav.data() + 24));
    qToLittleEndian(quint32(sampleRate * 2), reinterpret_cast<uchar*>(wav.data() + 28));
    qToLittleEndian(quint16(2), reinterpret_cast<uchar*>(wav.data() + 32));
    qToLittleEndian(quint16(16), reinterpret_cast<uchar*>(wav.data() + 34));
    std::memcpy(wav.data() + 36, "data", 4);
    qToLittleEndian(quint32(dataBytes), reinterpret_cast<uchar*>(wav.data() + 40));
    REQUIRE(tramp::probeWavDurationMs(wav) == 1000);
    const QString path = tmp.filePath(QStringLiteral("one-sec.wav"));
    QFile out(path);
    REQUIRE(out.open(QIODevice::WriteOnly));
    out.write(wav);
    out.close();
    REQUIRE(tramp::probeAudioDurationMs(path) == 1000);
  }

#ifdef TRAMP_HAVE_MPV
  {
    const QString fixture =
        QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath(QStringLiteral("fixtures/probe-tone.mp3"));
    REQUIRE(QFileInfo::exists(fixture));
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QStringList paths;
    for (int i = 0; i < 8; ++i) {
      const QString dest = tmp.filePath(QStringLiteral("t%1.mp3").arg(i));
      REQUIRE(QFile::copy(fixture, dest));
      paths.push_back(dest);
    }
    int probed = 0;
    int complete = 0;
    tramp::probeAudioDurations(paths, [] { return true; },
                               [&](const QString&, const tramp::ProbedAudio& p) {
                                 ++probed;
                                 if (!p.title.isEmpty() && p.durationMs && *p.durationMs > 0) ++complete;
                               });
    REQUIRE_EQ(probed, 8);
    REQUIRE_EQ(complete, 8);
  }
#endif

  {
    int applied = 0;
    int restored = 0;
    tramp::WaitCursorScope::installHooks({[&]() { ++applied; }, [&]() { ++restored; }});
    {
      tramp::WaitCursorScope outer;
      REQUIRE(tramp::WaitCursorScope::showing());
      REQUIRE_EQ(applied, 1);
      REQUIRE_EQ(restored, 0);
      {
        tramp::WaitCursorScope inner;
        REQUIRE_EQ(applied, 1);
        REQUIRE_EQ(restored, 0);
      }
      REQUIRE_EQ(applied, 1);
      REQUIRE_EQ(restored, 0);
    }
    REQUIRE_EQ(applied, 1);
    REQUIRE_EQ(restored, 1);
    {
      tramp::WaitCursorScope again;
      REQUIRE_EQ(applied, 2);
    }
    REQUIRE_EQ(restored, 2);
    tramp::WaitCursorScope::resetHooks();
  }

  {
    int applied = 0;
    int restored = 0;
    tramp::WaitCursorScope::installHooks({[&]() { ++applied; }, [&]() { ++restored; }});
    {
      tramp::WaitCursorScope outer;
      REQUIRE_EQ(applied, 1);
      {
        tramp::WaitCursorPause pause;
        REQUIRE(!tramp::WaitCursorScope::showing());
        REQUIRE_EQ(restored, 1);
      }
      REQUIRE(tramp::WaitCursorScope::showing());
      REQUIRE_EQ(applied, 2);
    }
    REQUIRE_EQ(restored, 2);
    tramp::WaitCursorScope::resetHooks();
  }

  if (gFails != 0) {
    std::fprintf(stderr, "%d assertion(s) failed\n", gFails);
    return 1;
  }
  std::puts("domain_test: ok");
  return 0;
}
