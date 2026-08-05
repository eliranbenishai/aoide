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

  test('play after stop is a no-op until open', () async {
    final engine = FakePlayerEngine();
    await engine.open(const Track(path: '/a.mp3', title: 'A'));
    await engine.play();
    expect(engine.isPlaying, isTrue);
    await engine.stop();
    expect(engine.hasMedia, isFalse);

    await engine.play();
    expect(engine.isPlaying, isFalse);

    await engine.open(const Track(path: '/a.mp3', title: 'A'));
    await engine.play();
    expect(engine.isPlaying, isTrue);
    await engine.dispose();
  });
}
