#include "equalizer.h"
#include "m3u.h"
#include "playlist.h"
#include "support_dir.h"
#include "track.h"
#include "transport.h"

#include <QDir>
#include <QFile>
#include <QVariant>
#include <cstdio>
#include <cstdlib>

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

using tramp::EqualizerSettings;
using tramp::M3uCodec;
using tramp::PlaylistController;
using tramp::RepeatMode;
using tramp::Track;
using tramp::buildEqualizerAf;
using tramp::nextIndex;
using tramp::previousIndex;
using tramp::resolveLinuxSupportPath;

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

  if (gFails != 0) {
    std::fprintf(stderr, "%d assertion(s) failed\n", gFails);
    return 1;
  }
  std::puts("domain_test: ok");
  return 0;
}
