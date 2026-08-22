#include "chrome_hits.h"

#include "chrome_layout.h"
#include "mockup_draw.h"
#include "tramp_metrics.h"

#include <QFontMetrics>
#include <QPointF>

namespace tramp {
namespace {

ChromeHit hitIf(const QRect& r, QPoint pos, ChromeHit::Kind kind, int index = -1) {
  if (r.contains(pos)) {
    return {kind, index, r};
  }
  return {};
}

/// Painted rectangles carry sub-pixel edges — anything sized to its own label
/// does — while hit regions are whole logical pixels. Rounding outwards keeps
/// every pixel a control paints on inside its hit region.
QRect toHitRect(const QRectF& r) { return r.toAlignedRect(); }

ChromeHit hitMain(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = panelBody(logical);
  const MainDisplayRow display = layoutMainDisplay(body);
  if (auto h = hitIf(toHitRect(display.options), pos, ChromeHit::Kind::options);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(display.skins), pos, ChromeHit::Kind::skins);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(display.trackInfo), pos, ChromeHit::Kind::trackInfo);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(display.well), pos, ChromeHit::Kind::timeToggle);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }

  const MainVolumeRow vol = layoutMainVolumeRow(body);
  if (auto h = hitIf(toHitRect(vol.mute), pos, ChromeHit::Kind::mute);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRect volume = sliderHitRect(vol.track, kVolumeThumbH);
  if (auto h = hitIf(volume, pos, ChromeHit::Kind::volume); h.kind != ChromeHit::Kind::none) return h;
  if (auto h = hitIf(toHitRect(vol.mono), pos, ChromeHit::Kind::mono);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(vol.eq), pos, ChromeHit::Kind::eqToggle);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(vol.pl), pos, ChromeHit::Kind::plToggle);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }

  const QFontMetricsF sm(monoFont(14));
  const SeekStamps stamps = mainSeekStamps(view);
  const QRect seek = sliderHitRect(layoutMainSeekRow(body, sm.horizontalAdvance(stamps.elapsed),
                                                     sm.horizontalAdvance(stamps.duration))
                                       .track,
                                   kSeekThumbH);
  if (auto h = hitIf(seek, pos, ChromeHit::Kind::seek); h.kind != ChromeHit::Kind::none) return h;

  const MainTransportRow play =
      layoutMainTransportRow(body, toggleBtnWidth(QStringLiteral("SHUFFLE")),
                             toggleBtnWidth(QStringLiteral("REPEAT")));
  const std::pair<const QRectF&, ChromeHit::Kind> transport[] = {
      {play.prev, ChromeHit::Kind::prev},     {play.play, ChromeHit::Kind::play},
      {play.pause, ChromeHit::Kind::pause},   {play.stop, ChromeHit::Kind::stop},
      {play.next, ChromeHit::Kind::next},     {play.eject, ChromeHit::Kind::eject},
      {play.shuffle, ChromeHit::Kind::shuffle}, {play.repeat, ChromeHit::Kind::repeat},
  };
  for (const auto& [rect, kind] : transport) {
    if (auto h = hitIf(toHitRect(rect), pos, kind); h.kind != ChromeHit::Kind::none) return h;
  }
  return {};
}

ChromeHit hitEq(QSize logical, QPoint pos) {
  const QRectF body = panelBody(logical);
  const EqHeaderRow header = layoutEqHeader(body, labelBtnWidth(QStringLiteral("ON")),
                                            labelBtnWidth(QStringLiteral("AUTO")),
                                            labelBtnWidth(QStringLiteral("PRESETS"), 16, 22));
  if (auto h = hitIf(toHitRect(header.on), pos, ChromeHit::Kind::eqOn);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(header.autoBtn), pos, ChromeHit::Kind::eqAuto);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(toHitRect(header.presets), pos, ChromeHit::Kind::eqPresets);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }

  const QRectF bandRow = eqBandRow(body);
  for (int i = 0; i < kEqBandCount; ++i) {
    const EqBandColumn column = eqBandColumn(bandRow, i);
    if (bandHitRect(column, kEqBandThumbH).contains(pos)) {
      return {i == 0 ? ChromeHit::Kind::eqPreamp : ChromeHit::Kind::eqBand, i == 0 ? -1 : i - 1,
              toHitRect(column.well)};
    }
  }
  return {};
}

ChromeHit hitPlaylist(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = panelBody(logical);
  const qreal collectionW = view.collectionCollapsed ? 0 : view.collectionWidth;
  if (view.collectionCollapsed) {
    if (auto h = hitIf(toHitRect(playlistReopenTab(body)), pos, ChromeHit::Kind::plCollapse);
        h.kind != ChromeHit::Kind::none) {
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
    const int n = int(view.collection.size());
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
  const int rows = int(view.tracks.size());
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
  const QRectF deckInner =
      QRectF(footer.left(), footer.top(), footer.width(), 74).adjusted(12, 10, -12, -10);
  const QString totalText = formatClock(view.playlistTotalMs);
  const qreal totalW = playlistStripTotalWidth(
      textWidth(condensedFont(11, 0.2), QStringLiteral("TOTAL")),
      textWidth(monoFont(18), totalText));
  const auto strip = layoutPlaylistStrip(deckInner, totalW);
  auto hitBtn = [&](const QRectF& r, ChromeHit::Kind kind) {
    return hitIf(r.toRect(), pos, kind);
  };
  if (auto h = hitBtn(strip.add, ChromeHit::Kind::plAdd); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.remove, ChromeHit::Kind::plRemove); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.sort, ChromeHit::Kind::plSort); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.options, ChromeHit::Kind::plOptions); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.prev, ChromeHit::Kind::plPrev); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.play, ChromeHit::Kind::plPlay); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.next, ChromeHit::Kind::plNext); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitBtn(strip.refresh, ChromeHit::Kind::plRefresh); h.kind != ChromeHit::Kind::none) {
    return h;
  }

  const QRect grip(logical.width() - 18, logical.height() - 18, 18, 18);
  if (auto h = hitIf(grip, pos, ChromeHit::Kind::plResize); h.kind != ChromeHit::Kind::none) return h;
  return {};
}

ChromeHit hitSettings(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF body = panelBody(logical);
  if (auto h = hitIf(QRect(int(body.left()), int(body.top()), 108, 42), pos,
                     ChromeHit::Kind::settingsGeneral);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  if (auto h = hitIf(QRect(int(body.left()), int(body.top() + 42), 108, 42), pos,
                     ChromeHit::Kind::settingsAudio);
      h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRect reset(int(body.left() + 12), int(body.bottom() - 36), 160, 24);
  if (auto h = hitIf(reset, pos, ChromeHit::Kind::settingsReset); h.kind != ChromeHit::Kind::none) {
    return h;
  }
  const QRectF pane = settingsPane(logical);
  if (view.settingsTab == 1) {
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

ChromeHit hitSkins(QSize logical, QPoint pos, const SessionView& view) {
  const QRectF pane = skinsPane(logical);
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
  const QRectF track = skinsListScrollTrack(viewport);
  if (skinsListMaxScroll(view.skins.size(), viewport) > 0) {
    if (auto h = hitIf(track.toRect(), pos, ChromeHit::Kind::settingsSkinScroll);
        h.kind != ChromeHit::Kind::none) {
      return h;
    }
  }
  if (viewport.contains(QPointF(pos))) {
    for (int i = 0; i < view.skins.size(); ++i) {
      const QRectF cell = skinsGridCell(viewport, i, view.skinsScroll);
      if (!cell.intersects(viewport)) continue;
      if (view.skins[i].canRemove) {
        if (auto h = hitIf(skinsGridTrashcan(cell).toRect(), pos, ChromeHit::Kind::settingsSkinRemove,
                           i);
            h.kind != ChromeHit::Kind::none) {
          return h;
        }
      }
      if (auto h = hitIf(cell.toRect(), pos, ChromeHit::Kind::settingsSkinRow, i);
          h.kind != ChromeHit::Kind::none) {
        return h;
      }
    }
  }
  return {};
}

ChromeHit hitAbout(QSize logical, QPoint pos) {
  const QRectF body = panelBody(logical);
  const QRectF inner = body.adjusted(16, 14, -16, -14);
  const QRectF plate(inner.left(), inner.bottom() - 48, inner.width(), 48);
  const QRect web =
      aboutWebPill(plate, textWidth(monoFont(10), QStringLiteral("tramp.music"))).toRect();
  if (auto h = hitIf(web, pos, ChromeHit::Kind::aboutWeb); h.kind != ChromeHit::Kind::none) return h;
  return {};
}

}  // namespace

SeekStamps mainSeekStamps(const SessionView& view) {
  return {formatClock(view.positionMs), formatClock(view.durationMs)};
}

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
    case WindowId::skins:
      return hitSkins(logical, pos, view);
  }
  return {};
}

QRect mainOptionsHit(QSize logical) {
  return toHitRect(layoutMainDisplay(panelBody(logical)).options);
}

QRect mainEqHit(QSize logical) { return toHitRect(layoutMainVolumeRow(panelBody(logical)).eq); }

QRect mainPlHit(QSize logical) { return toHitRect(layoutMainVolumeRow(panelBody(logical)).pl); }

}  // namespace tramp
