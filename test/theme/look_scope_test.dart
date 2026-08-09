import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/theme/look_scope.dart';

void main() {
  testWidgets('LookScope supplies palette', (tester) async {
    final look = BuiltinLook.resolved;
    await tester.pumpWidget(
      LookScope(
        look: look,
        child: Builder(
          builder: (context) {
            final c = LookScope.of(context).palette.phosphorDefault;
            return ColoredBox(color: c, key: const Key('swatch'));
          },
        ),
      ),
    );
    final box = tester.widget<ColoredBox>(find.byKey(const Key('swatch')));
    expect(box.color, look.palette.phosphorDefault);
  });
}
