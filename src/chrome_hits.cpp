#include "chrome_hits.h"

#include "chrome_layout.h"
#include "mockup_draw.h"
#include "tramp_metrics.h"

#include <QFontMetrics>
#include <QPointF>

namespace tramp {
namespace {

QRectF bodyRect(QSize logical) {
  return QRectF(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
}

ChromeHit hitIf(const QRect& r, QPoint pos, ChromeHit::Kind kind, int index = -1) {
  if (r.contains(pos)) {
    return {kind, index, r};
  }
  return {};
}

ChromeHit hitMain(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = bodyRect(logical);
  const QRect options(int(body.left() + 22), int(body.top() + 18), 26, 26);
  if (auto h = hitIf(options, pos, ChromeHit::Kind::options); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRectF well(body.left() + 96, body.top() + 14, 705, 132);
  if (QRect(well.toRect()).contains(pos)) {
    return {ChromeHit::Kind::timeToggle, -1, well.toRect()};
  }

  const QRectF volRow(body.left() + 22, body.top() + 156, body.width() - 44, 40);
  const QRect mute(int(volRow.left()), int(volRow.top()), 40, 40);
  if (auto h = hitIf(mute, pos, ChromeHit::Kind::mute); h.kind != ChromeHit::Kind::none) return h;
  const qreal volLabelLeft = volRow.left() + 40 + 14;
  const qreal plLeft = volRow.right() - 74;
  const qreal eqLeft = plLeft - 8 - 74;
  const qreal monoLeft = eqLeft - 14 - 86;
  const qreal sliderLeft = volLabelLeft + 34 + 10;
  const qreal sliderRight = monoLeft - 14;
  const QRect volume(int(sliderLeft), int(volRow.center().y() - 7), int(sliderRight - sliderLeft), 14);
  if (auto h = hitIf(volume, pos, ChromeHit::Kind::volume); h.kind != ChromeHit::Kind::none) return h;
  const QRect mono(int(monoLeft), int(volRow.top() + 1), 86, 38);
  if (auto h = hitIf(mono, pos, ChromeHit::Kind::mono); h.kind != ChromeHit::Kind::none) return h;
  const QRect eq(int(eqLeft), int(volRow.top() + 1), 74, 38);
  if (auto h = hitIf(eq, pos, ChromeHit::Kind::eqToggle); h.kind != ChromeHit::Kind::none) return h;
  const QRect pl(int(plLeft), int(volRow.top() + 1), 74, 38);
  if (auto h = hitIf(pl, pos, ChromeHit::Kind::plToggle); h.kind != ChromeHit::Kind::none) return h;

  const QRectF seekRow(body.left() + 22, body.top() + 206, body.width() - 44, 32);
  const QFont stamp = monoFont(14);
  const QFontMetricsF sm(stamp);
  const QString posText = formatClock(view.showElapsed ? view.positionMs
                                                       : qMax<qint64>(0, view.durationMs - view.positionMs));
  const QString durText = formatClock(view.durationMs);
  const qreal posW = sm.horizontalAdvance(view.goldenDemo ? QStringLiteral("2:41") : posText);
  const qreal durW = sm.horizontalAdvance(view.goldenDemo ? QStringLiteral("5:47") : durText);
  const QRect seek(int(seekRow.left() + posW + 14), int(seekRow.center().y() - 8),
                   int(seekRow.width() - posW - durW - 28), 16);
  if (auto h = hitIf(seek, pos, ChromeHit::Kind::seek); h.kind != ChromeHit::Kind::none) return h;

  const QRectF playRow(body.left() + 22, body.top() + 246, body.width() - 44, 50);
  qreal x = playRow.left();
  auto place = [&](qreal w, ChromeHit::Kind kind) -> ChromeHit {
    const QRect r(int(x), int(playRow.top()), int(w), 50);
    x += w + 6;
    return hitIf(r, pos, kind);
  };
  if (auto h = place(66, ChromeHit::Kind::prev); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = place(78, ChromeHit::Kind::play); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = place(66, ChromeHit::Kind::pause); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = place(66, ChromeHit::Kind::stop); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = place(66, ChromeHit::Kind::next); h.kind != ChromeHit::Kind::none) return h;
  x += 10;
  if (auto h = place(66, ChromeHit::Kind::eject); h.kind != ChromeHit::Kind::none) return h;
  const qreal shuffleW = toggleBtnWidth(QStringLiteral("SHUFFLE"));
  const qreal repeatW = toggleBtnWidth(QStringLiteral("REPEAT"));
  const QRect repeat(int(playRow.right() - repeatW), int(playRow.top()), int(repeatW), 50);
  const QRect shuffle(int(repeat.left() - 6 - shuffleW), int(playRow.top()), int(shuffleW), 50);
  if (auto h = hitIf(shuffle, pos, ChromeHit::Kind::shuffle); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = hitIf(repeat, pos, ChromeHit::Kind::repeat); h.kind != ChromeHit::Kind::none) return h;
  return {};
}

ChromeHit hitEq(QSize logical, QPoint pos) {
  const QRectF body = bodyRect(logical);
  const qreal onW = labelBtnWidth(QStringLiteral("ON"));
  const qreal autoW = labelBtnWidth(QStringLiteral("AUTO"));
  const qreal presetsW = labelBtnWidth(QStringLiteral("PRESETS"), 16, 22);
  qreal hx = body.left() + 22;
  const qreal hy = body.top() + 16;
  const QRect on(int(hx), int(hy), int(onW), 38);
  if (auto h = hitIf(on, pos, ChromeHit::Kind::eqOn); h.kind != ChromeHit::Kind::none) return h;
  hx += onW + 8;
  const QRect autoBtn(int(hx), int(hy), int(autoW), 38);
  if (auto h = hitIf(autoBtn, pos, ChromeHit::Kind::eqAuto); h.kind != ChromeHit::Kind::none) return h;
  hx += autoW + 8;
  const QRect presets(int(hx), int(hy), int(presetsW), 38);
  if (auto h = hitIf(presets, pos, ChromeHit::Kind::eqPresets); h.kind != ChromeHit::Kind::none) {
    return h;
  }

  const QRectF bandRow(body.left() + 22, body.top() + 92, body.width() - 44, 196);
  qreal x = bandRow.left() + 44;
  for (int i = 0; i < 11; ++i) {
    const qreal w = i == 0 ? 62 : 50;
    const QRect band(int(x), int(bandRow.top() + 18), int(w), 148);
    if (band.contains(pos)) {
      return {i == 0 ? ChromeHit::Kind::eqPreamp : ChromeHit::Kind::eqBand, i == 0 ? -1 : i - 1,
              band};
    }
    x += w + (i == 0 ? 16 : 0);
  }
  return {};
}

ChromeHit hitPlaylist(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = bodyRect(logical);
  const qreal collectionW = view.collectionCollapsed ? 0 : view.collectionWidth;
  if (view.collectionCollapsed) {
    const QRect reopen(int(body.left() + 4), int(body.top() + 12), 14, 56);
    if (auto h = hitIf(reopen, pos, ChromeHit::Kind::plCollapse); h.kind != ChromeHit::Kind::none) {
      return h;
    }
  } else {
    const QRectF collection(body.left(), body.top(), collectionW, body.height());
    const QRectF colInner = collection.adjusted(12, 12, -6, -12);
    const QRect collapse(int(colInner.right() - 24), int(colInner.top()), 24, 20);
    if (auto h = hitIf(collapse, pos, ChromeHit::Kind::plCollapse); h.kind != ChromeHit::Kind::none) {
      return h;
    }
    const QRectF colWell(colInner.left(), colInner.top() + 30, colInner.width(),
                         colInner.height() - 30 - 8 - 24);
    const int n = view.goldenDemo ? 3 : view.collection.size();
    for (int i = 0; i < n; ++i) {
      const QRect row(int(colWell.left()), int(colWell.top() + 4 + i * 26), int(colWell.width()), 26);
      if (auto h = hitIf(row, pos, ChromeHit::Kind::plCollectionRow, i);
          h.kind != ChromeHit::Kind::none) {
        return h;
      }
    }
    qreal cx = colInner.left();
    const qreal cy = colInner.bottom() - 24;
    const ChromeHit::Kind kinds[] = {ChromeHit::Kind::plAddCollection, ChromeHit::Kind::plCreate,
                                     ChromeHit::Kind::plRename, ChromeHit::Kind::plRemoveCollection};
    for (auto kind : kinds) {
      const QRect r(int(cx), int(cy), 30, 24);
      if (auto h = hitIf(r, pos, kind); h.kind != ChromeHit::Kind::none) return h;
      cx += 36;
    }
    const QRect divider(int(collection.right()), int(collection.top()), 8, int(collection.height()));
    if (auto h = hitIf(divider, pos, ChromeHit::Kind::plDivider); h.kind != ChromeHit::Kind::none) {
      return h;
    }
  }

  const QRectF tracks = playlistTracksPane(body, collectionW);
  const QRectF trackInner = playlistTrackInner(tracks);
  const QRectF listRow = playlistListRowRect(trackInner);
  const int rows = view.goldenDemo ? 13 : view.tracks.size();
  const QRectF listWell = playlistListWell(listRow, rows);
  const int first = view.trackScroll;
  const int visible = playlistVisibleRows(listWell.height()) + 1;
  for (int i = 0; i < visible && first + i < rows; ++i) {
    const QRect row(int(listWell.left()),
                    int(listWell.top() + kPlaylistRowPadTop + i * kPlaylistRowStride),
                    int(listWell.width()), int(kPlaylistRowStride));
    if (auto h = hitIf(row, pos, ChromeHit::Kind::plTrackRow, first + i);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
  }

  const QRectF footer(trackInner.left(), trackInner.bottom() - kPlaylistFooterH, trackInner.width(),
                      kPlaylistFooterH);
  const QRectF plateInner = QRectF(footer.left(), footer.top(), footer.width(), 74).adjusted(12, 10, -12, -10);
  qreal fx = plateInner.left();
  const qreal fy = plateInner.top();
  auto fbtn = [&](qreal w, ChromeHit::Kind kind) {
    const QRect r(int(fx), int(fy), int(w), 52);
    fx += w + 8;
    return hitIf(r, pos, kind);
  };
  if (auto h = fbtn(52, ChromeHit::Kind::plAdd); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = fbtn(52, ChromeHit::Kind::plRemove); h.kind != ChromeHit::Kind::none) return h;
  fx += 6 + 1 + 14;
  if (auto h = fbtn(52, ChromeHit::Kind::plSort); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = fbtn(52, ChromeHit::Kind::plOptions); h.kind != ChromeHit::Kind::none) return h;
  const qreal totalW = 120;
  const qreal transport = 52 * 3 + 16;
  fx = plateInner.right() - totalW - 8 - transport;
  if (auto h = fbtn(52, ChromeHit::Kind::plPrev); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = fbtn(52, ChromeHit::Kind::plPlay); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = fbtn(52, ChromeHit::Kind::plNext); h.kind != ChromeHit::Kind::none) return h;

  const QRect grip(logical.width() - 18, logical.height() - 18, 18, 18);
  if (auto h = hitIf(grip, pos, ChromeHit::Kind::plResize); h.kind != ChromeHit::Kind::none) return h;
  return {};
}

ChromeHit hitSettings(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = bodyRect(logical);
  if (auto h = hitIf(QRect(int(body.left()), int(body.top()), 108, 42), pos,
                     ChromeHit::Kind::settingsGeneral);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(QRect(int(body.left()), int(body.top() + 42), 108, 42), pos,
                     ChromeHit::Kind::settingsSkins);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRect reset(int(body.left() + 12), int(body.bottom() - 36), 160, 24);
  if (auto h = hitIf(reset, pos, ChromeHit::Kind::settingsReset); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRectF pane = settingsPane(logical);
  if (view.settingsTab == 1) {
    const qreal btnY = pane.bottom() - kSkinsBtnStackH;
    if (auto h = hitIf(QRect(int(pane.left() + 12), int(btnY), 148, 26), pos,
                       ChromeHit::Kind::settingsInstallZip);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
    if (auto h = hitIf(QRect(int(pane.left() + 168), int(btnY), 160, 26), pos,
                       ChromeHit::Kind::settingsInstallFolder);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
    if (auto h = hitIf(QRect(int(pane.left() + 12), int(btnY + 30), 148, 26), pos,
                       ChromeHit::Kind::settingsSkinsFolder);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
    if (auto h = hitIf(QRect(int(pane.left() + 168), int(btnY + 30), 160, 26), pos,
                       ChromeHit::Kind::settingsResetSkinsFolder);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
    const QRectF viewport = skinsListViewport(pane);
    if (viewport.contains(QPointF(pos))) {
      for (int i = 0; i < view.skins.size(); ++i) {
        const QRectF row = skinsListRow(viewport, i, view.skinsScroll);
        if (auto h = hitIf(row.toRect(), pos, ChromeHit::Kind::settingsSkinRow, i);
            h.kind != ChromeHit::Kind::none) {
          return h;
        }
      }
    }
    return {};
  }
  const ChromeHit::Kind toggles[] = {ChromeHit::Kind::settingsResume, ChromeHit::Kind::settingsConfirm,
                                     ChromeHit::Kind::settingsScroll, ChromeHit::Kind::settingsMinimize};
  for (int i = 0; i < 4; ++i) {
    const QRect row(int(pane.left() + 16), int(pane.top() + 12 + i * 36), int(pane.width() - 32), 32);
    if (auto h = hitIf(row, pos, toggles[i]); h.kind != ChromeHit::Kind::none) return h;
  }
  qreal sx = pane.left() + 16;
  const ChromeHit::Kind snaps[] = {ChromeHit::Kind::settingsSnapOff, ChromeHit::Kind::settingsSnapNormal,
                                   ChromeHit::Kind::settingsSnapStrong};
  for (int i = 0; i < 3; ++i) {
    const QRect r(int(sx), int(pane.top() + 194), 88, 28);
    if (auto h = hitIf(r, pos, snaps[i]); h.kind != ChromeHit::Kind::none) return h;
    sx += 96;
  }
  return {};
}

ChromeHit hitAbout(QSize logical, QPoint pos) {
  const QRectF body = bodyRect(logical);
  const QRectF inner = body.adjusted(16, 14, -16, -14);
  const QRectF plate(inner.left(), inner.bottom() - 48, inner.width(), 48);
  const QRect web(int(plate.right() - 13 - 110), int(plate.center().y() - 12), 110, 24);
  if (auto h = hitIf(web, pos, ChromeHit::Kind::aboutWeb); h.kind != ChromeHit::Kind::none) return h;
  return {};
}

}  // namespace

ChromeHit hitTest(WindowId id, QSize logical, QPoint pos, const SessionView& view) {
  switch (id) {
    case WindowId::main:
      return hitMain(logical, pos, view);
    case WindowId::equalizer:
      return hitEq(logical, pos);
    case WindowId::playlist:
      return hitPlaylist(logical, pos, view);
    case WindowId::settings:
      return hitSettings(logical, pos, view);
    case WindowId::about:
      return hitAbout(logical, pos);
  }
  return {};
}

QRect mainOptionsHit(QSize logical) {
  Q_UNUSED(logical);
  return QRect(22, kTitleBar + 18, 26, 26);
}

QRect mainEqHit(QSize logical) {
  const QRectF volRow(22, kTitleBar + 156, logical.width() - 44, 40);
  return QRect(int(volRow.right() - 74 - 8 - 74), int(volRow.top() + 1), 74, 38);
}

QRect mainPlHit(QSize logical) {
  const QRectF volRow(22, kTitleBar + 156, logical.width() - 44, 40);
  return QRect(int(volRow.right() - 74), int(volRow.top() + 1), 74, 38);
}

}  // namespace tramp
