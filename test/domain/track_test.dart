import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';

void main() {
  // Forward slashes are separators under every p.Style, so these fixtures read
  // the same on POSIX and Windows hosts.
  test('displayTitle prefers tag title over filename', () {
    const tagged = Track(path: '/music/a.mp3', title: 'Night Bus');
    expect(tagged.displayTitle, 'Night Bus');

    const plain = Track(path: '/music/copper-vein.flac');
    expect(plain.displayTitle, 'copper-vein.flac');
  });

  test('displayTitle falls back to the filename when the tag is blank', () {
    const blank = Track(path: '/music/copper-vein.flac', title: '   ');
    expect(blank.displayTitle, 'copper-vein.flac');
  });
}
