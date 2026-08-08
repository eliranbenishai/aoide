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
}
