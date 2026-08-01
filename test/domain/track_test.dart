import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';

void main() {
  test('displayTitle prefers tag title over filename', () {
    const tagged = Track(path: r'C:\music\a.mp3', title: 'Night Bus');
    expect(tagged.displayTitle, 'Night Bus');

    const plain = Track(path: r'C:\music\copper-vein.flac');
    expect(plain.displayTitle, 'copper-vein.flac');
  });
}
