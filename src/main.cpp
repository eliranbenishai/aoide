#include "app_icon.h"
#include "chrome_bodies.h"
#include "chrome_paint.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "mockup_draw.h"
#include "native_file_dialog.h"
#include "session.h"
#include "session_view.h"
#include "support_dir.h"
#include "title_chrome.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"
#include "tramp_version.h"
#include "window_spec.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSet>
#include <QShortcut>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>
#include <algorithm>
#include <clocale>
#include <cstdio>
#include <functional>
#include <vector>

namespace {

QString dumpName(tramp::WindowId id) {
  switch (id) {
    case tramp::WindowId::main:
      return QStringLiteral("main_player_window");
    case tramp::WindowId::equalizer:
      return QStringLiteral("equalizer_window");
    case tramp::WindowId::playlist:
      return QStringLiteral("playlist_window");
    case tramp::WindowId::settings:
      return QStringLiteral("settings_window");
    case tramp::WindowId::about:
      return QStringLiteral("about_window");
  }
  return QStringLiteral("window");
}

int dumpChrome(const QString& dirPath) {
  QDir().mkpath(dirPath);
  tramp::loadTrampFonts();
  QImage logo = tramp::loadTrampLogo();
  const tramp::SessionView golden = tramp::goldenDemoView();

  auto shoot = [&](const tramp::WindowSpec& spec, const tramp::SessionView& view,
                   const QString& name) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto title = tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);
    tramp::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo, view);
    p.end();
    const QRgb tl = img.pixel(0, 0);
    const QRgb tr = img.pixel(img.width() - 1, 0);
    if (qAlpha(tl) != 0 || qAlpha(tr) != 0) {
      std::fprintf(stderr, "dump-chrome: square title corners in %s (tl a=%d tr a=%d)\n",
                   qPrintable(name), qAlpha(tl), qAlpha(tr));
      return false;
    }
    return img.save(QDir(dirPath).filePath(name + QStringLiteral(".png")));
  };

  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    if (!shoot(spec, golden, dumpName(spec.id))) return 1;
    // Collapsing the collection is persisted, so a listener can spend every
    // session in it, and it lays the panel out differently. Dumping only the
    // default state left that layout with nothing watching it.
    if (spec.id == tramp::WindowId::playlist) {
      tramp::SessionView collapsed = golden;
      collapsed.collectionCollapsed = true;
      if (!shoot(spec, collapsed, dumpName(spec.id) + QStringLiteral("_collapsed"))) return 1;

      // The demo list is thirteen rows against a default well that shows
      // thirteen, so nothing in the pictures above can overflow. At the size
      // the panel clamps to it does, which is what puts the track scrollbar,
      // its thumb, and the row the well's bottom edge clips under the gate.
      // A disabled row of each kind rides along: both paint faint, and neither
      // had a picture either.
      tramp::WindowSpec smallest = spec;
      smallest.logicalSize = tramp::kPlaylistMinWithCollection;
      tramp::SessionView clamped = golden;
      clamped.collectionWidth = tramp::kPlaylistCollectionMinWidth;
      clamped.trackScroll = 2;
      clamped.tracks[3].disabled = true;
      clamped.collection[2].disabled = true;
      // A disabled track is left out of both footer readouts.
      clamped.playlistTrackCount = golden.playlistTrackCount - 1;
      clamped.playlistTotalMs = golden.playlistTotalMs - 243000;
      if (!shoot(smallest, clamped, dumpName(spec.id) + QStringLiteral("_clamped"))) return 1;
    }
    // The Skins tab shares no pixel with General: its list, scrollbar, four
    // buttons and error line are a pane of their own, so it needs a picture of
    // its own. The list is longer than the viewport on purpose — a catalogue
    // that fits would leave the scrollbar unphotographed a second time.
    if (spec.id == tramp::WindowId::settings) {
      tramp::SessionView skins = golden;
      skins.settingsTab = 1;
      skins.skins = {
          {QStringLiteral("builtin"), QStringLiteral("Built-in"),
           QStringLiteral("Proxima Magnifica")},
          {QStringLiteral("copper-rain"), QStringLiteral("Copper Rain"),
           QStringLiteral("Velvet Static")},
          {QStringLiteral("dusk-arcade"), QStringLiteral("Dusk Arcade"),
           QStringLiteral("Halogen Youth")},
          {QStringLiteral("fluorescent-hymn"), QStringLiteral("Fluorescent Hymn"),
           QStringLiteral("Nightbus Choir")},
          {QStringLiteral("green-screen"), QStringLiteral("Green Screen"), {}},
          {QStringLiteral("long-wave"), QStringLiteral("Long Wave"),
           QStringLiteral("Motel Tapes")},
          {QStringLiteral("night-bus"), QStringLiteral("Night Bus"),
           QStringLiteral("Moth & Marrow")},
          {QStringLiteral("parking-garage"), QStringLiteral("Parking Garage"),
           QStringLiteral("Aurora Kiosk")},
          {QStringLiteral("slow-dial"), QStringLiteral("Slow Dial"),
           QStringLiteral("The Brass Cassini")},
      };
      skins.activeSkinId = QStringLiteral("dusk-arcade");
      skins.skinsScroll = 24;
      skins.skinsError =
          QStringLiteral("nightbus-choir.zip: no skin.json at the archive root, so nothing "
                         "was installed.");
      if (!shoot(spec, skins, dumpName(spec.id) + QStringLiteral("_skins"))) return 1;
    }
  }
  return 0;
}

int benchChrome() {
#ifdef __OPTIMIZE__
  constexpr bool optimized = true;
#else
  constexpr bool optimized = false;
#endif
  tramp::loadTrampFonts();
  QImage logo = tramp::loadTrampLogo();
  tramp::SessionView view = tramp::goldenDemoView();
  const tramp::WindowSpec spec = tramp::windowSpecs().front();
  const auto title = tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);

  auto paintPass = [&](tramp::BodyPaint pass) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    tramp::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo, view, pass);
    p.end();
  };

  paintPass(tramp::BodyPaint::full);

  QElapsedTimer timer;
  constexpr int kFull = 8;
  timer.start();
  for (int i = 0; i < kFull; ++i) paintPass(tramp::BodyPaint::full);
  const qint64 fullNs = timer.nsecsElapsed();

  timer.restart();
  paintPass(tramp::BodyPaint::chassis);
  const qint64 chassisNs = timer.nsecsElapsed();

  constexpr int kLive = 30;
  timer.restart();
  for (int i = 0; i < kLive; ++i) paintPass(tramp::BodyPaint::live);
  const qint64 liveNs = timer.nsecsElapsed();

  const double fullMs = (fullNs / 1e6) / kFull;
  const double chassisMs = chassisNs / 1e6;
  const double liveMs = (liveNs / 1e6) / kLive;
  std::fprintf(stdout,
               "chrome bench: full %.2f ms/frame, chassis %.2f ms, live %.2f ms/frame\n",
               fullMs, chassisMs, liveMs);

  auto paintSpec = [&](const tramp::WindowSpec& s) {
    QImage img(s.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto t = tramp::TitleChromeLayout::forWindow(s.id, s.logicalSize);
    tramp::paintMockupWindow(p, s.logicalSize, s.id, t, &logo, view);
    p.end();
  };

  const auto specs = tramp::windowSpecs();
  paintSpec(specs[1]);
  constexpr int kEq = 8;
  timer.restart();
  for (int i = 0; i < kEq; ++i) paintSpec(specs[1]);
  const double eqMs = (timer.nsecsElapsed() / 1e6) / kEq;

  for (const auto& s : specs) paintSpec(s);
  constexpr int kAll = 4;
  timer.restart();
  for (int i = 0; i < kAll; ++i) {
    for (const auto& s : specs) paintSpec(s);
  }
  const double allMs = (timer.nsecsElapsed() / 1e6) / kAll;

  const tramp::WindowSpec eqSpec = specs[1];
  const auto eqTitle = tramp::TitleChromeLayout::forWindow(eqSpec.id, eqSpec.logicalSize);
  auto paintEqPass = [&](tramp::BodyPaint pass) {
    QImage img(eqSpec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    tramp::paintMockupWindow(p, eqSpec.logicalSize, eqSpec.id, eqTitle, &logo, view, pass);
    p.end();
  };
  paintEqPass(tramp::BodyPaint::live);
  timer.restart();
  for (int i = 0; i < kLive; ++i) paintEqPass(tramp::BodyPaint::live);
  const double eqLiveMs = (timer.nsecsElapsed() / 1e6) / kLive;
  std::fprintf(stdout,
               "chrome bench: eq %.2f ms/frame, eq-live %.2f ms/frame, all-five %.2f ms/frame\n",
               eqMs, eqLiveMs, allMs);

  // The chrome is CPU-rasterised every frame, so an unoptimised build does not
  // merely benchmark badly — it drags at a few frames per second. Catch that
  // here rather than in a listener's hands.
  if (!optimized) {
    std::fprintf(stderr,
                 "FAIL built without optimisation; chrome paint is ~25x slower. "
                 "Build with -O2 (build.sh) or a CMake Release/RelWithDebInfo type.\n");
    return 1;
  }
  constexpr double kFullBudgetMs = 120.0;
  if (fullMs > kFullBudgetMs) {
    std::fprintf(stderr, "FAIL full paint %.2f ms/frame exceeds %.0f ms budget\n", fullMs,
                 kFullBudgetMs);
    return 1;
  }
  if (liveMs > 8.0) {
    std::fprintf(stderr, "FAIL live paint %.2f ms/frame exceeds 8 ms budget\n", liveMs);
    return 1;
  }
  if (fullMs > 4.0 && liveMs * 4.0 > fullMs) {
    std::fprintf(stderr, "FAIL live is not 4x cheaper than full (%.2f vs %.2f)\n", liveMs,
                 fullMs);
    return 1;
  }
  if (eqLiveMs > 8.0) {
    std::fprintf(stderr, "FAIL eq live paint %.2f ms/frame exceeds 8 ms budget\n", eqLiveMs);
    return 1;
  }
  return 0;
}

const char* kValueFlags[] = {"--dump-chrome", "--bench-drag", "--bench-moves", "--bench-visible",
                             "--bench-tracks"};

/// What `chromeChanged` costs before a single pixel is drawn: one snapshot of
/// the whole session, rebuilt from scratch and handed to all five panels. The
/// counterpart to `HostWindow::PaintStats::chassisBuilds`, which counts what
/// the panels then throw away.
struct SnapshotMeter {
  int builds = 0;
  qint64 nanos = 0;

  void reset() { *this = {}; }
  double totalMs() const { return nanos / 1e6; }
  double eachMs() const { return builds ? totalMs() / builds : 0.0; }
};

QStringList launchFiles(const QStringList& args) {
  QStringList files;
  bool skipNext = false;
  for (int i = 1; i < args.size(); ++i) {
    if (skipNext) {
      skipNext = false;
      continue;
    }
    const QString& a = args.at(i);
    bool takesValue = false;
    for (const char* flag : kValueFlags) {
      if (a == QLatin1String(flag)) takesValue = true;
    }
    if (takesValue) {
      skipNext = true;
      continue;
    }
    if (a.startsWith(QLatin1Char('-'))) continue;
    files << a;
  }
  return files;
}

// ---------------------------------------------------------------------------
// --bench-drag: synthetic title-bar drag through the real event path.
//
// Posts press / N moves / release at the dragged panel's title bar and flushes
// the event loop after each move, so every repaint the drag causes is paid
// inside the measured interval. Reports wall time per move plus which panels
// repainted. Runs offscreen (client cost only) or on a live compositor
// (client cost plus surface commit back-pressure).
// ---------------------------------------------------------------------------

struct DragBenchOptions {
  bool on = false;
  bool resize = false;
  tramp::WindowId panel = tramp::WindowId::main;
  int moves = 60;
  bool setVisible = false;
  QSet<tramp::WindowId> visible;
};

bool panelFromName(const QString& name, tramp::WindowId* out) {
  const QString key = name.toLower();
  if (key == QLatin1String("main")) *out = tramp::WindowId::main;
  else if (key == QLatin1String("eq") || key == QLatin1String("equalizer"))
    *out = tramp::WindowId::equalizer;
  else if (key == QLatin1String("pl") || key == QLatin1String("playlist"))
    *out = tramp::WindowId::playlist;
  else if (key == QLatin1String("settings")) *out = tramp::WindowId::settings;
  else if (key == QLatin1String("about")) *out = tramp::WindowId::about;
  else return false;
  return true;
}

QString panelName(tramp::WindowId id) {
  switch (id) {
    case tramp::WindowId::main: return QStringLiteral("main");
    case tramp::WindowId::equalizer: return QStringLiteral("eq");
    case tramp::WindowId::playlist: return QStringLiteral("playlist");
    case tramp::WindowId::settings: return QStringLiteral("settings");
    case tramp::WindowId::about: return QStringLiteral("about");
  }
  return QStringLiteral("?");
}

DragBenchOptions parseDragBench(const QStringList& args) {
  DragBenchOptions opts;
  if (args.contains(QStringLiteral("--bench-resize"))) {
    opts.on = true;
    opts.resize = true;
    opts.panel = tramp::WindowId::playlist;
  }
  const int at = args.indexOf(QStringLiteral("--bench-drag"));
  if (at < 0 && !opts.on) return opts;
  opts.on = true;
  if (at >= 0 && at + 1 < args.size()) panelFromName(args.at(at + 1), &opts.panel);
  const int movesAt = args.indexOf(QStringLiteral("--bench-moves"));
  if (movesAt >= 0 && movesAt + 1 < args.size()) {
    const int n = args.at(movesAt + 1).toInt();
    if (n > 0) opts.moves = n;
  }
  const int visAt = args.indexOf(QStringLiteral("--bench-visible"));
  if (visAt >= 0 && visAt + 1 < args.size()) {
    opts.setVisible = true;
    for (const QString& part : args.at(visAt + 1).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
      tramp::WindowId id = tramp::WindowId::main;
      if (panelFromName(part.trimmed(), &id)) opts.visible.insert(id);
    }
  }
  return opts;
}

void pumpFor(int ms) {
  QElapsedTimer clock;
  clock.start();
  do {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  } while (clock.elapsed() < ms);
}

int runDragBench(const DragBenchOptions& opts, tramp::TrampSession& session, HostShell& shell,
                 const std::vector<HostWindow*>& windows, SnapshotMeter& snapshots) {
  pumpFor(700);  // let the host map and the first full paints settle

  if (opts.setVisible) {
    for (HostWindow* w : windows) {
      if (w->id() == tramp::WindowId::main) continue;
      session.setWindowVisible(w->id(), opts.visible.contains(w->id()));
    }
    pumpFor(400);
  }

  HostWindow* target = nullptr;
  for (HostWindow* w : windows) {
    if (w->id() == opts.panel) target = w;
  }
  if (!target || !target->isVisible()) {
    std::fprintf(stderr, "bench-drag: panel %s is not visible\n", qPrintable(panelName(opts.panel)));
    return 1;
  }

  QStringList shown;
  for (HostWindow* w : windows) {
    if (w->isVisible()) shown << panelName(w->id());
  }
  const char* tag = opts.resize ? "resize" : "drag";
  std::fprintf(stdout,
               "%s bench: platform=%s panel=%s visible=%s moves=%d zoom=%d%% host=%dx%d dpr=%.2f\n",
               tag, qPrintable(QGuiApplication::platformName()),
               qPrintable(panelName(opts.panel)), qPrintable(shown.join(QLatin1Char('+'))),
               opts.moves, session.zoomPercent(), shell.width(), shell.height(),
               shell.devicePixelRatioF());

  // Title-bar drag zone is left of the button cluster, inside kTitleBar; the
  // playlist resize grip is the bottom-right 18 logical px.
  const QPointF grab = opts.resize
                           ? QPointF(target->width() - 5, target->height() - 5)
                           : QPointF(target->width() * 0.35, target->height() * 0.02 + 4);
  auto sendMouse = [&](QEvent::Type type, QPointF local, Qt::MouseButton button,
                       Qt::MouseButtons buttons) {
    QMouseEvent ev(type, local, target->mapToGlobal(local.toPoint()), button, buttons,
                   Qt::NoModifier);
    QCoreApplication::sendEvent(target, &ev);
  };

  sendMouse(QEvent::MouseButtonPress, grab, Qt::LeftButton, Qt::LeftButton);
  pumpFor(50);

  // Warm-up moves: first paints build the chassis, which must not recur below.
  for (int i = 0; i < 3; ++i) {
    sendMouse(QEvent::MouseMove, grab + QPointF(i + 1, 0), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
  }
  for (HostWindow* w : windows) w->resetPaintStats();
  snapshots.reset();

  QVector<qint64> perMove;
  perMove.reserve(opts.moves);
  QElapsedTimer total;
  total.start();
  for (int i = 0; i < opts.moves; ++i) {
    // Serpentine path so both axes move and the panel stays on the desktop.
    const int leg = i % 40;
    const qreal dx = 4 + (leg < 20 ? leg * 3 : (40 - leg) * 3);
    const qreal dy = (i % 20) - 10;
    QElapsedTimer move;
    move.start();
    sendMouse(QEvent::MouseMove, grab + QPointF(dx, dy), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    perMove.push_back(move.nsecsElapsed());
  }
  const qint64 totalNs = total.nsecsElapsed();
  sendMouse(QEvent::MouseButtonRelease, grab, Qt::LeftButton, Qt::NoButton);

  QVector<qint64> sorted = perMove;
  std::sort(sorted.begin(), sorted.end());
  auto at = [&](double q) {
    if (sorted.isEmpty()) return qint64(0);
    const int idx = qBound(0, int(q * (sorted.size() - 1)), sorted.size() - 1);
    return sorted.at(idx);
  };
  const double meanMs = (totalNs / 1e6) / qMax(1, opts.moves);
  std::fprintf(stdout,
               "%s bench: per-move mean %.2f ms  median %.2f  p95 %.2f  max %.2f  → %.1f fps\n",
               tag, meanMs, at(0.5) / 1e6, at(0.95) / 1e6,
               sorted.isEmpty() ? 0.0 : sorted.last() / 1e6, meanMs > 0 ? 1000.0 / meanMs : 0.0);

  QStringList paints;
  QStringList costs;
  int chassisBuilds = 0;
  for (HostWindow* w : windows) {
    const auto stats = w->paintStats();
    if (stats.paints == 0) continue;
    chassisBuilds += stats.chassisBuilds;
    paints << QStringLiteral("%1=%2").arg(panelName(w->id())).arg(stats.paints);
    const double per = stats.paints;
    costs << QStringLiteral("%1=%2 [layers %3 in %4, blur %5, fonts %6 in %7]")
                 .arg(panelName(w->id()))
                 .arg(stats.nanos / 1e6 / per, 0, 'f', 2)
                 .arg(stats.layerNanos / 1e6 / per, 0, 'f', 2)
                 .arg(stats.layers / stats.paints)
                 .arg(stats.blurNanos / 1e6 / per, 0, 'f', 2)
                 .arg(stats.fontNanos / 1e6 / per, 0, 'f', 2)
                 .arg(stats.fonts / stats.paints);
  }
  std::fprintf(stdout, "%s bench: paints %s  chassis-rebuilds=%d  snapshots=%d\n", tag,
               qPrintable(paints.join(QLatin1Char(' '))), chassisBuilds, snapshots.builds);
  std::fprintf(stdout, "%s bench: paint ms %s\n", tag, qPrintable(costs.join(QLatin1Char(' '))));
  return 0;
}

// ---------------------------------------------------------------------------
// --bench-invalidate: what one interaction costs the five panels.
//
// A title-bar drag publishes no snapshot at all, so `--bench-drag` cannot see
// this. Here each interaction is driven through the session and the loop is
// turned after every step, so the snapshots it builds and the panel rasters
// those snapshots throw away are both paid inside the measured window. The
// number to read is how many panels rebuilt for a change they do not paint.
// ---------------------------------------------------------------------------

/// A playlist long enough for its rows to dominate the snapshot. Every row
/// carries an `#EXTINF` duration and title so nothing needs probing: the bench
/// is measuring interaction, not ingest.
QString writeBenchPlaylist(const QDir& dir, int tracks) {
  QStringList lines{QStringLiteral("#EXTM3U")};
  for (int i = 0; i < tracks; ++i) {
    const QString audio =
        dir.filePath(QStringLiteral("bench-%1.mp3").arg(i, 5, 10, QLatin1Char('0')));
    QFile file(audio);
    if (!file.open(QIODevice::WriteOnly)) return {};
    file.close();
    lines << QStringLiteral("#EXTINF:%1,Bench Artist %2 - Bench Title %2")
                 .arg(120 + i % 240)
                 .arg(i);
    lines << audio;
  }
  const QString list = dir.filePath(QStringLiteral("bench.m3u"));
  QFile out(list);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return {};
  out.write(lines.join(QLatin1Char('\n')).toUtf8() + '\n');
  out.close();
  return list;
}

int runInvalidateBench(int trackCount, int reps, tramp::TrampSession& session,
                       const std::vector<HostWindow*>& windows, SnapshotMeter& snapshots) {
  QTemporaryDir tmp;
  if (!tmp.isValid()) {
    std::fprintf(stderr, "bench-invalidate: no temporary directory\n");
    return 1;
  }
  const QString list = writeBenchPlaylist(QDir(tmp.path()), trackCount);
  if (list.isEmpty()) {
    std::fprintf(stderr, "bench-invalidate: could not write the bench playlist\n");
    return 1;
  }

  pumpFor(700);  // let the host map and the first full paints settle
  for (HostWindow* w : windows) {
    if (w->id() != tramp::WindowId::main) session.setWindowVisible(w->id(), true);
  }
  pumpFor(400);
  session.applyDroppedPaths({list}, true);
  // Empty files will not play, but the open tries to; stop it so the spectrum
  // timer is not ticking through the measurement.
  session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::stop, -1, {}}, Qt::NoModifier,
                    {});
  pumpFor(900);

  QStringList shown;
  for (HostWindow* w : windows) {
    if (w->isVisible()) shown << panelName(w->id());
  }
  std::fprintf(stdout,
               "invalidate bench: platform=%s visible=%s tracks=%d reps=%d zoom=%d%%\n",
               qPrintable(QGuiApplication::platformName()),
               qPrintable(shown.join(QLatin1Char('+'))), trackCount, reps, session.zoomPercent());

  auto measure = [&](const char* name, const std::function<void(int)>& step) {
    pumpFor(300);  // an interaction must not be charged for the one before it
    snapshots.reset();
    for (HostWindow* w : windows) w->resetPaintStats();
    for (int i = 0; i < reps; ++i) {
      step(i);
      QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
    QStringList rasters;
    int total = 0;
    int paints = 0;
    qint64 paintNanos = 0;
    for (HostWindow* w : windows) {
      const auto stats = w->paintStats();
      total += stats.chassisBuilds;
      paints += stats.paints;
      paintNanos += stats.nanos;
      rasters << QStringLiteral("%1=%2").arg(panelName(w->id())).arg(stats.chassisBuilds);
    }
    std::fprintf(stdout,
                 "invalidate bench: %-8s snapshots=%-4d %6.2f ms (%.3f each)  rasters=%-4d %s  "
                 "paints=%-4d %.1f ms\n",
                 name, snapshots.builds, snapshots.totalMs(), snapshots.eachMs(), total,
                 qPrintable(rasters.join(QLatin1Char(' '))), paints, paintNanos / 1e6);
  };

  // The floor: whatever the timers cost with nobody touching anything. Every
  // row below is only worth reading against this one.
  measure("idle", [](int) { pumpFor(8); });
  measure("scroll", [&](int) { session.handleWheel(tramp::WindowId::playlist, -120); });
  measure("select", [&](int i) {
    tramp::ChromeHit hit;
    hit.kind = tramp::ChromeHit::Kind::plTrackRow;
    hit.index = i % qMax(1, trackCount);
    session.handleHit(tramp::WindowId::playlist, hit, Qt::NoModifier, QPoint(200, 100));
  });
  measure("mono", [&](int) {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::mono, -1, {}},
                      Qt::NoModifier, {});
  });
  measure("divider", [&](int i) {
    tramp::ChromeHit hit;
    hit.kind = tramp::ChromeHit::Kind::plDivider;
    session.handleDrag(tramp::WindowId::playlist, hit, QPoint(180 + (i % 40) * 6, 200));
  });
  session.handleRelease(tramp::WindowId::playlist);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  tramp::sanitizeInheritedQtPluginPath();
  QApplication app(argc, argv);
  std::setlocale(LC_NUMERIC, "C");
  app.setApplicationName(QStringLiteral("Tramp"));
  app.setApplicationVersion(QLatin1String(TRAMP_VERSION));
  app.setOrganizationName(QStringLiteral("Proxima Magnifica"));
  app.setDesktopFileName(QString::fromLatin1(tramp::kApplicationId));
  app.setWindowIcon(tramp::appIcon());
  app.setQuitOnLastWindowClosed(false);

  const QStringList args = app.arguments();
  const int dumpAt = args.indexOf(QStringLiteral("--dump-chrome"));
  if (dumpAt >= 0) {
    const QString dir =
        dumpAt + 1 < args.size() ? args.at(dumpAt + 1) : QStringLiteral(".");
    return dumpChrome(dir);
  }
  if (args.contains(QStringLiteral("--bench-chrome"))) {
    return benchChrome();
  }
  const DragBenchOptions dragBench = parseDragBench(args);
  const bool invalidateBench = args.contains(QStringLiteral("--bench-invalidate"));
  int benchTracks = 400;
  const int tracksAt = args.indexOf(QStringLiteral("--bench-tracks"));
  if (tracksAt >= 0 && tracksAt + 1 < args.size()) {
    const int n = args.at(tracksAt + 1).toInt();
    if (n > 0) benchTracks = n;
  }
  // A bench run must never write the listener's settings.
  if (dragBench.on || invalidateBench) qputenv("TRAMP_AUTO_QUIT", "1");

  tramp::loadTrampFonts();
  tramp::TrampSession session;

  HostShell hostShell;
  session.setShell(&hostShell);

  std::vector<HostWindow*> windows;
  windows.reserve(5);
  HostWindow* mainWindow = nullptr;
  HostWindow* eqWindow = nullptr;
  HostWindow* plWindow = nullptr;
  HostWindow* settingsWindow = nullptr;
  HostWindow* aboutWindow = nullptr;
  for (const tramp::WindowSpec& spec : tramp::windowSpecs()) {
    auto* window = new HostWindow(spec, &hostShell);
    windows.push_back(window);
    switch (spec.id) {
      case tramp::WindowId::main:
        mainWindow = window;
        break;
      case tramp::WindowId::equalizer:
        eqWindow = window;
        break;
      case tramp::WindowId::playlist:
        plWindow = window;
        break;
      case tramp::WindowId::settings:
        settingsWindow = window;
        break;
      case tramp::WindowId::about:
        aboutWindow = window;
        break;
    }
  }
  session.setWindows(mainWindow, eqWindow, plWindow, settingsWindow, aboutWindow);
  hostShell.setPrimaryPanel(mainWindow);
  mainWindow->setQuitConfirmer([&]() {
    if (!session.confirmQuit()) return true;
    const auto answer = QMessageBox::question(&hostShell, QStringLiteral("Quit Tramp"),
                                              QStringLiteral("Quit Tramp?"));
    return answer == QMessageBox::Yes;
  });

  auto applyZoom = [&](int next) {
    session.setZoomPercent(next);
    for (HostWindow* window : windows) window->setZoomPercent(next);
  };

  SnapshotMeter snapshots;
  auto refresh = [&]() {
    QElapsedTimer clock;
    clock.start();
    const tramp::SessionView view = session.view();
    snapshots.nanos += clock.nsecsElapsed();
    snapshots.builds += 1;
    for (HostWindow* window : windows) window->setSessionView(view);
  };

  QObject::connect(&session, &tramp::TrampSession::chromeChanged, mainWindow, refresh);
  QObject::connect(&session, &tramp::TrampSession::mainChromeChanged, mainWindow, [&]() {
    mainWindow->applyLiveReadouts(session.mainLive());
  });
  QObject::connect(&session, &tramp::TrampSession::zoomChanged, mainWindow, [&](int z) {
    for (HostWindow* window : windows) window->setZoomPercent(z);
  });
  QObject::connect(&session, &tramp::TrampSession::requestShow, mainWindow, [&](tramp::WindowId id) {
    HostWindow* w = nullptr;
    switch (id) {
      case tramp::WindowId::equalizer:
        w = eqWindow;
        break;
      case tramp::WindowId::playlist:
        w = plWindow;
        break;
      case tramp::WindowId::settings:
        w = settingsWindow;
        break;
      case tramp::WindowId::about:
        w = aboutWindow;
        break;
      case tramp::WindowId::main:
        w = mainWindow;
        break;
    }
    if (w) {
      w->show();
      w->raise();
    }
    if (id != tramp::WindowId::settings && settingsWindow->isVisible()) {
      settingsWindow->raise();
    }
    refresh();
  });
  QObject::connect(&session, &tramp::TrampSession::requestHide, mainWindow, [&](tramp::WindowId id) {
    if (id == tramp::WindowId::equalizer) eqWindow->hide();
    if (id == tramp::WindowId::playlist) plWindow->hide();
    if (id == tramp::WindowId::settings) settingsWindow->hide();
    if (id == tramp::WindowId::about) aboutWindow->hide();
    refresh();
  });
  QObject::connect(&session, &tramp::TrampSession::requestRaise, mainWindow, [&](tramp::WindowId id) {
    HostWindow* w = nullptr;
    switch (id) {
      case tramp::WindowId::equalizer:
        w = eqWindow;
        break;
      case tramp::WindowId::playlist:
        w = plWindow;
        break;
      case tramp::WindowId::settings:
        w = settingsWindow;
        break;
      case tramp::WindowId::about:
        w = aboutWindow;
        break;
      case tramp::WindowId::main:
        w = mainWindow;
        break;
    }
    if (w) w->raise();
  });

  for (HostWindow* window : windows) {
    QObject::connect(window, &HostWindow::zoomOutRequested, mainWindow, [&]() {
      applyZoom(tramp::prevZoomPercent(session.zoomPercent()));
    });
    QObject::connect(window, &HostWindow::zoomInRequested, mainWindow, [&]() {
      applyZoom(tramp::nextZoomPercent(session.zoomPercent()));
    });
    QObject::connect(window, &HostWindow::chromePressed, mainWindow,
                     [&, window](tramp::ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
                       session.handleHit(window->id(), hit, mods, logical);
                     });
    QObject::connect(window, &HostWindow::chromeDragged, mainWindow,
                     [&, window](tramp::ChromeHit hit, QPoint logical) {
                       session.handleDrag(window->id(), hit, logical);
                     });
    QObject::connect(window, &HostWindow::chromeReleased, mainWindow,
                     [&, window]() { session.handleRelease(window->id()); });
    QObject::connect(window, &HostWindow::wheelScrolled, mainWindow,
                     [&, window](int d) { session.handleWheel(window->id(), d); });
    QObject::connect(window, &HostWindow::nativeMoved, mainWindow,
                     [&, window](QPoint pos) { session.windowMoved(window->id(), pos, false); });
    QObject::connect(window, &HostWindow::titleDragStarted, mainWindow,
                     [&, window]() { session.titleDragBegan(window->id()); });
    QObject::connect(window, &HostWindow::titleDragFinished, mainWindow,
                     [&, window]() { session.titleDragEnded(window->id()); });
    QObject::connect(window, &HostWindow::nativeResized, mainWindow,
                     [&, window](QSize size) {
                       if (window->id() == tramp::WindowId::playlist) session.playlistResized(size);
                     });
    QObject::connect(window, &HostWindow::filesDropped, mainWindow, [&](const QStringList& paths) {
      session.applyDroppedPaths(paths, false);
    });
    QObject::connect(window, &HostWindow::extraHidden, mainWindow, [&, window]() {
      session.extraClosed(window->id());
      refresh();
    });
    QObject::connect(window, &HostWindow::extraMapped, mainWindow, [&, window]() {
      session.extraWasMapped(window->id());
    });
    QObject::connect(window, &HostWindow::shadedChanged, mainWindow,
                     [&, window](bool shaded) { session.setShaded(window->id(), shaded); });
    QObject::connect(window, &HostWindow::trackActivated, mainWindow,
                     [&](int index) { session.playTrackAt(index); });
  }

  QObject::connect(mainWindow, &HostWindow::aboutToQuit, mainWindow, [&]() { session.persistNow(); });
  QObject::connect(&hostShell, &HostShell::minimizedChanged, mainWindow,
                   [&](bool minimized) { session.mainMinimized(minimized); });
  QObject::connect(&hostShell, &HostShell::activated, mainWindow, [&]() { session.mainActivated(); });

  auto addAppShortcut = [&](const QKeySequence& seq, auto fn) {
    auto* sc = new QShortcut(seq, &hostShell);
    sc->setContext(Qt::ApplicationShortcut);
    QObject::connect(sc, &QShortcut::activated, &hostShell, fn);
  };
  // Space and the play media key toggle. The chrome has separate Play and Pause
  // faces, so routing these at Kind::play only ever started playback.
  addAppShortcut(QKeySequence(Qt::Key_Space), [&]() { session.togglePlayPause(); });
  addAppShortcut(QKeySequence::SelectAll, [&]() { session.selectAllTracks(); });
  addAppShortcut(QKeySequence::Delete, [&]() { session.removeSelectedTracks(); });
  addAppShortcut(QKeySequence(Qt::Key_Backspace), [&]() { session.removeSelectedTracks(); });
  addAppShortcut(QKeySequence(Qt::Key_MediaPlay), [&]() { session.togglePlayPause(); });
  addAppShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause),
                 [&]() { session.togglePlayPause(); });
  addAppShortcut(QKeySequence(Qt::Key_MediaPause), [&]() { session.togglePlayPause(); });
  addAppShortcut(QKeySequence(Qt::Key_MediaStop), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::stop, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaNext), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::next, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaPrevious), [&]() {
    session.handleHit(tramp::WindowId::main, {tramp::ChromeHit::Kind::prev, -1, {}},
                      Qt::NoModifier, {});
  });

  session.bootstrap(launchFiles(args));
  applyZoom(session.zoomPercent());
  refresh();

  hostShell.show();
  session.reapplyWindowFrames();

  if (dragBench.on) {
    return runDragBench(dragBench, session, hostShell, windows, snapshots);
  }
  if (invalidateBench) {
    return runInvalidateBench(benchTracks, 20, session, windows, snapshots);
  }

  if (qEnvironmentVariable("TRAMP_AUTO_QUIT") == QLatin1String("1")) {
    session.persistNow();
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
  }

  const int rc = app.exec();
  session.detachWindows();
  return rc;
}
