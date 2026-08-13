import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/theme/tramp_metrics.dart';

void main() {
  test('mockup canvases are classic×3', () {
    expect(TrampMetrics.mainPlayer, const Size(825, 348));
    expect(TrampMetrics.equalizer, const Size(825, 348));
    expect(TrampMetrics.titleBar, 42.0);
  });

  test('playlist default leaves the track side a classic×3 width', () {
    // The Playlist Manager sets the collection panel beside the track list, so
    // the window is wider than classic×3 by the panel and its divider. The
    // right panel keeps 825 — the width the mockup footer was authored against.
    expect(
      TrampMetrics.playlistDefault.width -
          TrampSettings.defaultPlaylistCollectionWidth -
          TrampMetrics.playlistDividerWidth,
      825,
    );
    expect(TrampMetrics.playlistDefault, const Size(1073, 696));
  });

  test('collection panel never squeezes the footer below its minimum', () {
    expect(
      TrampMetrics.playlistMinWithCollection.width -
          TrampMetrics.playlistCollectionMinWidth -
          TrampMetrics.playlistDividerWidth,
      TrampMetrics.playlistMin.width,
    );
  });

  test('about canvas fits credits without matching the EQ/main seed', () {
    expect(TrampMetrics.about, const Size(480, 360));
    expect(TrampMetrics.settings, const Size(520, 420));
  });

  test('zoomed pixel size is logical × zoom percent / 100', () {
    expect(TrampMetrics.zoomed(const Size(480, 360), 75), const Size(360, 270));
    expect(TrampMetrics.zoomed(const Size(825, 348), 75), const Size(618.75, 261));
    expect(TrampMetrics.zoomed(const Size(480, 360), 100), const Size(480, 360));
  });

  test('native unmapped seeds are 75% canvases, rounded', () {
    // Native plugin defaults must match these literals (not the EQ 619×261
    // seed) or about/settings keep a black FlView gutter around the chrome.
    expect(
      TrampMetrics.nativeUnmappedSeed(TrampMetrics.about),
      const Size(360, 270),
    );
    expect(
      TrampMetrics.nativeUnmappedSeed(TrampMetrics.settings),
      const Size(390, 315),
    );
    expect(
      TrampMetrics.nativeUnmappedSeed(TrampMetrics.equalizer),
      const Size(619, 261),
    );
    expect(
      TrampMetrics.nativeUnmappedSeed(TrampMetrics.playlistDefault),
      const Size(805, 522),
    );
  });
}
