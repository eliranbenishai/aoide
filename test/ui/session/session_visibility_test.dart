import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/session/session_visibility.dart';

void main() {
  test('session windows stay hidden until the session is ready', () {
    expect(
      sessionWindowShouldShow(sessionReady: false, layoutVisible: true),
      isFalse,
    );
    expect(
      sessionWindowShouldShow(sessionReady: true, layoutVisible: true),
      isTrue,
    );
  });

  test('session windows stay hidden when layout says hidden', () {
    expect(
      sessionWindowShouldShow(sessionReady: true, layoutVisible: false),
      isFalse,
    );
  });

  test('session windows stay hidden while minimize is suppressing show', () {
    expect(
      sessionWindowShouldShow(
        sessionReady: true,
        layoutVisible: true,
        minimizeSuppressed: true,
      ),
      isFalse,
    );
  });
}
