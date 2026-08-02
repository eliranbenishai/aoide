import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

void main() {
  testWidgets('of() exposes the factor to descendants', (tester) async {
    double? seen;
    await tester.pumpWidget(
      ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: Builder(
          builder: (context) {
            seen = ZoomScope.of(context).factor;
            return const SizedBox();
          },
        ),
      ),
    );
    expect(seen, 1.5);
  });

  test('snap rounds a hairline to a whole device pixel', () {
    // At 150% on a 1x display a 1px bevel would land on 1.5 device px and blur.
    const scope = ZoomScope(
      factor: 1.5,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    // 1 logical * 1.5 = 1.5 device px -> rounds to 2 -> back to 1.333 logical.
    expect(scope.snap(1), closeTo(2 / 1.5, 1e-9));
  });

  test('snap never returns less than one device pixel', () {
    const scope = ZoomScope(
      factor: 1,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    expect(scope.snap(0.2), 1.0);
  });

  test('snap is a no-op when the result is already whole', () {
    const scope = ZoomScope(
      factor: 2,
      devicePixelRatio: 1,
      child: SizedBox(),
    );
    expect(scope.snap(1), 1.0);
  });

  test('device pixel ratio participates in the rounding', () {
    const scope = ZoomScope(
      factor: 1.25,
      devicePixelRatio: 2,
      child: SizedBox(),
    );
    // 1 * 1.25 * 2 = 2.5 device px. Dart rounds half away from zero, so that
    // is 3 device px, or 3 / 2.5 logical.
    expect(scope.snap(1), closeTo(3 / 2.5, 1e-9));
  });

  testWidgets('hairlineFor falls back to an unsnapped hairline with no scope',
      (tester) async {
    double? seen;
    await tester.pumpWidget(
      Builder(
        builder: (context) {
          seen = ZoomScope.hairlineFor(context);
          return const SizedBox();
        },
      ),
    );
    expect(seen, ZoomScope.hairline);
  });

  testWidgets('hairlineFor snaps when a scope is present', (tester) async {
    double? seen;
    await tester.pumpWidget(
      ZoomScope(
        factor: 1.5,
        devicePixelRatio: 1,
        child: Builder(
          builder: (context) {
            seen = ZoomScope.hairlineFor(context);
            return const SizedBox();
          },
        ),
      ),
    );
    expect(seen, closeTo(2 / 1.5, 1e-9));
  });

  // Tested directly rather than by counting rebuilds: a widget test that
  // re-pumps a fresh child rebuilds it on widget identity alone, so it cannot
  // isolate the inherited dependency.
  test('updateShouldNotify fires only when the factor or ratio changes', () {
    const base = ZoomScope(factor: 1, devicePixelRatio: 1, child: SizedBox());
    const sameValues =
        ZoomScope(factor: 1, devicePixelRatio: 1, child: SizedBox());
    const otherFactor =
        ZoomScope(factor: 2, devicePixelRatio: 1, child: SizedBox());
    const otherRatio =
        ZoomScope(factor: 1, devicePixelRatio: 2, child: SizedBox());

    expect(base.updateShouldNotify(sameValues), isFalse);
    expect(otherFactor.updateShouldNotify(base), isTrue);
    expect(otherRatio.updateShouldNotify(base), isTrue);
  });
}
