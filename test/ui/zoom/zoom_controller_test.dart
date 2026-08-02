import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';

void main() {
  // A monitor big enough that every step fits, so tests that aren't about
  // clamping don't accidentally hit it.
  const hugeWorkArea = Size(6000, 4000);

  ZoomController build({
    Size workArea = hugeWorkArea,
    int initialPercent = 100,
  }) =>
      ZoomController(workArea: workArea, initialPercent: initialPercent);

  test('steps are the six documented levels', () {
    expect(ZoomController.steps, [100, 125, 150, 200, 250, 300]);
  });

  test('factor is percent over one hundred', () {
    final c = build(initialPercent: 150);
    expect(c.factor, 1.5);
  });

  test('stepUp and stepDown move one step and clamp at the ends', () {
    final c = build(initialPercent: 100);
    c.stepDown();
    expect(c.percent, 100, reason: 'already at the smallest step');
    c.stepUp();
    expect(c.percent, 125);
    c.setPercent(300);
    c.stepUp();
    expect(c.percent, 300, reason: 'already at the largest step');
  });

  test('reset returns to 100 percent', () {
    final c = build(initialPercent: 250);
    c.reset();
    expect(c.percent, 100);
  });

  test('notifies listeners when the step changes but not when it repeats', () {
    final c = build(initialPercent: 100);
    var calls = 0;
    c.addListener(() => calls++);
    c.setPercent(150);
    expect(calls, 1);
    c.setPercent(150);
    expect(calls, 1, reason: 'setting the same step must not notify');
  });

  test('window size scales the canvas stack by the factor', () {
    final c = build();
    // 812 panel + 6 frame either side = 824 logical; 6 + 242 + 6 + 240 + 6 = 500.
    expect(c.windowSizeFor(100), const Size(824, 500));
    expect(c.windowSizeFor(200), const Size(1648, 1000));
  });

  test('minimum window size never clips the player chrome', () {
    final c = build();
    final min100 = c.minimumWindowSizeFor(100);
    expect(min100.width, 824);
    // Player + gutter + a collapsed equalizer title bar, plus the frame.
    expect(min100.height, lessThan(c.windowSizeFor(100).height));
  });

  test('steps too wide for the work area are disabled', () {
    // 300% needs 2472 logical px of width; a 1600px-wide monitor cannot host it.
    final c = build(workArea: const Size(1600, 1200));
    expect(c.canUse(100), isTrue);
    expect(c.canUse(150), isTrue);
    expect(c.canUse(300), isFalse);
    expect(c.enabledSteps, [100, 125, 150]);
  });

  test('setPercent refuses a step that does not fit', () {
    final c = build(workArea: const Size(1600, 1200), initialPercent: 100);
    c.setPercent(300);
    expect(c.percent, 100, reason: 'must not adopt a step that would clip');
  });

  test('changing the work area re-clamps the current step', () {
    final c = build(initialPercent: 300);
    c.workArea = const Size(1600, 1200);
    expect(c.percent, 150, reason: 'largest step that still fits');
  });

  test('bestInitialPercent picks the largest fitting step capped at 150', () {
    expect(ZoomController.bestInitialPercent(const Size(6000, 4000)), 150);
    expect(ZoomController.bestInitialPercent(const Size(1000, 700)), 100);
  });

  test('metrics match the locked canvases', () {
    expect(TrampMetrics.mainPlayer, const Size(812, 242));
    expect(TrampMetrics.equalizer, const Size(812, 206));
    expect(TrampMetrics.gutter, 6.0);
  });
}
