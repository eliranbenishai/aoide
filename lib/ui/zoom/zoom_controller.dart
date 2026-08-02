import 'package:flutter/foundation.dart';
import 'package:flutter/painting.dart';

import '../../theme/tramp_metrics.dart';

/// Owns the discrete zoom step.
///
/// The UI is authored at a fixed logical size and scaled by [factor] with a
/// single transform, so this class only has to decide which step is current and
/// which steps the display can host.
class ZoomController extends ChangeNotifier {
  ZoomController({
    required Size workArea,
    int initialPercent = 100,
    this.onPercentChanged,
  })  : _workArea = workArea,
        _percent = 100 {
    _percent = _largestFitting(initialPercent);
  }

  static const List<int> steps = [100, 125, 150, 200, 250, 300];

  /// Never auto-select a step larger than this on first run — a fresh install
  /// on a 4K display should not open enormous.
  static const int _initialCap = 150;

  /// Called after a successful step change so the host can resize the window
  /// and persist the choice.
  final void Function(int percent)? onPercentChanged;

  int _percent;
  Size _workArea;

  int get percent => _percent;
  double get factor => _percent / 100;

  Size get workArea => _workArea;

  set workArea(Size value) {
    if (_workArea == value) return;
    _workArea = value;
    final clamped = _largestFitting(_percent);
    final percentChanged = clamped != _percent;
    if (percentChanged) {
      _percent = clamped;
      onPercentChanged?.call(_percent);
    }
    notifyListeners();
  }

  List<int> get enabledSteps => steps.where(canUse).toList();

  bool canUse(int percent) {
    final needed = minimumWindowSizeFor(percent);
    return needed.width <= _workArea.width &&
        needed.height <= _workArea.height;
  }

  void setPercent(int value) {
    if (!steps.contains(value)) return;
    if (!canUse(value)) return;
    if (value == _percent) return;
    _percent = value;
    notifyListeners();
    onPercentChanged?.call(_percent);
  }

  void stepUp() {
    final index = steps.indexOf(_percent);
    for (var i = index + 1; i < steps.length; i++) {
      if (canUse(steps[i])) {
        setPercent(steps[i]);
        return;
      }
    }
  }

  void stepDown() {
    final index = steps.indexOf(_percent);
    for (var i = index - 1; i >= 0; i--) {
      if (canUse(steps[i])) {
        setPercent(steps[i]);
        return;
      }
    }
  }

  void reset() => setPercent(100);

  /// Comfortable default window: player, gutter, and a usable lower region.
  Size windowSizeFor(int percent) {
    final f = percent / 100;
    final width = TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2;
    final height = TrampMetrics.frame * 2 +
        TrampMetrics.mainPlayer.height +
        TrampMetrics.gutter +
        TrampMetrics.minLowerRegion;
    return Size(width * f, height * f);
  }

  /// Smallest window that still shows the whole player and a collapsed
  /// equalizer title bar without clipping.
  Size minimumWindowSizeFor(int percent) {
    final f = percent / 100;
    final width = TrampMetrics.mainPlayer.width + TrampMetrics.frame * 2;
    final height = TrampMetrics.frame * 2 +
        TrampMetrics.mainPlayer.height +
        TrampMetrics.gutter +
        TrampMetrics.titleBar;
    return Size(width * f, height * f);
  }

  static int bestInitialPercent(Size workArea) {
    final probe = ZoomController(workArea: workArea);
    var best = steps.first;
    for (final step in steps) {
      if (step > _initialCap) break;
      if (probe.canUse(step)) best = step;
    }
    return best;
  }

  int _largestFitting(int desired) {
    if (steps.contains(desired) && canUse(desired)) return desired;
    var best = steps.first;
    for (final step in steps) {
      if (step > desired) break;
      if (canUse(step)) best = step;
    }
    return best;
  }
}
