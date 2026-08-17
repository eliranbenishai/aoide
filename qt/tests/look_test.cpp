#include "look.h"

#include "mockup_tokens.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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
  using tramp::ChromeTokens;
  using tramp::LookError;
  using tramp::SkinConflictChoice;
  using tramp::SkinController;
  using tramp::TrampSettings;
  using tramp::parseLookManifest;
  using tramp::resolveLook;
  using tramp::scanLookCatalog;

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
    tramp::LookManifest a;
    a.id = QStringLiteral("a");
    a.name = QStringLiteral("A");
    a.extendsId = QStringLiteral("b");
    tramp::LookManifest b;
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
    QVector<tramp::LookManifest> installed;
    for (int i = 0; i < 9; ++i) {
      tramp::LookManifest m;
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
    REQUIRE(t.spectrumStops.size() == 4);
    REQUIRE_EQ(hex(t.phos), hex(tramp::kPhos));
  }

  {
    tramp::LookManifest overlay = parseLookManifest(obj(QStringLiteral(R"({
      "formatVersion": 1, "id": "amber-shift", "name": "Amber", "extends": "builtin",
      "colors": { "shell": { "mid": "#1c1812" } }
    })")));
    const auto resolved = resolveLook(QStringLiteral("amber-shift"), {overlay});
    REQUIRE_EQ(hex(ChromeTokens::from(resolved).plateFace), QStringLiteral("#201d18"));
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
    writePack(bundled.path(), QStringLiteral("violet-pulse"), QStringLiteral(R"({
      "formatVersion": 1, "id": "violet-pulse", "name": "Violet Pulse", "author": "Tramp",
      "extends": "builtin",
      "colors": { "phosphor": { "default": "#c77dff", "hot": "#ebc8ff", "dim": "#6b3a8a", "deep": "#2e1840" } }
    })"));
    TrampSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), bundled.path(), settings);
    REQUIRE(skins.catalog().size() >= 2);
    REQUIRE(skins.activate(QStringLiteral("violet-pulse"), settings));
    REQUIRE_EQ(settings.activeSkinId, QStringLiteral("violet-pulse"));
    REQUIRE_EQ(hex(skins.tokens().phos), QStringLiteral("#c77dff"));
    REQUIRE(QDir(QDir(support.path()).filePath(QStringLiteral("skins/violet-pulse"))).exists());
  }

  {
    QTemporaryDir support;
    TrampSettings settings;
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
    TrampSettings settings;
    SkinController skins;
    skins.bootstrap(support.path(), {}, settings);
    REQUIRE(skins.installDirectory(QDir(incoming.path()).filePath(QStringLiteral("neon-cyan")),
                                   [](const tramp::SkinConflict&) {
                                     return SkinConflictChoice::cancel;
                                   }));
    REQUIRE(skins.activate(QStringLiteral("neon-cyan"), settings));
    REQUIRE_EQ(hex(skins.tokens().phos), QStringLiteral("#112233"));
  }

  if (gFails != 0) {
    std::fprintf(stderr, "%d assertion(s) failed\n", gFails);
    return 1;
  }
  std::puts("look_test: ok");
  return 0;
}
