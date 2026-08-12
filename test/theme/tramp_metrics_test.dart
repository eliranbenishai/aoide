import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_metrics.dart';

void main() {
  test('mockup canvases are classic×3', () {
    expect(TrampMetrics.mainPlayer, const Size(825, 348));
    expect(TrampMetrics.equalizer, const Size(825, 348));
    expect(TrampMetrics.playlistDefault, const Size(825, 696));
    expect(TrampMetrics.titleBar, 42.0);
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
      const Size(619, 522),
    );
  });
}
