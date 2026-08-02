import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/fake_player_engine.dart';

void main() {
  test('open emits unknown as the first format frame', () async {
    final engine = FakePlayerEngine();
    final pending = expectLater(
      engine.formatStream,
      emitsInOrder([AudioFormatInfo.unknown]),
    );

    await engine.open(const Track(path: '/a.mp3', title: 'A'));
    await pending;
    await engine.dispose();
  });
}
