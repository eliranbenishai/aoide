import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/open_url.dart';

void main() {
  test('openExternalUrl rejects non-http schemes', () {
    expect(
      () => openExternalUrl(Uri.parse('file:///tmp/x')),
      throwsArgumentError,
    );
  });
}
