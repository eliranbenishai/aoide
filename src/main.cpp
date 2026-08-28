#include "app_icon.h"
#include "chrome_bodies.h"
#include "chrome_paint.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "mockup_draw.h"
#include "native_file_dialog.h"
#include "panel_registry.h"
#include "session.h"
#include "session_view.h"
#include "support_dir.h"
#include "title_chrome.h"
#include "aoide_fonts.h"
#include "aoide_metrics.h"
#include "aoide_version.h"
#include "window_spec.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileOpenEvent>
#include <QGuiApplication>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSet>
#include <QShortcut>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVector>
#ifdef Q_OS_MACOS
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#endif
#include <algorithm>
#include <clocale>
#include <cstdio>
#include <functional>
#include <optional>
#include <vector>

namespace {

QString dumpName(aoide::WindowId id) { return aoide::panelSpec(id).dumpName; }

int dumpChrome(const QString& dirPath) {
  QDir().mkpath(dirPath);
  aoide::loadAoideFonts();
  QImage logo = aoide::loadAoideLogo();
  const aoide::SessionView golden = aoide::goldenDemoView();

  auto shoot = [&](const aoide::WindowSpec& spec, const aoide::SessionView& view,
                   const QString& name) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto title = aoide::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);
    aoide::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo, view);
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

  for (const aoide::WindowSpec& spec : aoide::windowSpecs()) {
    if (!shoot(spec, golden, dumpName(spec.id))) return 1;
    if (spec.id == aoide::WindowId::main) {
      // The pairing host withdraws zoom-in at the default three-panel stack.
      // The golden dump keeps the buttons live so that picture stays the
      // demo; this is the dead zoom-in face that dump has to watch.
      aoide::SessionView zoomDisabled = golden;
      zoomDisabled.zoomInEnabled = false;
      if (!shoot(spec, zoomDisabled, dumpName(spec.id) + QStringLiteral("_zoom_disabled")))
        return 1;
    }
    // Collapsing the collection is persisted, so a listener can spend every
    // session in it, and it lays the panel out differently. Dumping only the
    // default state left that layout with nothing watching it.
    if (spec.id == aoide::WindowId::playlist) {
      aoide::SessionView collapsed = golden;
      collapsed.collectionCollapsed = true;
      if (!shoot(spec, collapsed, dumpName(spec.id) + QStringLiteral("_collapsed"))) return 1;

      // The default well shows thirteen rows. At the size the panel clamps to it
      // shows fewer, which is what puts the track scrollbar, its thumb, and the
      // row the well's bottom edge clips under the gate. A disabled row of each
      // kind rides along: both paint faint, and neither had a picture either.
      aoide::WindowSpec smallest = spec;
      smallest.logicalSize = aoide::kPlaylistMinWithCollection;
      aoide::SessionView clamped = golden;
      clamped.collectionWidth = aoide::kPlaylistCollectionMinWidth;
      clamped.trackScroll = 2;
      clamped.tracks[3].disabled = true;
      clamped.collection[2].disabled = true;
      // A disabled track is left out of both footer readouts. The subtraction is
      // tracks[3]'s own length, so it tracks the demo list rather than a literal
      // that silently stops matching it.
      clamped.playlistTrackCount = golden.playlistTrackCount - 1;
      clamped.playlistTotalMs = golden.playlistTotalMs - 220000;
      if (!shoot(smallest, clamped, dumpName(spec.id) + QStringLiteral("_clamped"))) return 1;

      // The demo list fills both wells. Ticket 15's empty-state copy lives
      // only when there are no tracks and no saved playlists, with the
      // collection column still open — a different view, not the golden
      // list with words painted over it.
      aoide::SessionView empty;
      empty.goldenDemo = true;
      empty.collectionCollapsed = false;
      if (!shoot(spec, empty, dumpName(spec.id) + QStringLiteral("_empty"))) return 1;
    }
    // Audio paints its own pane; it shares no pixel with General.
    if (spec.id == aoide::WindowId::settings) {
      aoide::SessionView audio = golden;
      audio.settingsTab = 1;
      if (!shoot(spec, audio, dumpName(spec.id) + QStringLiteral("_audio"))) return 1;
    }
    // The Skins panel's matrix, scrollbar, footer glyphs and error line need a
    // picture of their own. The catalogue is longer than the viewport on purpose
    // — a grid that fits would leave the scrollbar unphotographed.
    if (spec.id == aoide::WindowId::skins) {
      aoide::SessionView skins = golden;
      skins.skins = {
          {QStringLiteral("builtin"), QStringLiteral("Aoide"),
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
      for (auto& entry : skins.skins) {
        entry.canRemove =
            entry.id != skins.activeSkinId && entry.id != QLatin1String("builtin");
      }
      skins.skinsScroll = 24;
      skins.skinsError =
          QStringLiteral("nightbus-choir.zip: no skin.json at the archive root, so nothing "
                         "was installed.");
      if (!shoot(spec, skins, dumpName(spec.id))) return 1;
    }
  }
  return 0;
}

int benchChrome() {
// GCC and Clang predefine __OPTIMIZE__ from -O1 up. MSVC predefines nothing
// equivalent, and without a stand-in every Windows build failed this gate while
// painting within a few ms of the Linux one. NDEBUG is that stand-in: every
// Windows build here comes from CMake, which pairs it with /O2 in Release and
// RelWithDebInfo and with /O1 in MinSizeRel, and gives Debug /Od and _DEBUG
// instead. The budgets below do not cover for this: an -O0 build measures ~38 ms
// of full paint against a 120 ms budget and passes, so the macro is the guard.
#if defined(__OPTIMIZE__) || (defined(_MSC_VER) && defined(NDEBUG))
  constexpr bool optimized = true;
#else
  constexpr bool optimized = false;
#endif
  aoide::loadAoideFonts();
  QImage logo = aoide::loadAoideLogo();
  aoide::SessionView view = aoide::goldenDemoView();
  const aoide::WindowSpec spec = aoide::windowSpecs().front();
  const auto title = aoide::TitleChromeLayout::forWindow(spec.id, spec.logicalSize);

  auto paintPass = [&](aoide::BodyPaint pass) {
    QImage img(spec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    aoide::paintMockupWindow(p, spec.logicalSize, spec.id, title, &logo, view, pass);
    p.end();
  };

  paintPass(aoide::BodyPaint::full);

  QElapsedTimer timer;
  constexpr int kFull = 8;
  timer.start();
  for (int i = 0; i < kFull; ++i) paintPass(aoide::BodyPaint::full);
  const qint64 fullNs = timer.nsecsElapsed();

  timer.restart();
  paintPass(aoide::BodyPaint::chassis);
  const qint64 chassisNs = timer.nsecsElapsed();

  constexpr int kLive = 30;
  timer.restart();
  for (int i = 0; i < kLive; ++i) paintPass(aoide::BodyPaint::live);
  const qint64 liveNs = timer.nsecsElapsed();

  const double fullMs = (fullNs / 1e6) / kFull;
  const double chassisMs = chassisNs / 1e6;
  const double liveMs = (liveNs / 1e6) / kLive;
  std::fprintf(stdout,
               "chrome bench: full %.2f ms/frame, chassis %.2f ms, live %.2f ms/frame\n",
               fullMs, chassisMs, liveMs);

  auto paintSpec = [&](const aoide::WindowSpec& s) {
    QImage img(s.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const auto t = aoide::TitleChromeLayout::forWindow(s.id, s.logicalSize);
    aoide::paintMockupWindow(p, s.logicalSize, s.id, t, &logo, view);
    p.end();
  };

  const auto specs = aoide::windowSpecs();
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

  const aoide::WindowSpec eqSpec = specs[1];
  const auto eqTitle = aoide::TitleChromeLayout::forWindow(eqSpec.id, eqSpec.logicalSize);
  auto paintEqPass = [&](aoide::BodyPaint pass) {
    QImage img(eqSpec.logicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    aoide::paintMockupWindow(p, eqSpec.logicalSize, eqSpec.id, eqTitle, &logo, view, pass);
    p.end();
  };
  paintEqPass(aoide::BodyPaint::live);
  timer.restart();
  for (int i = 0; i < kLive; ++i) paintEqPass(aoide::BodyPaint::live);
  const double eqLiveMs = (timer.nsecsElapsed() / 1e6) / kLive;
  std::fprintf(stdout,
               "chrome bench: eq %.2f ms/frame, eq-live %.2f ms/frame, all-six %.2f ms/frame\n",
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
/// the whole session, rebuilt from scratch and handed to all six panels. The
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
  aoide::WindowId panel = aoide::WindowId::main;
  int moves = 60;
  bool setVisible = false;
  QSet<aoide::WindowId> visible;
};

bool panelFromName(const QString& name, aoide::WindowId* out) {
  const auto id = aoide::panelForName(name);
  if (!id) return false;
  *out = *id;
  return true;
}

QString panelName(aoide::WindowId id) { return aoide::panelSpec(id).commandNames.first(); }

DragBenchOptions parseDragBench(const QStringList& args) {
  DragBenchOptions opts;
  if (args.contains(QStringLiteral("--bench-resize"))) {
    opts.on = true;
    opts.resize = true;
    opts.panel = aoide::WindowId::playlist;
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
      aoide::WindowId id = aoide::WindowId::main;
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

int runDragBench(const DragBenchOptions& opts, aoide::AoideSession& session, HostShell& shell,
                 const std::vector<HostWindow*>& windows, SnapshotMeter& snapshots) {
  pumpFor(700);  // let the host map and the first full paints settle

  if (opts.setVisible) {
    for (HostWindow* w : windows) {
      if (w->id() == aoide::WindowId::main) continue;
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

int runInvalidateBench(int trackCount, int reps, aoide::AoideSession& session,
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
    if (w->id() != aoide::WindowId::main) session.setWindowVisible(w->id(), true);
  }
  pumpFor(400);
  session.applyDroppedPaths({list}, true);
  // Empty files will not play, but the open tries to; stop it so the spectrum
  // timer is not ticking through the measurement.
  session.handleHit(aoide::WindowId::main, {aoide::ChromeHit::Kind::stop, -1, {}}, Qt::NoModifier,
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
  measure("scroll", [&](int) { session.handleWheel(aoide::WindowId::playlist, -120); });
  measure("select", [&](int i) {
    aoide::ChromeHit hit;
    hit.kind = aoide::ChromeHit::Kind::plTrackRow;
    hit.index = i % qMax(1, trackCount);
    session.handleHit(aoide::WindowId::playlist, hit, Qt::NoModifier, QPoint(200, 100));
  });
  measure("mono", [&](int) {
    session.handleHit(aoide::WindowId::main, {aoide::ChromeHit::Kind::mono, -1, {}},
                      Qt::NoModifier, {});
  });
  measure("divider", [&](int i) {
    aoide::ChromeHit hit;
    hit.kind = aoide::ChromeHit::Kind::plDivider;
    session.handleDrag(aoide::WindowId::playlist, hit, QPoint(180 + (i % 40) * 6, 200));
  });
  session.handleRelease(aoide::WindowId::playlist);
  return 0;
}

/// macOS delivers Finder "Open With", Dock drops, and reopen-while-running as
/// QFileOpenEvent, which never appears in argv. Those used to be dropped.
class AoideApplication : public QApplication {
 public:
  using QApplication::QApplication;

  QStringList takeQueuedFileOpens() {
    QStringList out;
    queuedFileOpens_.swap(out);
    return out;
  }

  void setFileOpenHandler(std::function<void(const QStringList&)> handler) {
    fileOpenHandler_ = std::move(handler);
    if (fileOpenHandler_ && !queuedFileOpens_.isEmpty()) {
      fileOpenHandler_(takeQueuedFileOpens());
    }
  }

 protected:
  bool event(QEvent* event) override {
    if (event->type() == QEvent::FileOpen) {
      const auto* openEvent = static_cast<const QFileOpenEvent*>(event);
      QString path = openEvent->file();
      if (path.isEmpty()) path = openEvent->url().toLocalFile();
      if (path.isEmpty()) return true;
      if (fileOpenHandler_) {
        fileOpenHandler_({path});
      } else {
        queuedFileOpens_ << path;
      }
      return true;
    }
    return QApplication::event(event);
  }

 private:
  std::function<void(const QStringList&)> fileOpenHandler_;
  QStringList queuedFileOpens_;
};

}  // namespace

int main(int argc, char** argv) {
  aoide::sanitizeInheritedQtPluginPath();
  AoideApplication app(argc, argv);
  std::setlocale(LC_NUMERIC, "C");
  app.setApplicationName(QStringLiteral("Aoide"));
  app.setApplicationVersion(QLatin1String(AOIDE_VERSION));
  app.setOrganizationName(QStringLiteral("Proxima Magnifica"));
  app.setDesktopFileName(QString::fromLatin1(aoide::kApplicationId));
  app.setWindowIcon(aoide::appIcon());
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
  if (dragBench.on || invalidateBench) qputenv("AOIDE_AUTO_QUIT", "1");

  aoide::loadAoideFonts();
  aoide::AoideSession session;

  HostShell hostShell;
  session.setShell(&hostShell);

  std::vector<HostWindow*> windows;
  windows.reserve(aoide::kPanelCount);
  aoide::PanelWindows panels;
  for (const aoide::WindowSpec& spec : aoide::windowSpecs()) {
    auto* window = new HostWindow(spec, &hostShell);
    windows.push_back(window);
    panels.set(spec.id, window);
  }
  HostWindow* mainWindow = panels[aoide::WindowId::main];
  HostWindow* settingsWindow = panels[aoide::WindowId::settings];
  HostWindow* skinsWindow = panels[aoide::WindowId::skins];
  session.setWindows(panels);
  hostShell.setPrimaryPanel(mainWindow);
  mainWindow->setQuitConfirmer([&]() {
    if (!session.confirmQuit()) return true;
    const auto answer = QMessageBox::question(&hostShell, QStringLiteral("Quit Aoide"),
                                              QStringLiteral("Quit Aoide?"));
    return answer == QMessageBox::Yes;
  });

  // Sizes the panels from the step the session took, which is not always the
  // step it was handed: the layout refuses one the display's work area cannot
  // hold. Pushing the requested step here instead would scale every panel's
  // chrome while the layout stayed at the size it kept, which reads as the
  // panels having drifted out of their own frames.
  auto applyZoom = [&](int requested) {
    session.setZoomPercent(requested);
    const int taken = session.zoomPercent();
    for (HostWindow* window : windows) window->setZoomPercent(taken);
  };

  SnapshotMeter snapshots;
  auto refresh = [&]() {
    QElapsedTimer clock;
    clock.start();
    const aoide::SessionView view = session.view();
    snapshots.nanos += clock.nsecsElapsed();
    snapshots.builds += 1;
    for (HostWindow* window : windows) window->setSessionView(view);
  };

  QObject::connect(&session, &aoide::AoideSession::chromeChanged, mainWindow, refresh);
  QObject::connect(&session, &aoide::AoideSession::mainChromeChanged, mainWindow, [&]() {
    mainWindow->applyLiveReadouts(session.mainLive());
  });
  QObject::connect(&session, &aoide::AoideSession::zoomChanged, mainWindow, [&](int z) {
    for (HostWindow* window : windows) window->setZoomPercent(z);
  });
  QObject::connect(&session, &aoide::AoideSession::requestShow, mainWindow, [&](aoide::WindowId id) {
    if (HostWindow* w = panels[id]) {
      w->show();
      w->raise();
    }
    if (id != aoide::WindowId::settings && settingsWindow->isVisible()) {
      settingsWindow->raise();
    }
    if (id != aoide::WindowId::skins && skinsWindow->isVisible()) {
      skinsWindow->raise();
    }
    refresh();
  });
  QObject::connect(&session, &aoide::AoideSession::requestHide, mainWindow, [&](aoide::WindowId id) {
    // Main is the host's reason to exist and cannot be hidden; the layout says
    // so too, so a request naming it is one nothing should have sent.
    if (id != aoide::WindowId::main) {
      if (HostWindow* w = panels[id]) w->hide();
    }
    refresh();
  });
  QObject::connect(&session, &aoide::AoideSession::requestRaise, mainWindow, [&](aoide::WindowId id) {
    if (HostWindow* w = panels[id]) w->raise();
  });

  for (HostWindow* window : windows) {
    // The session is asked for the step rather than told the next rung of the
    // ladder: at either end, and at a step the work area cannot hold, there is
    // no step to take and the press does nothing at all. Handing over a rung
    // that will be refused would work too, but it makes the button's dead
    // presses look like a bug in the setter rather than the absence of a step.
    QObject::connect(window, &HostWindow::zoomOutRequested, mainWindow, [&]() {
      if (const std::optional<int> step = session.zoomStepDown()) applyZoom(*step);
    });
    QObject::connect(window, &HostWindow::zoomInRequested, mainWindow, [&]() {
      if (const std::optional<int> step = session.zoomStepUp()) applyZoom(*step);
    });
    QObject::connect(window, &HostWindow::chromePressed, mainWindow,
                     [&, window](aoide::ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
                       session.handleHit(window->id(), hit, mods, logical);
                     });
    QObject::connect(window, &HostWindow::chromeDragged, mainWindow,
                     [&, window](aoide::ChromeHit hit, QPoint logical) {
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
                       if (window->id() == aoide::WindowId::playlist) session.playlistResized(size);
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
    session.handleHit(aoide::WindowId::main, {aoide::ChromeHit::Kind::stop, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaNext), [&]() {
    session.handleHit(aoide::WindowId::main, {aoide::ChromeHit::Kind::next, -1, {}},
                      Qt::NoModifier, {});
  });
  addAppShortcut(QKeySequence(Qt::Key_MediaPrevious), [&]() {
    session.handleHit(aoide::WindowId::main, {aoide::ChromeHit::Kind::prev, -1, {}},
                      Qt::NoModifier, {});
  });

#ifdef Q_OS_MACOS
  auto* menuBar = new QMenuBar;
  menuBar->setNativeMenuBar(true);
  auto* appMenu = menuBar->addMenu(QStringLiteral("Aoide"));
  auto* aboutAction = appMenu->addAction(QStringLiteral("About Aoide"));
  aboutAction->setMenuRole(QAction::AboutRole);
  QObject::connect(aboutAction, &QAction::triggered, &hostShell, [&]() {
    session.setWindowVisible(aoide::WindowId::about, true);
  });
  auto* settingsAction = appMenu->addAction(QStringLiteral("Settings…"));
  settingsAction->setMenuRole(QAction::PreferencesRole);
  QObject::connect(settingsAction, &QAction::triggered, &hostShell, [&]() {
    session.setWindowVisible(aoide::WindowId::settings, true);
  });
  auto* quitAction = appMenu->addAction(QStringLiteral("Quit Aoide"));
  quitAction->setMenuRole(QAction::QuitRole);
  // Cmd+Q must take the same confirmer as the chrome close box; the role
  // action would otherwise quit without asking.
  QObject::connect(quitAction, &QAction::triggered, &hostShell, [&]() { mainWindow->close(); });
#endif

  QStringList startupFiles = launchFiles(args);
  startupFiles.append(app.takeQueuedFileOpens());
  session.bootstrap(startupFiles);
  applyZoom(session.zoomPercent());
  refresh();
  app.setFileOpenHandler(
      [&](const QStringList& paths) { session.applyDroppedPaths(paths, false); });

  hostShell.show();
  session.reapplyWindowFrames();

  if (dragBench.on) {
    return runDragBench(dragBench, session, hostShell, windows, snapshots);
  }
  if (invalidateBench) {
    return runInvalidateBench(benchTracks, 20, session, windows, snapshots);
  }

  if (qEnvironmentVariable("AOIDE_AUTO_QUIT") == QLatin1String("1")) {
    session.persistNow();
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
  }

  const int rc = app.exec();
  session.detachWindows();
  return rc;
}
