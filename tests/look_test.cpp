#include "look.h"

#include "mockup_tokens.h"
#include "aoide_metrics.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRectF>
#include <QSizeF>
#include <QTemporaryDir>
#include <QVariant>
#include <cstdio>
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

QString hex(const QColor& c) { return c.name(QColor::HexRgb).toLower(); }

QJsonObject obj(const QString& json) {
  return QJsonDocument::fromJson(json.toUtf8()).object();
}

void writePack(const QString& dir, const QString& folder, const QString& json,
               const QString& filename = QStringLiteral("skin.json")) {
  const QString pack = QDir(dir).filePath(folder);
  QDir().mkpath(pack);
  QFile f(QDir(pack).filePath(filename));
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
  f.write(json.toUtf8());
}

}  // namespace

int main() {
  using aoide::ChromeTokens;
  using aoide::LookError;
  using aoide::SkinConflictChoice;
  using aoide::SkinController;
  using aoide::AoideSettings;
  using aoide::parseLookManifest;
  using aoide::resolveLook;
  using aoide::scanLookCatalog;

  {
    const auto m = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1,
      "id": "neon-cyan",
      "name": "Neon Cyan",
      "author": "Example",
      "extends": "builtin",
      "colors": { "phosphor": { "default": "#3de7ff" } }
    })")));
    REQUIRE_EQ(m.id, QStringLiteral("neon-cyan"));
    REQUIRE_EQ(m.extendsId, QStringLiteral("builtin"));
    REQUIRE(m.colors.contains(QStringLiteral("phosphor")));
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "com.example.neon", "name": "X", "extends": "builtin"
      })")));
    } catch (const LookError&) {
      threw = true;
    }
    REQUIRE(threw);
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "neon-cyan", "name": "X", "extends": "builtin",
        "colors": { "neon": "#ffffff" }
      })")));
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("unknown color key"));
    }
    REQUIRE(threw);
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "neon-cyan", "name": "X", "extends": "builtin",
        "fonts": { "lcd": { "file": "/tmp/evil.ttf" } }
      })")));
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("absolute"));
    }
    REQUIRE(threw);
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "neon-cyan", "name": "X", "extends": "builtin",
        "fonts": { "lcd": { "file": "fonts/../../evil.ttf" } }
      })")));
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral(".."));
    }
    REQUIRE(threw);
  }

  {
    const auto neon = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin",
      "colors": { "phosphor": { "default": "#3de7ff" }, "accent": { "default": "#ff3d9a" } }
    })")));
    const auto resolved = resolveLook(QStringLiteral("neon-cyan"), {neon});
    REQUIRE_EQ(hex(resolved.palette.shellHi), QStringLiteral("#323744"));
    REQUIRE_EQ(hex(resolved.palette.phos), QStringLiteral("#3de7ff"));
  }

  {
    const auto overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "custom", "name": "Custom", "extends": "builtin",
      "materials": { "spectrum": { "stops": ["#ff0000", "#00ff00"] } }
    })")));
    const auto resolved = resolveLook(QStringLiteral("custom"), {overlay});
    REQUIRE_EQ(resolved.materials.spectrumStops.size(), 2);
    REQUIRE_EQ(hex(resolved.materials.spectrumStops[0]), QStringLiteral("#ff0000"));
    REQUIRE_EQ(hex(resolved.materials.spectrumStops[1]), QStringLiteral("#00ff00"));
  }

  {
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(100, 100), 0), 0);
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(100, 100), 10), 10);
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(100, 100), 40), 25);
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(200, 80), 40), 20);
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(825, 348), 200), 87);
    REQUIRE_EQ(aoide::rectangularCornerRadius(QSizeF(0, 80), 10), 0);
  }

  {
    const auto m = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "round-pack", "name": "Round", "extends": "builtin",
      "radii": { "window": 12, "surface": 8, "button": 2 }
    })")));
    REQUIRE_EQ(m.radii.value(QStringLiteral("window")).toDouble(), 12);
    REQUIRE_EQ(m.radii.value(QStringLiteral("surface")).toDouble(), 8);
    REQUIRE_EQ(m.radii.value(QStringLiteral("button")).toDouble(), 2);
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "round-pack", "name": "X", "extends": "builtin",
        "radii": { "window": -1 }
      })")));
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("radii.window"));
    }
    REQUIRE(threw);
  }

  {
    bool threw = false;
    try {
      parseLookManifest(obj(QStringLiteral(R"({
        "formatVersion": 1, "id": "round-pack", "name": "X", "extends": "builtin",
        "radii": { "pill": 4 }
      })")));
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("unknown radii key"));
    }
    REQUIRE(threw);
  }

  {
    const auto overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "sharp", "name": "Sharp", "extends": "builtin",
      "radii": { "window": 0, "button": 0 }
    })")));
    const auto resolved = resolveLook(QStringLiteral("sharp"), {overlay});
    REQUIRE_EQ(resolved.radii.window, 0);
    REQUIRE_EQ(resolved.radii.surface, 6);
    REQUIRE_EQ(resolved.radii.button, 0);
    const auto tokens = ChromeTokens::from(resolved);
    REQUIRE_EQ(tokens.windowRadius(QRectF(0, 0, 825, 348)), 0);
    REQUIRE_EQ(tokens.surfaceRadius(QRectF(0, 0, 705, 132)), 6);
    REQUIRE_EQ(tokens.buttonRadius(QRectF(0, 0, 52, 38)), 0);
  }

  {
    const auto overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "huge", "name": "Huge", "extends": "builtin",
      "radii": { "window": 200, "surface": 80, "button": 40 }
    })")));
    const auto tokens = ChromeTokens::from(resolveLook(QStringLiteral("huge"), {overlay}));
    REQUIRE_EQ(tokens.radii.window, 200);
    REQUIRE_EQ(tokens.windowRadius(QRectF(0, 0, 825, 348)), 87);
    REQUIRE_EQ(tokens.surfaceRadius(QRectF(0, 0, 705, 132)), 33);
    REQUIRE_EQ(tokens.buttonRadius(QRectF(0, 0, 52, 38)), 19);
  }

  {
    aoide::LookManifest a;
    a.id = QStringLiteral("a");
    a.name = QStringLiteral("A");
    a.extendsId = QStringLiteral("b");
    aoide::LookManifest b;
    b.id = QStringLiteral("b");
    b.name = QStringLiteral("B");
    b.extendsId = QStringLiteral("a");
    bool threw = false;
    try {
      resolveLook(QStringLiteral("a"), {a, b});
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("cycle"));
    }
    REQUIRE(threw);
  }

  {
    QVector<aoide::LookManifest> installed;
    for (int i = 0; i < 9; ++i) {
      aoide::LookManifest m;
      m.id = QStringLiteral("pack-%1").arg(i);
      m.name = QStringLiteral("Pack %1").arg(i);
      m.extendsId = i == 0 ? QStringLiteral("builtin") : QStringLiteral("pack-%1").arg(i - 1);
      installed.push_back(m);
    }
    bool threw = false;
    try {
      resolveLook(QStringLiteral("pack-8"), installed);
    } catch (const LookError& e) {
      threw = e.message().contains(QStringLiteral("chain"));
    }
    REQUIRE(threw);
  }

  {
    REQUIRE_EQ(aoide::builtinLookManifest().name, QStringLiteral("Aoide"));
    REQUIRE_EQ(aoide::builtinLookManifest().author, QStringLiteral("Proxima Magnifica"));
    aoide::SkinController skins;
    REQUIRE_EQ(skins.catalog().size(), 1);
    REQUIRE_EQ(skins.catalog()[0].id, QStringLiteral("builtin"));
    REQUIRE_EQ(skins.catalog()[0].name, QStringLiteral("Aoide"));
    REQUIRE_EQ(skins.catalog()[0].author, QStringLiteral("Proxima Magnifica"));
  }

  {
    const ChromeTokens& t = ChromeTokens::builtin();
    REQUIRE_EQ(hex(t.titleBar0), QStringLiteral("#3c4356"));
    REQUIRE_EQ(hex(t.titleBar26), QStringLiteral("#2c3241"));
    REQUIRE_EQ(hex(t.titleBar62), QStringLiteral("#1d222c"));
    REQUIRE_EQ(hex(t.titleBar100), QStringLiteral("#12151c"));
    REQUIRE_EQ(hex(t.wbtn0), QStringLiteral("#454d60"));
    REQUIRE_EQ(hex(t.wbtn55), QStringLiteral("#2f3543"));
    REQUIRE_EQ(hex(t.wbtn100), QStringLiteral("#20242e"));
    REQUIRE_EQ(hex(t.wbtnClose0), QStringLiteral("#9c2a60"));
    REQUIRE_EQ(hex(t.wbtnClose55), QStringLiteral("#79204a"));
    REQUIRE_EQ(hex(t.wbtnClose100), QStringLiteral("#4a1129"));
    REQUIRE_EQ(hex(t.btnIdle0), QStringLiteral("#3f4657"));
    REQUIRE_EQ(hex(t.wordmark), QStringLiteral("#eaf2ff"));
    REQUIRE_EQ(hex(t.btnOn0), QStringLiteral("#a9f4ff"));
    REQUIRE_EQ(hex(t.btnOnInk), QStringLiteral("#04222b"));
    REQUIRE_EQ(hex(t.sliderFillHi), QStringLiteral("#cbf9ff"));
    REQUIRE_EQ(hex(t.plateFace), QStringLiteral("#1e222c"));
    REQUIRE_EQ(hex(t.coolSheen), QStringLiteral("#e2ecff"));
    REQUIRE_EQ(hex(t.closeGlyph), QStringLiteral("#ffd6e8"));
    REQUIRE_EQ(hex(t.accentHot), QStringLiteral("#ffd6ea"));
    REQUIRE_EQ(hex(t.screenWash0), QStringLiteral("#0f1c2a"));
    REQUIRE_EQ(hex(t.listWash0), QStringLiteral("#0f1c2a"));
    REQUIRE_EQ(hex(t.listWash1), QStringLiteral("#071018"));
    REQUIRE_EQ(hex(t.listWash2), QStringLiteral("#04070c"));
    REQUIRE_EQ(hex(t.listSheen), QStringLiteral("#e2ecff"));
    REQUIRE_EQ(hex(t.metalHi), QStringLiteral("#636876"));
    REQUIRE_EQ(hex(t.metalMid), QStringLiteral("#2a303c"));
    REQUIRE_EQ(hex(t.metalLo), QStringLiteral("#1e222c"));
    REQUIRE_EQ(hex(t.eqThumbHi), QStringLiteral("#757c8f"));
    REQUIRE_EQ(hex(t.scrollThumbHi), QStringLiteral("#7d8496"));
    REQUIRE(t.spectrumStops.size() == 4);
    REQUIRE_EQ(hex(t.phos), hex(aoide::kPhos));
    REQUIRE_EQ(t.radii.window, 6);
    REQUIRE_EQ(t.radii.surface, 6);
    REQUIRE_EQ(t.radii.button, 4);
    REQUIRE_EQ(t.windowRadius(QRectF(0, 0, 825, 348)), 6);
    REQUIRE_EQ(t.buttonRadius(QRectF(0, 0, 52, 38)), 4);
  }

  {
    aoide::LookManifest overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "amber-shift", "name": "Amber", "extends": "builtin",
      "colors": { "shell": { "highlight": "#3a3228", "mid": "#1c1812" } }
    })")));
    const auto resolved = resolveLook(QStringLiteral("amber-shift"), {overlay});
    REQUIRE_EQ(hex(ChromeTokens::from(resolved).plateFace), QStringLiteral("#201d18"));
    REQUIRE_EQ(hex(ChromeTokens::from(resolved).metalHi), QStringLiteral("#6b635a"));
  }

  {
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const auto empty = scanLookCatalog(tmp.path());
    REQUIRE(empty.manifests.isEmpty());
    REQUIRE(empty.warnings.isEmpty());
  }

  {
    QTemporaryDir tmp;
    writePack(tmp.path(), QStringLiteral("neon-cyan"), QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin"
    })"),
              QStringLiteral("look.json"));
    const auto result = scanLookCatalog(tmp.path());
    REQUIRE_EQ(result.manifests.size(), 1);
    REQUIRE_EQ(result.manifests[0].name, QStringLiteral("Neon Cyan"));
    REQUIRE(result.warnings.isEmpty());
  }

  {
    QTemporaryDir tmp;
    writePack(tmp.path(), QStringLiteral("wrong-folder"), QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin"
    })"));
    const auto result = scanLookCatalog(tmp.path());
    REQUIRE(result.manifests.isEmpty());
    REQUIRE_EQ(result.warnings.size(), 1);
    REQUIRE(result.warnings[0].contains(QStringLiteral("wrong-folder")));
    REQUIRE(result.warnings[0].contains(QStringLiteral("neon-cyan")));
  }

  {
    QTemporaryDir support;
    QTemporaryDir bundled;
    writePack(bundled.path(), QStringLiteral("gamma"), QStringLiteral(R"({
      "formatVersion": 1, "id": "gamma", "name": "Gamma", "author": "Aoide",
      "extends": "builtin",
      "colors": { "phosphor": { "default": "#5cff4d", "hot": "#c8ff9a", "dim": "#2a8a22", "deep": "#143c10" } }
    })"));
    writePack(bundled.path(), QStringLiteral("thunder"), QStringLiteral(R"({
      "formatVersion": 1, "id": "thunder", "name": "Thunder", "author": "Aoide",
      "extends": "builtin"
    })"));
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), bundled.path(), settings);
    REQUIRE(skins.catalog().size() >= 3);
    REQUIRE_EQ(skins.catalog()[0].id, QStringLiteral("builtin"));
    REQUIRE_EQ(skins.catalog()[0].name, QStringLiteral("Aoide"));
    REQUIRE_EQ(skins.catalog()[1].id, QStringLiteral("thunder"));
    REQUIRE_EQ(skins.catalog()[2].id, QStringLiteral("gamma"));
    REQUIRE(skins.activate(QStringLiteral("gamma"), settings));
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("gamma"));
    REQUIRE_EQ(hex(skins.tokens().phos), QStringLiteral("#5cff4d"));
    REQUIRE(QDir(QDir(support.path()).filePath(QStringLiteral("skins/gamma"))).exists());
  }

  {
    QTemporaryDir support;
    QTemporaryDir bundled;
    writePack(support.path() + QStringLiteral("/skins"), QStringLiteral("amber-terminal"),
              QStringLiteral(R"({
      "formatVersion": 1, "id": "amber-terminal", "name": "Amber Terminal", "extends": "builtin"
    })"));
    writePack(support.path() + QStringLiteral("/skins"), QStringLiteral("violet-pulse"),
              QStringLiteral(R"({
      "formatVersion": 1, "id": "violet-pulse", "name": "Violet Pulse", "extends": "builtin"
    })"));
    writePack(bundled.path(), QStringLiteral("arc"), QStringLiteral(R"({
      "formatVersion": 1, "id": "arc", "name": "Arc", "extends": "builtin"
    })"));
    AoideSettings settings;
    settings.activeSkinId = QStringLiteral("amber-terminal");
    SkinController skins;
    skins.bootstrap(support.path(), bundled.path(), settings);
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("builtin"));
    REQUIRE(!QDir(QDir(support.path()).filePath(QStringLiteral("skins/amber-terminal"))).exists());
    REQUIRE(!QDir(QDir(support.path()).filePath(QStringLiteral("skins/violet-pulse"))).exists());
    REQUIRE(QDir(QDir(support.path()).filePath(QStringLiteral("skins/arc"))).exists());
  }

  {
    QTemporaryDir support;
    QTemporaryDir bundled;
    writePack(support.path() + QStringLiteral("/skins"), QStringLiteral("chaos"),
              QStringLiteral(R"({
      "formatVersion": 1, "id": "chaos", "name": "Chaos", "extends": "builtin"
    })"));
    writePack(bundled.path(), QStringLiteral("mind"), QStringLiteral(R"({
      "formatVersion": 1, "id": "mind", "name": "Mind", "extends": "builtin"
    })"));
    AoideSettings settings;
    settings.activeSkinId = QStringLiteral("chaos");
    SkinController skins;
    skins.bootstrap(support.path(), bundled.path(), settings);
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("mind"));
    REQUIRE(!QDir(QDir(support.path()).filePath(QStringLiteral("skins/chaos"))).exists());
    REQUIRE(QDir(QDir(support.path()).filePath(QStringLiteral("skins/mind"))).exists());
  }

  {
    QTemporaryDir support;
    QTemporaryDir custom;
    writePack(custom.path(), QStringLiteral("amber-terminal"), QStringLiteral(R"({
      "formatVersion": 1, "id": "amber-terminal", "name": "Amber Terminal", "extends": "builtin"
    })"));
    writePack(custom.path(), QStringLiteral("arc"), QStringLiteral(R"({
      "formatVersion": 1, "id": "arc", "name": "Arc", "extends": "builtin"
    })"));
    AoideSettings settings;
    settings.skinsDirectory = custom.path();
    settings.activeSkinId = QStringLiteral("violet-pulse");
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("builtin"));
    REQUIRE_EQ(skins.catalog()[0].id, QStringLiteral("builtin"));
    REQUIRE_EQ(skins.catalog()[1].id, QStringLiteral("arc"));
    REQUIRE_EQ(skins.catalog().size(), 2);
  }

  {
    QTemporaryDir support;
    AoideSettings settings;
    settings.activeSkinId = QStringLiteral("missing-pack");
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("builtin"));
    REQUIRE_EQ(skins.tokens().id, QStringLiteral("builtin"));
  }

  {
    QTemporaryDir support;
    QTemporaryDir incoming;
    writePack(incoming.path(), QStringLiteral("neon-cyan"), QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin",
      "colors": { "phosphor": { "default": "#112233", "hot": "#b8f6ff", "dim": "#1a7a88", "deep": "#0d3d46" } }
    })"));
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE(skins.installDirectory(QDir(incoming.path()).filePath(QStringLiteral("neon-cyan")),
                                   [](const aoide::SkinConflict&) {
                                     return SkinConflictChoice::cancel;
                                   }));
    REQUIRE(skins.activate(QStringLiteral("neon-cyan"), settings));
    REQUIRE_EQ(hex(skins.tokens().phos), QStringLiteral("#112233"));
  }

  // The extractor is a host binary and differs per platform: unzip everywhere
  // but Windows, which has no unzip and gets System32 tar instead. The fixture
  // carries the shape a real pack has — a nested root folder and a fonts
  // subdirectory — so a host that unpacks only the top level fails here.
  {
    const QString zip = QFileInfo(QString::fromUtf8(__FILE__))
                            .dir()
                            .filePath(QStringLiteral("fixtures/skin-pack.zip"));
    REQUIRE(QFileInfo::exists(zip));
    QTemporaryDir support;
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE(skins.installZip(zip, [](const aoide::SkinConflict&) {
      return SkinConflictChoice::cancel;
    }));
    REQUIRE(skins.lastError().isEmpty());
    REQUIRE(skins.activate(QStringLiteral("neon-zip"), settings));
    REQUIRE_EQ(hex(skins.tokens().phos), QStringLiteral("#0aff9d"));
    REQUIRE(QFileInfo::exists(
        QDir(support.path()).filePath(QStringLiteral("skins/neon-zip/fonts/note.txt"))));
  }

  {
    AoideSettings saved;
    saved.main.left = 640;
    saved.main.top = 200;
    saved.equalizer.left = 640;
    saved.equalizer.top = 548;
    saved.playlist.left = 40;
    saved.playlist.top = 40;
    saved.playlist.width = 1073;
    saved.playlist.height = 696;
    const QByteArray compact =
        QJsonDocument(saved.toJson()).toJson(QJsonDocument::Compact);
    const AoideSettings loaded =
        AoideSettings::fromJson(QJsonDocument::fromJson(compact).object());
    REQUIRE_EQ(loaded.main.left, 640.0);
    REQUIRE_EQ(loaded.main.top, 200.0);
    REQUIRE_EQ(loaded.equalizer.left, 640.0);
    REQUIRE_EQ(loaded.equalizer.top, 548.0);
    REQUIRE_EQ(loaded.playlist.left, 40.0);
    REQUIRE_EQ(loaded.playlist.top, 40.0);
    REQUIRE(loaded.playlist.width && *loaded.playlist.width == 1073.0);
  }

  {
    AoideSettings saved;
    saved.showElapsed = false;
    saved.scrollTitle = false;
    const AoideSettings loaded = AoideSettings::fromJson(saved.toJson());
    REQUIRE(!loaded.showElapsed);
    REQUIRE(!loaded.scrollTitle);
    REQUIRE(loaded.toJson().value(QStringLiteral("showElapsed")).toBool() == false);
  }

  {
    const AoideSettings defaults = AoideSettings::fromJson(QJsonObject{});
    REQUIRE(defaults.showElapsed);
    REQUIRE(defaults.scrollTitle);
  }

  {
    const AoideSettings remain = AoideSettings::fromJson(obj(QStringLiteral(R"({
      "showElapsed": false
    })")));
    REQUIRE(!remain.showElapsed);
    REQUIRE(remain.scrollTitle);
  }

  {
    const AoideSettings loaded = AoideSettings::fromJson(obj(QStringLiteral(R"({
      "main": {"visible": true, "shaded": false, "left": 120, "top": 80},
      "equalizer": {"visible": true, "shaded": false, "left": 120, "top": 428},
      "playlist": {"visible": true, "shaded": false, "left": 0, "top": 776, "width": 900, "height": 500}
    })")));
    REQUIRE_EQ(loaded.main.left, 120.0);
    REQUIRE_EQ(loaded.main.top, 80.0);
    REQUIRE_EQ(loaded.equalizer.left, 120.0);
    REQUIRE_EQ(loaded.equalizer.top, 428.0);
    REQUIRE_EQ(loaded.playlist.left, 0.0);
    REQUIRE_EQ(loaded.playlist.top, 776.0);
    REQUIRE(loaded.playlist.width && *loaded.playlist.width == 900.0);
  }

#ifdef AOIDE_SKINS_DIR
  {
    const auto bundled = scanLookCatalog(QStringLiteral(AOIDE_SKINS_DIR));
    REQUIRE(bundled.warnings.isEmpty());
    REQUIRE_EQ(bundled.manifests.size(), 7);
    QStringList ids;
    for (const auto& m : bundled.manifests) ids.push_back(m.id);
    for (const char* id : {"arc", "shield", "thunder", "gamma", "widow", "marksman", "mind"}) {
      REQUIRE(ids.contains(QString::fromUtf8(id)));
    }
    const auto arc = resolveLook(QStringLiteral("arc"), bundled.manifests);
    REQUIRE_EQ(hex(arc.palette.shell), QStringLiteral("#700f15"));
    REQUIRE_EQ(hex(arc.palette.ink), QStringLiteral("#f8f2e2"));
    REQUIRE_EQ(hex(arc.palette.phos), QStringLiteral("#ffc107"));
    REQUIRE_EQ(hex(arc.palette.accent), QStringLiteral("#e8b020"));
    const auto arcTokens = ChromeTokens::from(arc);
    REQUIRE(hex(arcTokens.listWash0) != QStringLiteral("#0f1c2a"));
    REQUIRE(hex(arcTokens.listWash0) != QStringLiteral("#191011"));
    REQUIRE(arcTokens.listWash0.lightness() == QColor(QStringLiteral("#0f1c2a")).lightness());
    REQUIRE(arcTokens.listWash0.red() > arcTokens.listWash0.blue());
    REQUIRE(hex(arcTokens.listWash0) != hex(arcTokens.screenWash0));
    const int accentHue = QColor(QStringLiteral("#e8b020")).hslHue();
    REQUIRE(arcTokens.closeGlyph.hslHue() >= 0);
    REQUIRE(qAbs(arcTokens.closeGlyph.hslHue() - accentHue) <= 40);
    QTemporaryDir support;
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), QStringLiteral(AOIDE_SKINS_DIR), settings);
    REQUIRE_EQ(skins.catalog().size(), 8);
    REQUIRE_EQ(skins.catalog()[0].name, QStringLiteral("Aoide"));
    REQUIRE_EQ(skins.catalog()[1].id, QStringLiteral("arc"));
    REQUIRE_EQ(skins.catalog()[2].id, QStringLiteral("shield"));
    REQUIRE_EQ(skins.catalog()[3].id, QStringLiteral("thunder"));
    REQUIRE_EQ(skins.catalog()[4].id, QStringLiteral("gamma"));
    REQUIRE_EQ(skins.catalog()[5].id, QStringLiteral("widow"));
    REQUIRE_EQ(skins.catalog()[6].id, QStringLiteral("marksman"));
    REQUIRE_EQ(skins.catalog()[7].id, QStringLiteral("mind"));
  }
#endif

  {
    aoide::LookManifest overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "green-close", "name": "Green", "extends": "builtin",
      "colors": { "accent": { "default": "#00cc44", "dim": "#007722" } }
    })")));
    const auto tokens = ChromeTokens::from(resolveLook(QStringLiteral("green-close"), {overlay}));
    REQUIRE(hex(tokens.wbtnClose0) != QStringLiteral("#9c2a60"));
    REQUIRE(hex(tokens.closeGlyph) != QStringLiteral("#ffd6e8"));
    const int accentHue = QColor(QStringLiteral("#00cc44")).hslHue();
    REQUIRE(tokens.wbtnClose0.hslHue() >= 0);
    REQUIRE(qAbs(tokens.wbtnClose0.hslHue() - accentHue) <= 40);
    REQUIRE(qAbs(tokens.closeGlyph.hslHue() - accentHue) <= 40);
  }

  {
    aoide::LookManifest overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "arc-ink", "name": "Arc Ink", "extends": "builtin",
      "colors": {
        "ink": { "default": "#ff2d3a", "dim": "#c45a62", "faint": "#8a3e44" },
        "accent": { "default": "#c5ccd6", "dim": "#6a7380" }
      }
    })")));
    const auto tokens = ChromeTokens::from(resolveLook(QStringLiteral("arc-ink"), {overlay}));
    REQUIRE(tokens.closeGlyph.red() > tokens.closeGlyph.green() + 40);
    REQUIRE(tokens.closeGlyph.red() > tokens.closeGlyph.blue() + 40);
    const int inkHue = QColor(QStringLiteral("#ff2d3a")).hslHue();
    REQUIRE(tokens.closeGlyph.hslHue() >= 0);
    const int dh = qAbs(tokens.closeGlyph.hslHue() - inkHue);
    REQUIRE(dh <= 25 || dh >= 335);
  }

  {
    QTemporaryDir support;
    QTemporaryDir bundled;
    writePack(bundled.path(), QStringLiteral("arc"), QStringLiteral(R"({
      "formatVersion": 1, "id": "arc", "name": "Arc", "extends": "builtin"
    })"));
    writePack(bundled.path(), QStringLiteral("gamma"), QStringLiteral(R"({
      "formatVersion": 1, "id": "gamma", "name": "Gamma", "extends": "builtin"
    })"));
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), bundled.path(), settings);
    REQUIRE(skins.catalog().size() >= 3);
    QDir(QDir(support.path()).filePath(QStringLiteral("skins/arc"))).removeRecursively();
    skins.rescan();
    QStringList ids;
    for (const auto& e : skins.catalog()) ids.push_back(e.id);
    REQUIRE(ids.contains(QStringLiteral("builtin")));
    REQUIRE(ids.contains(QStringLiteral("gamma")));
    REQUIRE(!ids.contains(QStringLiteral("arc")));
  }

  {
    QTemporaryDir support;
    QTemporaryDir incoming;
    writePack(incoming.path(), QStringLiteral("neon-cyan"), QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin"
    })"));
    writePack(incoming.path(), QStringLiteral("dusk"), QStringLiteral(R"({
      "formatVersion": 1, "id": "dusk", "name": "Dusk", "extends": "neon-cyan"
    })"));
    AoideSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE(skins.installDirectory(QDir(incoming.path()).filePath(QStringLiteral("neon-cyan")),
                                   [](const aoide::SkinConflict&) {
                                     return SkinConflictChoice::cancel;
                                   }));
    REQUIRE(skins.installDirectory(QDir(incoming.path()).filePath(QStringLiteral("dusk")),
                                   [](const aoide::SkinConflict&) {
                                     return SkinConflictChoice::cancel;
                                   }));
    int writes = 0;
    auto writeStub = [&](const QString&, const QString& path, const QVector<aoide::LookManifest>&,
                         QString*) {
      ++writes;
      QDir().mkpath(QFileInfo(path).absolutePath());
      QFile f(path);
      if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
      static const char kPng[] = "\x89PNG\r\n\x1a\nIHDR";
      f.write(QByteArray::fromRawData(kPng, 12));
      return true;
    };
    skins.ensurePreviews(writeStub);
    REQUIRE(writes >= 3);
    REQUIRE(QFileInfo::exists(skins.previewPath(QStringLiteral("builtin"))));
    REQUIRE(QFileInfo::exists(skins.previewPath(QStringLiteral("neon-cyan"))));
    writes = 0;
    skins.ensurePreviews(writeStub);
    REQUIRE_EQ(writes, 0);

    QFile stamp(QDir(QFileInfo(skins.previewPath(QStringLiteral("builtin"))).absolutePath())
                    .filePath(QStringLiteral("generation")));
    REQUIRE(stamp.open(QIODevice::WriteOnly | QIODevice::Truncate));
    stamp.write("0\n");
    stamp.close();
    writes = 0;
    skins.ensurePreviews(writeStub);
    REQUIRE(writes >= 3);

    QFile png(skins.previewPath(QStringLiteral("neon-cyan")));
    REQUIRE(png.open(QIODevice::ReadWrite));
    REQUIRE(png.setFileTime(QDateTime::currentDateTime().addSecs(-120),
                            QFileDevice::FileModificationTime));
    png.close();
    writePack(QDir(support.path()).filePath(QStringLiteral("skins")), QStringLiteral("neon-cyan"),
              QStringLiteral(R"({
      "formatVersion": 1, "id": "neon-cyan", "name": "Neon Cyan", "extends": "builtin"
    })"));
    skins.rescan();
    writes = 0;
    skins.ensurePreviews(writeStub);
    REQUIRE(writes >= 1);

    {
      QFile junk(skins.previewPath(QStringLiteral("neon-cyan")));
      REQUIRE(junk.open(QIODevice::WriteOnly | QIODevice::Truncate));
      junk.write("not a png");
      junk.close();
      writes = 0;
      skins.ensurePreviews(writeStub);
      REQUIRE(writes >= 1);
    }
    {
      QFile::remove(skins.previewPath(QStringLiteral("builtin")));
      auto failWrite = [&](const QString&, const QString&, const QVector<aoide::LookManifest>&,
                           QString* error) {
        if (error) *error = QStringLiteral("disk full");
        return false;
      };
      skins.ensurePreviews(failWrite);
      REQUIRE(skins.lastError() == QStringLiteral("disk full"));
    }

    REQUIRE(!skins.remove(QStringLiteral("builtin"), settings));
    REQUIRE(skins.activate(QStringLiteral("neon-cyan"), settings));
    REQUIRE(!skins.remove(QStringLiteral("neon-cyan"), settings));
    REQUIRE(!skins.remove(QStringLiteral("neon-cyan"), settings));
    REQUIRE(skins.activate(QStringLiteral("builtin"), settings));
    REQUIRE(!skins.remove(QStringLiteral("neon-cyan"), settings));
    REQUIRE(skins.remove(QStringLiteral("dusk"), settings));
    REQUIRE(skins.remove(QStringLiteral("neon-cyan"), settings));
    QStringList left;
    for (const auto& e : skins.catalog()) left.push_back(e.id);
    REQUIRE(!left.contains(QStringLiteral("neon-cyan")));
    REQUIRE(!left.contains(QStringLiteral("dusk")));
    REQUIRE(!QDir(QDir(support.path()).filePath(QStringLiteral("skins/neon-cyan"))).exists());
    REQUIRE(!QFileInfo::exists(skins.previewPath(QStringLiteral("neon-cyan"))));
    REQUIRE(!QFileInfo::exists(skins.previewPath(QStringLiteral("dusk"))));
  }

  if (gFails != 0) {
    std::fprintf(stderr, "%d assertion(s) failed\n", gFails);
    return 1;
  }
  std::puts("look_test: ok");
  return 0;
}
