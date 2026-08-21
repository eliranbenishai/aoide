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
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTemporaryDir>
#include <QByteArray>
#include <QtEndian>
#include <QVariant>
#include <QVector>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <thread>

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
    // Next must never hand back the track that is already playing. When the
    // current index is missing from the shuffle order — a playlist replace
    // clears it while the open file keeps playing — the fallback used to be
    // the current index itself, so Next re-opened the same track.
    const QVector<int> order{2, 0, 1};
    REQUIRE_EQ(nextIndex(3, 4, true, RepeatMode::off, order).value_or(-1), 2);
    REQUIRE_EQ(previousIndex(3, 4, true, RepeatMode::off, order).value_or(-1), 1);
    // With no order drawn yet, shuffle walks the list rather than standing still.
    REQUIRE_EQ(nextIndex(0, 3, true, RepeatMode::off, {}).value_or(-1), 1);
    REQUIRE_EQ(previousIndex(2, 3, true, RepeatMode::off, {}).value_or(-1), 1);
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
    // Two derivations of one spectrogram. analyzeMonoPcm — what the blocks above
    // measure — folds the whole buffer in one pass; the app runs the cancellable
    // load, which folds a couple of seconds of audio at a time so teardown does
    // not have to wait out a track. They agree because a slice starts on a hop
    // boundary and carries the analysis window's tail, and every frame is
    // windowed and normalised on its own. Nothing but this pins that down.
    constexpr int kFftSize = 4096;
    constexpr int kFramesPerSecond = 30;
    constexpr double kPi = 3.14159265358979323846;
    const auto foldsTheSame = [](int sampleRateHz, int count) {
      QVector<double> samples(count);
      for (int i = 0; i < count; ++i) {
        const double t = double(i) / double(sampleRateHz);
        samples[i] = 0.4 * std::sin(2.0 * kPi * 440.0 * t) + 0.2 * std::sin(2.0 * kPi * 5300.0 * t);
      }
      const PcmBuffer pcm{samples, sampleRateHz};
      const SpectrumAnalyzer sliced(SpectrumAnalyzer::CancellablePcmLoader(
          [pcm](const QString&, const SpectrumAnalyzer::CancelFn&) { return pcm; }));
      const Spectrogram whole = SpectrumAnalyzer().analyzeMonoPcm(samples, sampleRateHz);
      const Spectrogram inSlices =
          sliced.load(QStringLiteral("fixture://tone.wav"), []() { return true; });
      REQUIRE_EQ(inSlices.framesPerSecond, whole.framesPerSecond);
      REQUIRE_EQ(inSlices.sampleRateHz, whole.sampleRateHz);
      REQUIRE_EQ(int(inSlices.frames.size()), int(whole.frames.size()));
      if (inSlices.frames.size() != whole.frames.size()) return;
      // Bit-identical, not near: the slices run the same FFT over the same
      // windowed samples, so any drift at all is a cut in the wrong place.
      int differing = 0;
      for (int f = 0; f < whole.frames.size(); ++f) {
        for (size_t band = 0; band < whole.frames[f].size(); ++band) {
          if (inSlices.frames[f][band] != whole.frames[f][band]) ++differing;
        }
      }
      REQUIRE_EQ(differing, 0);
    };

    const int hop = 44100 / kFramesPerSecond;
    const int oneSlice = (kFramesPerSecond * 2 - 1) * hop + kFftSize;
    foldsTheSame(44100, 0);                 // nothing to fold
    foldsTheSame(44100, 100);               // shorter than the analysis window
    foldsTheSame(44100, kFftSize - 1);
    foldsTheSame(44100, kFftSize);          // exactly one frame
    foldsTheSame(44100, oneSlice - 1);      // one frame short of a full slice
    foldsTheSame(44100, oneSlice);          // exactly one slice
    foldsTheSame(44100, oneSlice + hop);    // a second slice holding a single frame
    foldsTheSame(48000, 3 * 48000);         // a rate that is not 44100
  }

  {
    // Cancelling is not failing. The fold stops with no frames and load reports
    // silence, which nobody reads: the worker that asked to stop drops it.
    const SpectrumAnalyzer analyzer(SpectrumAnalyzer::CancellablePcmLoader(
        [](const QString&, const SpectrumAnalyzer::CancelFn&) {
          return PcmBuffer{QVector<double>(200000, 0.25), 44100};
        }));
    const Spectrogram spec = analyzer.load(QStringLiteral("tone.wav"), []() { return false; });
    REQUIRE_EQ(int(spec.frames.size()), 1);
    REQUIRE(!spec.levelsAt(0).synthetic);
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
    // Stopping drops the bars to rest. The musical release holds peaks near the
    // top for seconds, which reads as a track still sounding after it ended.
    SpectrumHold hold;
    AudioLevels frame;
    frame.bands.fill(1.0);
    hold.apply(frame);
    REQUIRE(!hold.atRest());
    int frames = 0;
    while (!hold.atRest() && frames < 200) {
      hold.release();
      ++frames;
    }
    REQUIRE(hold.atRest());
    // ~0.5s of fall at the 33 ms spectrum tick, not the ~6s kPeakDecay took.
    REQUIRE(frames > 4);
    REQUIRE(frames < 25);
    for (int i = 0; i < AudioLevels::kBandCount; ++i) {
      REQUIRE(hold.peaks[size_t(i)] <= SpectrumHold::kRestFloor);
    }
  }

  {
    SpectrumHold hold;
    REQUIRE(hold.atRest());
    AudioLevels frame;
    frame.bands[3] = 0.5;
    hold.apply(frame);
    REQUIRE(!hold.atRest());
  }

  {
    // A hidden panel keeps the position it will reappear at. Counting it against
    // the desktop edge reserved ghost space: a closed About parked left of main
    // stopped main reaching the left edge of the screen.
    tramp::DockLayout layout;
    layout.main = {true, false, 400, 100, {}, {}};
    layout.equalizer = {true, false, 400, 448, {}, {}};
    layout.playlist = {false, false, 0, 0, {}, {}};
    layout.settings = {false, false, 0, 0, {}, {}};
    layout.about = {false, false, 0, 500, {}, {}};
    const QVector<tramp::WindowId> members = tramp::visibleClusterMembers(layout);
    REQUIRE_EQ(members.size(), 2);
    REQUIRE(members.contains(tramp::WindowId::main));
    REQUIRE(members.contains(tramp::WindowId::equalizer));
    REQUIRE(!members.contains(tramp::WindowId::about));

    layout.about.visible = true;
    REQUIRE(tramp::visibleClusterMembers(layout).contains(tramp::WindowId::about));
  }

  {
    // Main cannot be hidden, so it anchors the cluster even if persist says otherwise.
    tramp::DockLayout layout;
    layout.main = {false, false, 0, 0, {}, {}};
    layout.equalizer = {false, false, 0, 0, {}, {}};
    layout.playlist = {false, false, 0, 0, {}, {}};
    layout.settings = {false, false, 0, 0, {}, {}};
    layout.about = {false, false, 0, 0, {}, {}};
    REQUIRE_EQ(tramp::visibleClusterMembers(layout), QVector<tramp::WindowId>{tramp::WindowId::main});
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
    // A crawl of two logical pixels is under the peel delta, so the dock edge
    // holds and the panel keeps the group it is in.
    tramp::DockLayout layout;
    layout.main = {true, false, 0, 0, {}, {}};
    layout.equalizer = {true, false, 0, 348, {}, {}};
    layout.playlist.visible = false;
    layout.dockEdges = {
        {tramp::WindowId::equalizer, tramp::WindowId::main, tramp::DockSide::top}};
    tramp::DockingCoordinator dock(layout);
    dock.setSnapThreshold(20);
    dock.move(tramp::WindowId::equalizer, QPointF(2, 350), false, true);
    REQUIRE_EQ(dock.layout().dockEdges.size(), 1);
    REQUIRE_EQ(dock.layout().equalizer.left, 2.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 350.0);

    // Shift undocks the same crawl: the edge goes at once, and the drop is left
    // where the listener put it instead of snapping back onto main.
    dock.move(tramp::WindowId::equalizer, QPointF(4, 352), true, true);
    REQUIRE(dock.layout().dockEdges.isEmpty());
    REQUIRE_EQ(dock.layout().equalizer.left, 4.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 352.0);
    // Main is not dragged along by an undock.
    REQUIRE_EQ(dock.layout().main.left, 0.0);
    REQUIRE_EQ(dock.layout().main.top, 0.0);

    // Undocked is not un-dockable: the next ordinary drop this close snaps back.
    dock.move(tramp::WindowId::equalizer, QPointF(4, 352), false, true);
    REQUIRE(!dock.layout().dockEdges.isEmpty());
    REQUIRE_EQ(dock.layout().equalizer.left, 0.0);
    REQUIRE_EQ(dock.layout().equalizer.top, 348.0);
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
    // The refused file also has to be let go: stop is what unloads media, and
    // without it mpv keeps the errored load attached.
    class RefusingEngine : public NullEngine {
     public:
      void open(const Track&) override {
        if (onDuration) onDuration(200000);
        if (onError) onError(QStringLiteral("cannot open"));
      }
      void play() override { played = true; }
      void stop() override {
        stopped = true;
        NullEngine::stop();
      }
      bool played = false;
      bool stopped = false;
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
    REQUIRE(engine.stopped);
    REQUIRE_EQ(playback.failureMessage(), QStringLiteral("cannot open"));
    REQUIRE_EQ(playback.durationMs(), qint64(200000));

    // Stop unloads media, and the readouts let go with it. They used to
    // survive: the subtitle still carried the error and the clock still showed
    // the length of a track nothing was playing.
    playback.stop();
    REQUIRE(playback.failureMessage().isEmpty());
    REQUIRE_EQ(playback.durationMs(), qint64(0));
    REQUIRE_EQ(playback.positionMs(), qint64(0));
    REQUIRE(!playback.currentTrack().has_value());
    REQUIRE(!playback.playingIndex().has_value());
    REQUIRE(!playback.playing());
    REQUIRE(!playback.paused());
    // Play picks the list back up from the row still selected.
    playback.playPause();
    REQUIRE_EQ(playback.playingIndex().value_or(-1), 0);
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
    // Playlists written on Windows are routinely CP1252, and UTF-8 ones often
    // carry a byte-order mark. Decoding everything as UTF-8 turned those titles
    // into replacement characters and left their paths unresolvable.
    const QByteArray withBom =
        QByteArray("\xEF\xBB\xBF#EXTM3U\n#EXTINF:1,Bj\xC3\xB6rk\n/m/a.mp3\n");
    const QString bomText = tramp::decodeM3uBytes(withBom);
    REQUIRE(!bomText.startsWith(QChar(0xFEFF)));
    REQUIRE(bomText.contains(QString::fromUtf8("Björk")));
    REQUIRE(bomText.startsWith(QStringLiteral("#EXTM3U")));

    QByteArray latin1("#EXTM3U\n#EXTINF:1,Caf");
    latin1.append(char(0xE9));
    latin1.append("\n/m/b.mp3\n");
    const QString latinText = tramp::decodeM3uBytes(latin1);
    REQUIRE(latinText.contains(QString::fromUtf8("Café")));
    REQUIRE(!latinText.contains(QChar(QChar::ReplacementCharacter)));
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
    // A listener who ran the eight-step ladder has 200% or 50% saved. Restoring
    // a percent the ladder no longer carries would leave the readout on a number
    // the zoom buttons cannot get back to, so it snaps to the nearest surviving
    // step rather than being thrown away for the default.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    tramp::SupportStore store(tmp.path());
    const QString path = QDir(tmp.path()).filePath(QStringLiteral("settings.json"));
    const auto restoredZoom = [&](int saved) {
      QFile file(path);
      REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
      file.write(QByteArray("{\"zoomPercent\": ") + QByteArray::number(saved) +
                 QByteArray("}"));
      file.close();
      return store.readSettings().zoomPercent;
    };
    REQUIRE_EQ(restoredZoom(200), 150);
    REQUIRE_EQ(restoredZoom(300), 150);
    REQUIRE_EQ(restoredZoom(50), 75);
    REQUIRE_EQ(restoredZoom(125), 125);
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
    // A relative path in a state file keys everything on whichever directory
    // Tramp happened to be started from: the same collection read from another
    // working directory misses every cached track. Nothing relative is written,
    // and nothing relative survives a read.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    tramp::SupportStore store(tmp.path());
    const QString relList = QStringLiteral("lists/set.m3u");
    const QString relTrack = QStringLiteral("lists/a.mp3");
    const QString absList = tramp::normalizePlaylistPath(relList);
    const QString absTrack = tramp::normalizePlaylistPath(relTrack);

    tramp::SavedPlaylist entry;
    entry.path = relList;
    entry.trackCount = 1;
    REQUIRE(store.writeCollectionIndex({entry}));
    tramp::CollectionTrackSets sets;
    sets.byEntry.insert(relList, {relTrack});
    sets.durationsMs.insert(relTrack, 1000);
    sets.meta.insert(relTrack, {QStringLiteral("A"), QString(), QString()});
    REQUIRE(store.writeTrackSets(sets));

    auto slurp = [&](const QString& name) {
      QFile f(QDir(tmp.path()).filePath(name));
      REQUIRE(f.open(QIODevice::ReadOnly));
      const QByteArray text = f.readAll();
      f.close();
      return text;
    };
    const QByteArray index = slurp(QStringLiteral("playlists.json"));
    REQUIRE(!index.contains(QByteArray("\"") + relList.toUtf8() + "\""));
    REQUIRE(index.contains(absList.toUtf8()));
    const QByteArray cached = slurp(QStringLiteral("playlist_tracks.json"));
    REQUIRE(!cached.contains(QByteArray("\"") + relTrack.toUtf8() + "\""));
    REQUIRE(cached.contains(absTrack.toUtf8()));

    // And the two files still describe the same tracks after the round trip.
    const tramp::CollectionTrackSets back = store.readTrackSets();
    REQUIRE_EQ(back.byEntry.value(absList).size(), 1);
    REQUIRE_EQ(back.byEntry.value(absList).value(0), absTrack);
    REQUIRE_EQ(back.durationsMs.value(absTrack, -1), qint64(1000));
    REQUIRE_EQ(back.meta.value(absTrack).title, QStringLiteral("A"));
  }

  {
    // A disabled row has to come back disabled. The kept playlist was written
    // without that state, so a restored altered list painted every row enabled
    // until the background check caught up — and while it did, footer TOTAL and
    // N TRACKS counted files that are not there.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    tramp::SupportStore store(tmp.path());
    Track live;
    live.path = QStringLiteral("/music/live.mp3");
    live.durationMs = 1000;
    Track dead;
    dead.path = QStringLiteral("/music/dead.mp3");
    dead.durationMs = 2000;
    dead.disabled = true;
    REQUIRE(store.writeAltered({{live, dead}, QStringLiteral("/music/set.m3u")}));

    const tramp::AlteredPlaylist back = store.readAltered();
    REQUIRE_EQ(back.tracks.size(), 2);
    REQUIRE(!back.tracks[0].disabled);
    REQUIRE(back.tracks[1].disabled);
    REQUIRE_EQ(tramp::playableTrackCount(back.tracks), 1);
    REQUIRE_EQ(tramp::playableTotalMs(back.tracks), 1000);
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
    // Shuffle was seeded from the playing index, so turning it on at the same
    // track always dealt the same order — the listener's second evening was
    // the first one again.
    PlaylistController playlist;
    QVector<Track> many;
    for (int i = 0; i < 30; ++i) {
      Track t;
      t.path = QStringLiteral("/tmp/shuffle-%1.mp3").arg(i);
      many.push_back(t);
    }
    playlist.setTracks(many);
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    QSet<int> dealt;
    for (int attempt = 0; attempt < 8; ++attempt) {
      playback.setShuffle(false);
      playback.playIndex(0);
      playback.setShuffle(true);
      playback.next();
      dealt.insert(playback.playingIndex().value_or(-1));
    }
    REQUIRE(dealt.size() > 1);
  }

  {
    // Repeat-all has to deal a new pass when the list wraps. Replaying the one
    // order is the same complaint one lap later.
    PlaylistController playlist;
    QVector<Track> many;
    for (int i = 0; i < 12; ++i) {
      Track t;
      t.path = QStringLiteral("/tmp/lap-%1.mp3").arg(i);
      many.push_back(t);
    }
    playlist.setTracks(many);
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.setRepeatMode(RepeatMode::all);
    playback.playIndex(0);
    playback.setShuffle(true);
    auto restOfPass = [&]() {
      QVector<int> visited;
      for (int i = 1; i < many.size(); ++i) {
        playback.next();
        visited.push_back(playback.playingIndex().value_or(-1));
      }
      return visited;
    };
    const QVector<int> first = restOfPass();
    playback.next();
    const int secondOpener = playback.playingIndex().value_or(-1);
    const QVector<int> second = restOfPass();
    REQUIRE(first != second);
    // A new deal must not re-open the track that just finished.
    REQUIRE(secondOpener != first.back());
    // And each pass still gives every enabled row exactly one turn.
    QSet<int> firstPass(first.begin(), first.end());
    firstPass.insert(0);
    REQUIRE_EQ(firstPass.size(), many.size());
    QSet<int> secondPass(second.begin(), second.end());
    secondPass.insert(secondOpener);
    REQUIRE_EQ(secondPass.size(), many.size());
  }

  {
    // Auto-start and resume both hand the transport an index nothing checked:
    // the first row, or the one the listener left. playIndex refuses a disabled
    // row, so a playlist whose first file went missing came up silent with
    // nothing on the display saying why.
    PlaylistController playlist;
    Track gone;
    gone.path = QStringLiteral("/tmp/gone.mp3");
    gone.disabled = true;
    Track live;
    live.path = QStringLiteral("/tmp/live.mp3");
    playlist.setTracks({gone, live});
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playFrom(0);
    REQUIRE(playback.playing());
    REQUIRE_EQ(playback.playingIndex().value_or(-1), 1);
    REQUIRE(playback.failureMessage().contains(QStringLiteral("gone.mp3")));
    // A playable row is played as asked, with nothing to report.
    playback.playFrom(1);
    REQUIRE_EQ(playback.playingIndex().value_or(-1), 1);
    REQUIRE(playback.failureMessage().isEmpty());
    // Play and double-click still refuse a disabled row outright.
    playback.playIndex(0);
    REQUIRE_EQ(playback.playingIndex().value_or(-1), 1);
  }

  {
    // A Spin is one track played through to the end. The counter used to fire
    // on the end-of-file event alone, so dragging the seek bar to the last
    // second of a track earned a spin nobody listened to.
    class ClockEngine : public NullEngine {
     public:
      void open(const Track&) override {
        at = 0;
        if (onDuration) onDuration(200000);
      }
      void seekMs(qint64 positionMs) override {
        at = positionMs;
        NullEngine::seekMs(positionMs);
      }
      /// Hand the clock forward the way playback does, a tick at a time.
      void playTo(qint64 ms) {
        for (qint64 t = at + 250; t <= ms; t += 250) {
          at = t;
          if (onPosition) onPosition(t);
        }
      }
      void finish() {
        if (onCompleted) onCompleted();
      }
      qint64 at = 0;
    };
    PlaylistController playlist;
    Track track;
    track.path = QStringLiteral("/tmp/spin.mp3");
    playlist.setTracks({track});
    ClockEngine engine;
    PlaybackController playback(&playlist, &engine);

    playback.playIndex(0);
    engine.playTo(200000);
    engine.finish();
    REQUIRE_EQ(playback.spins(), 1);

    playback.playIndex(0);
    engine.playTo(5000);
    playback.seekMs(199000);
    engine.playTo(200000);
    engine.finish();
    REQUIRE_EQ(playback.spins(), 1);

    // Most of the track is enough — the last few seconds of applause are not
    // what makes it a listen.
    playback.playIndex(0);
    engine.playTo(195000);
    engine.finish();
    REQUIRE_EQ(playback.spins(), 2);
  }

  {
    // Nothing left to fall through to: say so rather than sit silent.
    PlaylistController playlist;
    Track gone;
    gone.path = QStringLiteral("/tmp/gone.mp3");
    gone.disabled = true;
    Track alsoGone;
    alsoGone.path = QStringLiteral("/tmp/also-gone.mp3");
    alsoGone.disabled = true;
    playlist.setTracks({gone, alsoGone});
    NullEngine engine;
    PlaybackController playback(&playlist, &engine);
    playback.playFrom(1);
    REQUIRE(!playback.playing());
    REQUIRE(!playback.playingIndex().has_value());
    REQUIRE(!playback.failureMessage().isEmpty());
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
    // The figures count the files that are on this machine, so the two the
    // playlist names have to be there.
    for (const char* name : {"track.mp3", "other.flac"}) {
      QFile audio(tmp.filePath(QString::fromLatin1(name)));
      REQUIRE(audio.open(QIODevice::WriteOnly));
      audio.write("x");
      audio.close();
    }

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
    // Pruning is the one thing here that can destroy what the app cannot get
    // back, so the rules are pinned before anything calls it: a live list keeps
    // every row it mentions, and only rows nothing mentions go.
    tramp::CollectionTrackSets sets;
    sets.byEntry.insert(QStringLiteral("/music/keep.m3u"),
                        {QStringLiteral("/music/a.mp3"), QStringLiteral("/music/shared.mp3")});
    sets.byEntry.insert(QStringLiteral("/music/gone.m3u"),
                        {QStringLiteral("/music/b.mp3"), QStringLiteral("/music/shared.mp3")});
    sets.durationsMs.insert(QStringLiteral("/music/a.mp3"), 1000);
    sets.durationsMs.insert(QStringLiteral("/music/b.mp3"), 2000);
    sets.durationsMs.insert(QStringLiteral("/music/shared.mp3"), 3000);
    sets.durationsMs.insert(QStringLiteral("/music/orphan.mp3"), 4000);
    sets.meta.insert(QStringLiteral("/music/a.mp3"), {QStringLiteral("A"), {}, {}});
    sets.meta.insert(QStringLiteral("/music/b.mp3"), {QStringLiteral("B"), {}, {}});
    sets.meta.insert(QStringLiteral("/music/orphan.mp3"), {QStringLiteral("Orphan"), {}, {}});

    const auto kept = tramp::pruneTrackSets(sets, {QStringLiteral("/music/keep.m3u")});
    REQUIRE_EQ(kept.byEntry.size(), 1);
    REQUIRE(kept.byEntry.contains(QStringLiteral("/music/keep.m3u")));
    REQUIRE_EQ(kept.byEntry.value(QStringLiteral("/music/keep.m3u")).size(), 2);
    REQUIRE_EQ(kept.durationsMs.value(QStringLiteral("/music/a.mp3"), -1), 1000);
    // Shared with a list that left, but still named by one that stayed.
    REQUIRE_EQ(kept.durationsMs.value(QStringLiteral("/music/shared.mp3"), -1), 3000);
    REQUIRE(!kept.durationsMs.contains(QStringLiteral("/music/b.mp3")));
    REQUIRE(!kept.durationsMs.contains(QStringLiteral("/music/orphan.mp3")));
    REQUIRE_EQ(kept.meta.value(QStringLiteral("/music/a.mp3")).title, QStringLiteral("A"));
    REQUIRE(!kept.meta.contains(QStringLiteral("/music/b.mp3")));
    REQUIRE(!kept.meta.contains(QStringLiteral("/music/orphan.mp3")));

    // Both entries live: nothing goes but the orphan.
    const auto both = tramp::pruneTrackSets(
        sets, {QStringLiteral("/music/keep.m3u"), QStringLiteral("/music/gone.m3u")});
    REQUIRE_EQ(both.byEntry.size(), 2);
    REQUIRE_EQ(both.durationsMs.size(), 3);
    REQUIRE_EQ(both.meta.size(), 2);

    // An empty collection keeps nothing, and a prune of nothing is not a crash.
    REQUIRE(tramp::pruneTrackSets(sets, {}).byEntry.isEmpty());
    REQUIRE(tramp::pruneTrackSets(sets, {}).durationsMs.isEmpty());
    REQUIRE(tramp::pruneTrackSets({}, {QStringLiteral("/music/keep.m3u")}).byEntry.isEmpty());

    // A key that was never normalized still matches the live entry it belongs
    // to. Comparing raw strings here would throw the listener's durations away.
    tramp::CollectionTrackSets unclean;
    unclean.byEntry.insert(QStringLiteral("/music/./keep.m3u"),
                           {QStringLiteral("/music/sub/../c.mp3")});
    unclean.durationsMs.insert(QStringLiteral("/music/c.mp3"), 5000);
    const auto tidied = tramp::pruneTrackSets(unclean, {QStringLiteral("/music/keep.m3u")});
    REQUIRE_EQ(tidied.byEntry.size(), 1);
    REQUIRE_EQ(tidied.durationsMs.value(QStringLiteral("/music/c.mp3"), -1), 5000);
  }

  {
    // Whether a saved playlist is disabled was worked out once, at bootstrap.
    // A row therefore stayed enabled after its M3U was deleted mid-session, and
    // stayed disabled after the file came back — CONTEXT.md says it enables
    // itself again, and it did, next restart.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString pl = QDir(tmp.path()).filePath(QStringLiteral("mid-session.m3u"));
    auto writeList = [&]() {
      QFile f(pl);
      REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Text));
      f.write(QByteArray("#EXTM3U\n"));
      f.close();
    };
    writeList();

    tramp::PlaylistCollection col;
    col.setValidationIntervalMs(0);
    col.add(pl);
    const QString key = tramp::normalizePlaylistPath(pl);
    REQUIRE(!col.disabledPaths().contains(key));

    REQUIRE(QFile::remove(pl));
    REQUIRE(col.disabledPaths().contains(key));

    writeList();
    REQUIRE(!col.disabledPaths().contains(key));
  }

  {
    // ON THIS MACHINE has to mean what is on this machine. The figures summed
    // every path the cache had ever seen, so an album deleted from disk kept
    // padding TRACKS and TOTAL TIME for the life of the collection.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString keep = QDir(tmp.path()).filePath(QStringLiteral("keep.mp3"));
    const QString gone = QDir(tmp.path()).filePath(QStringLiteral("gone.mp3"));
    for (const QString& path : {keep, gone}) {
      QFile f(path);
      REQUIRE(f.open(QIODevice::WriteOnly));
      f.write("x");
      f.close();
    }
    const QString pl = QDir(tmp.path()).filePath(QStringLiteral("set.m3u"));
    QFile list(pl);
    REQUIRE(list.open(QIODevice::WriteOnly | QIODevice::Text));
    list.write(QStringLiteral("#EXTM3U\n%1\n%2\n").arg(keep, gone).toUtf8());
    list.close();

    tramp::PlaylistCollection col;
    Track a;
    a.path = keep;
    a.durationMs = 1000;
    Track b;
    b.path = gone;
    b.durationMs = 2000;
    col.addWritten(pl, {a, b});
    REQUIRE_EQ(col.readFigures().tracks, 2);
    REQUIRE_EQ(col.readFigures().totalDurationMs, 3000);

    REQUIRE(QFile::remove(gone));
    col.validateReferences();
    REQUIRE_EQ(col.readFigures().tracks, 1);
    REQUIRE_EQ(col.readFigures().totalDurationMs, 1000);
    // The row is still in the playlist and still in the cache; it is the file
    // that is gone, so it comes back with the file.
    REQUIRE_EQ(col.tracksFor(pl).size(), 2);

    // Refresh is the mid-session route to the same answer, and the one that
    // runs while the app is open: re-reading a list re-asks about that list's
    // tracks, so a file that came back is counted again without a restart.
    QFile back(gone);
    REQUIRE(back.open(QIODevice::WriteOnly));
    back.write("x");
    back.close();
    col.addWritten(pl, {a, b});
    REQUIRE_EQ(col.readFigures().tracks, 2);
    REQUIRE(QFile::remove(gone));
    col.addWritten(pl, {a, b});
    REQUIRE_EQ(col.readFigures().tracks, 1);

    tramp::SupportStore store(tmp.path());
    col.saveIndex(store);
    col.saveTrackSets(store);
    tramp::PlaylistCollection reloaded;
    reloaded.load(store);
    reloaded.validateReferences();  // what bootstrap does, and the only sweep
    REQUIRE_EQ(reloaded.readFigures().playlists, 1);
    REQUIRE_EQ(reloaded.readFigures().tracks, 1);
    REQUIRE_EQ(reloaded.readFigures().totalDurationMs, 1000);
  }

  {
    // Presence used to be worked out lazily, by any read that found the last
    // look two seconds old. Two of those reads are hot: readFigures runs once
    // per probed duration through an ingest, and the collection view asks
    // disabledPaths once per row. So a stat for every track in the collection
    // could land inside a repaint — tens of milliseconds on an SSD, and a stall
    // on a share that has dropped. The reads are pure now; this keeps them so.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    tramp::PlaylistCollection col;
    int listAsks = 0;
    int trackAsks = 0;
    col.setExists([&](const QString& path) {
      if (path.endsWith(QStringLiteral(".m3u"))) ++listAsks;
      else ++trackAsks;
      return true;
    });
    for (int i = 0; i < 3; ++i) {
      QVector<Track> tracks;
      for (int t = 0; t < 20; ++t) {
        Track track;
        track.path = QDir(tmp.path()).filePath(QStringLiteral("t-%1-%2.mp3").arg(i).arg(t));
        track.durationMs = 1000;
        tracks.push_back(track);
      }
      col.addWritten(QDir(tmp.path()).filePath(QStringLiteral("set-%1.m3u").arg(i)), tracks);
    }
    const int rows = col.entries().size();
    REQUIRE_EQ(rows, 3);

    // Bootstrap is where the whole collection may be asked about, once.
    listAsks = 0;
    trackAsks = 0;
    col.validateReferences();
    REQUIRE_EQ(trackAsks, 60);
    REQUIRE_EQ(listAsks, rows);

    // A chrome snapshot: entries, the disabled set once per row, the About
    // figures. Not one question about a track.
    listAsks = 0;
    trackAsks = 0;
    for (int i = 0; i < rows; ++i) {
      (void)col.entries();
      (void)col.disabledPaths();
    }
    REQUIRE_EQ(col.readFigures().tracks, 60);
    REQUIRE_EQ(trackAsks, 0);
    REQUIRE_EQ(listAsks, 0);

    // Once the validation pass goes stale a read brings it up to date, which is
    // what notices a playlist file coming and going. That costs one question
    // per row — never one per track, however much music the collection holds.
    col.setValidationIntervalMs(0);
    listAsks = 0;
    trackAsks = 0;
    (void)col.disabledPaths();
    REQUIRE_EQ(listAsks, rows);
    REQUIRE_EQ(trackAsks, 0);

    // And the figures still stay off the disk when they are read hot, the way
    // an ingest reads them.
    listAsks = 0;
    trackAsks = 0;
    for (int i = 0; i < 100; ++i) {
      col.mergeTrackDuration(QDir(tmp.path()).filePath(QStringLiteral("t-0-0.mp3")), 2000 + i);
      (void)col.readFigures();
    }
    REQUIRE_EQ(trackAsks, 0);
    REQUIRE_EQ(listAsks, 0);
  }

  {
    // Every existence check goes through the probe, including the ones on write
    // paths. A check that asks the real disk behind an injected double turns
    // into a silent no-op for whoever writes the next test.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString pl = QDir(tmp.path()).filePath(QStringLiteral("set.m3u"));
    QFile list(pl);
    REQUIRE(list.open(QIODevice::WriteOnly | QIODevice::Text));
    list.write("#EXTM3U\n#EXTINF:120,Wire Garden - One\none.mp3\n");
    list.close();

    tramp::PlaylistCollection col;
    col.add(pl);
    REQUIRE_EQ(col.entries().front().trackCount, 1);

    // Re-adding a playlist whose file has gone keeps the figures it had.
    REQUIRE(QFile::remove(pl));
    col.setExists([](const QString&) { return false; });
    col.add(pl);
    REQUIRE_EQ(col.entries().front().trackCount, 1);

    // Say it is there and the figures are rebuilt from what could be read,
    // which is nothing — the probe decides, not QFileInfo behind its back.
    col.setExists([](const QString&) { return true; });
    col.add(pl);
    REQUIRE_EQ(col.entries().front().trackCount, 0);
  }

  {
    // A disabled playlist is still in the collection, and the cache is the only
    // thing left to paint its rows from — pruning must not touch it.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString pl = QDir(tmp.path()).filePath(QStringLiteral("vanished.m3u"));
    tramp::PlaylistCollection col;
    Track a;
    a.path = QDir(tmp.path()).filePath(QStringLiteral("a.mp3"));
    a.title = QStringLiteral("A");
    a.durationMs = 1000;
    col.addWritten(pl, {a});
    col.validateReferences();
    REQUIRE(col.disabledPaths().contains(tramp::normalizePlaylistPath(pl)));
    tramp::SupportStore store(tmp.path());
    col.saveTrackSets(store);
    REQUIRE_EQ(col.tracksFor(pl).size(), 1);
    REQUIRE_EQ(col.tracksFor(pl)[0].durationMs.value_or(0), 1000);
    tramp::PlaylistCollection reloaded;
    reloaded.load(store);
    REQUIRE_EQ(reloaded.tracksFor(pl).size(), 1);
  }

  {
    // The track-set cache never evicted anything: removing a collection entry
    // dropped its list but left every duration and tag row behind, so
    // playlist_tracks.json grew for as long as Tramp was used and every write
    // serialised the lot.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString cache = QDir(tmp.path()).filePath(QStringLiteral("playlist_tracks.json"));
    tramp::SupportStore store(tmp.path());
    tramp::PlaylistCollection col;
    col.saveTrackSets(store);
    const qint64 empty = QFileInfo(cache).size();
    REQUIRE(empty > 0);
    for (int i = 0; i < 50; ++i) {
      const QString pl = QDir(tmp.path()).filePath(QStringLiteral("set-%1.m3u").arg(i));
      QFile f(pl);
      REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Text));
      f.write(QStringLiteral("#EXTM3U\n"
                             "#EXTINF:221,Wire Garden - Hymn %1\n"
                             "track-%1-a.mp3\n"
                             "#EXTINF:180,Wire Garden - Rain %1\n"
                             "track-%1-b.mp3\n")
                  .arg(i)
                  .toUtf8());
      f.close();
      col.add(pl);
      col.remove(pl);
    }
    col.saveTrackSets(store);
    REQUIRE_EQ(QFileInfo(cache).size(), empty);
    REQUIRE_EQ(col.readFigures().tracks, 0);
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
    // The two ends of the background probe apply an answer differently on
    // purpose. An ingest fills in what the row does not know; Refresh believes
    // the file over the file that listed it, so a stale #EXTINF is corrected
    // rather than kept.
    Track stale;
    stale.path = QStringLiteral("/tmp/stale.mp3");
    stale.title = QStringLiteral("Wrong Title");
    stale.durationMs = 9000;

    tramp::ProbedAudio truth;
    truth.title = QStringLiteral("Right Title");
    truth.durationMs = 221000;

    Track kept = stale;
    tramp::applyProbedAudio(kept, truth, false);
    REQUIRE_EQ(kept.title, QStringLiteral("Wrong Title"));
    REQUIRE_EQ(kept.durationMs.value_or(0), qint64(9000));

    Track corrected = stale;
    tramp::applyProbedAudio(corrected, truth, true);
    REQUIRE_EQ(corrected.title, QStringLiteral("Right Title"));
    REQUIRE_EQ(corrected.durationMs.value_or(0), qint64(221000));

    // Refresh puts the corrected row back by path, and a row the path verify
    // has already disabled stays disabled.
    PlaylistController list;
    Track row = stale;
    row.disabled = true;
    list.setTracks({row}, QStringLiteral("/tmp/current.m3u"));
    Track next = list.tracks()[0];
    tramp::applyProbedAudio(next, truth, true);
    REQUIRE(list.updateTrackByPath(stale.path, next));
    REQUIRE_EQ(list.tracks()[0].title, QStringLiteral("Right Title"));
    REQUIRE_EQ(list.tracks()[0].durationMs.value_or(0), qint64(221000));
    REQUIRE(list.tracks()[0].disabled);
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

  {
    // A WAV header promises however much audio it likes. A file cut short by a
    // failed copy or a full disk still claimed the whole length, in the row and
    // in TOTAL TIME, so the duration is worth no more than the bytes behind it.
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QByteArray full = makePcm16Wav(QVector<qint16>(8000, 0), 1, 8000);
    REQUIRE_EQ(tramp::probeWavDurationMs(full).value_or(-1), qint64(1000));

    const QByteArray cut = full.left(44 + 4000);
    REQUIRE_EQ(tramp::probeWavDurationMs(cut).value_or(-1), qint64(250));

    const QString cutPath = tmp.filePath(QStringLiteral("cut.wav"));
    QFile cutFile(cutPath);
    REQUIRE(cutFile.open(QIODevice::WriteOnly));
    cutFile.write(cut);
    cutFile.close();
    REQUIRE_EQ(tramp::probeAudioDurationMs(cutPath).value_or(-1), qint64(250));

    // And a file bigger than the window the probe reads still reports its whole
    // length: the head is not the file.
    const QByteArray longer = makePcm16Wav(QVector<qint16>(80000, 0), 1, 8000);
    REQUIRE(longer.size() > 64 * 1024);
    const QString longPath = tmp.filePath(QStringLiteral("ten-sec.wav"));
    QFile longFile(longPath);
    REQUIRE(longFile.open(QIODevice::WriteOnly));
    longFile.write(longer);
    longFile.close();
    REQUIRE_EQ(tramp::probeAudioDurationMs(longPath).value_or(-1), qint64(10000));
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
    // Every answer is reported the moment it lands, so a list can paint itself
    // while the rest of it is still being asked about.
    const QStringList paths{QStringLiteral("a.flac"), QStringLiteral("b.flac")};
    QStringList order;
    tramp::probeAudioDurations(
        paths, {},
        [&](const QString& p, const tramp::ProbedAudio&) { order.push_back(QLatin1String("said ") + p); },
        [&](const QString& p, const tramp::ProbeCancelFn&) {
          order.push_back(QLatin1String("asked ") + p);
          tramp::ProbedAudio probed;
          probed.durationMs = 2000;
          return std::optional<tramp::ProbedAudio>(probed);
        });
    REQUIRE_EQ(order.join(QLatin1Char('/')),
               QStringLiteral("asked a.flac/said a.flac/asked b.flac/said b.flac"));
  }

  {
    // A stalled network mount cannot be mounted in a test, so the per-file probe
    // is the seam: this one takes as long as a file mpv accepts and never
    // reports loaded, and answers the cancel question on the same small ticks
    // the libmpv wait does. Cancelling has to land inside the file being probed
    // — waiting out the file is what held teardown for twenty seconds.
    QStringList paths;
    for (int i = 0; i < 20; ++i) paths.push_back(QStringLiteral("/slow/mount/%1.flac").arg(i));
    std::atomic<bool> live{true};
    QStringList asked;
    QStringList said;

    QElapsedTimer clock;
    clock.start();
    std::thread worker([&]() {
      tramp::probeAudioDurations(
          paths, [&]() { return live.load(); },
          [&](const QString& p, const tramp::ProbedAudio&) { said.push_back(p); },
          [&](const QString& p, const tramp::ProbeCancelFn& stillWanted) {
            asked.push_back(p);
            for (int tick = 0; tick < 200; ++tick) {
              if (stillWanted && !stillWanted()) return std::optional<tramp::ProbedAudio>();
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            tramp::ProbedAudio probed;
            probed.durationMs = 1000;
            return std::optional<tramp::ProbedAudio>(probed);
          });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    live.store(false);
    worker.join();

    // One file is 200 ms and the whole run is four seconds; stopping has to cost
    // a tick, not a file, and nothing after the cancelled file gets asked at all.
    REQUIRE(clock.elapsed() < 150);
    REQUIRE_EQ(asked.size(), 1);
    REQUIRE(said.isEmpty());
  }

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
