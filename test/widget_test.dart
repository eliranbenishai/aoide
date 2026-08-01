import 'package:flutter_test/flutter_test.dart';

import 'package:tramp/app.dart';

void main() {
  testWidgets('TrampApp shows chrome shell with brand', (WidgetTester tester) async {
    await tester.pumpWidget(const TrampApp());
    expect(find.textContaining('TRAMP'), findsOneWidget);
  });
}
