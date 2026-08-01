import 'package:flutter_test/flutter_test.dart';

import 'package:tramp/app.dart';

void main() {
  testWidgets('TrampApp shows scaffold text', (WidgetTester tester) async {
    await tester.pumpWidget(const TrampApp());
    expect(find.text('Tramp scaffold'), findsOneWidget);
  });
}
