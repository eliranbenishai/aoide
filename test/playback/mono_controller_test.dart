import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/mono_controller.dart';

void main() {
  test('setForceMono(true) sets audio-channels=mono', () async {
    final calls = <MapEntry<String, String>>[];
    final mono = MonoController(
      setProperty: (name, value) async {
        calls.add(MapEntry(name, value));
      },
    );

    await mono.setForceMono(true);

    expect(calls, hasLength(1));
    expect(calls.single.key, 'audio-channels');
    expect(calls.single.value, 'mono');
  });

  test('setForceMono(false) restores audio-channels=auto', () async {
    final calls = <MapEntry<String, String>>[];
    final mono = MonoController(
      setProperty: (name, value) async {
        calls.add(MapEntry(name, value));
      },
    );

    await mono.setForceMono(false);

    expect(calls.single.value, 'auto');
  });
}
