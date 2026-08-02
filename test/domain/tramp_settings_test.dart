import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/tramp_settings.dart';

void main() {
  test('playlist window size round-trips in JSON', () {
    const s = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
      playlistWindowWidth: 900,
      playlistWindowHeight: 700,
    );
    final again = TrampSettings.fromJson(s.toJson());
    expect(again.playlistWindowWidth, 900);
    expect(again.playlistWindowHeight, 700);
  });

  test('defaults omit playlist window size from JSON', () {
    final json = TrampSettings.defaults.toJson();
    expect(json.containsKey('playlistWindowWidth'), isFalse);
    expect(json.containsKey('playlistWindowHeight'), isFalse);
  });

  test('fromJson ignores invalid or non-positive playlist sizes', () {
    final settings = TrampSettings.fromJson({
      'zoomPercent': 100,
      'lowerRegion': 'playlist',
      'playlistWindowWidth': -1,
      'playlistWindowHeight': 0,
    });
    expect(settings.playlistWindowWidth, isNull);
    expect(settings.playlistWindowHeight, isNull);

    final fromStrings = TrampSettings.fromJson({
      'zoomPercent': 100,
      'lowerRegion': 'playlist',
      'playlistWindowWidth': '900',
      'playlistWindowHeight': 'garbage',
    });
    expect(fromStrings.playlistWindowWidth, isNull);
    expect(fromStrings.playlistWindowHeight, isNull);
  });

  test('fromJson accepts numeric playlist sizes from int or double', () {
    final settings = TrampSettings.fromJson({
      'zoomPercent': 100,
      'lowerRegion': 'playlist',
      'playlistWindowWidth': 900,
      'playlistWindowHeight': 700.5,
    });
    expect(settings.playlistWindowWidth, 900);
    expect(settings.playlistWindowHeight, 700.5);
  });

  test('copyWith updates playlist window size fields', () {
    const base = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
    );
    final updated = base.copyWith(
      playlistWindowWidth: 800,
      playlistWindowHeight: 600,
    );
    expect(updated.playlistWindowWidth, 800);
    expect(updated.playlistWindowHeight, 600);
    expect(updated.zoomPercent, 100);
  });

  test('equality and hashCode include playlist window size', () {
    const a = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
      playlistWindowWidth: 900,
      playlistWindowHeight: 700,
    );
    const b = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
      playlistWindowWidth: 900,
      playlistWindowHeight: 700,
    );
    const c = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
      playlistWindowWidth: 901,
      playlistWindowHeight: 700,
    );
    expect(a, b);
    expect(a.hashCode, b.hashCode);
    expect(a, isNot(c));
  });
}
