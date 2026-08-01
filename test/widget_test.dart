import 'package:flutter_test/flutter_test.dart';

import 'package:tramp/app.dart';
import 'package:tramp/playback/fake_player_engine.dart';

void main() {
  testWidgets('TrampApp shows chrome shell with brand', (WidgetTester tester) async {
    await tester.pumpWidget(TrampApp(engine: FakePlayerEngine()));
    expect(find.textContaining('TRAMP'), findsOneWidget);
  });
}
