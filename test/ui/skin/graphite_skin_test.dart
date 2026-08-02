import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/skin/graphite_skin.dart';

void main() {
  test('main display well sits inside 812x242', () {
    final r = GraphiteSkin.mainDisplayWell;
    expect(r.left >= 0, isTrue);
    expect(r.top >= 0, isTrue);
    expect(r.right <= 812, isTrue);
    expect(r.bottom <= 242, isTrue);
  });

  test('main face asset path is under graphite skin', () {
    expect(GraphiteSkin.mainFace, startsWith('assets/skin/graphite/'));
  });
}
