import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/about_stats.dart';

void main() {
  group('counts', () {
    test('group thousands', () {
      const stats = AboutStats(
        playlists: 12,
        tracks: 1284,
        totalDuration: Duration.zero,
        spins: 1000000,
      );

      expect(stats.playlistsLabel, '12');
      expect(stats.tracksLabel, '1,284');
      expect(stats.spinsLabel, '1,000,000');
    });

    test('leave three digits ungrouped', () {
      const stats = AboutStats(
        playlists: 0,
        tracks: 999,
        totalDuration: Duration.zero,
        spins: 7,
      );

      expect(stats.tracksLabel, '999');
      expect(stats.playlistsLabel, '0');
      expect(stats.spinsLabel, '7');
    });
  });

  group('total time', () {
    AboutStats withDuration(Duration d) => AboutStats(
          playlists: 1,
          tracks: 1,
          totalDuration: d,
          spins: 1,
        );

    test('reads as days and hours past a day', () {
      expect(
        withDuration(const Duration(days: 3, hours: 22, minutes: 40))
            .totalTimeLabel,
        '3 d 22 h',
      );
    });

    test('reads as hours and minutes under a day', () {
      expect(
        withDuration(const Duration(hours: 22, minutes: 14, seconds: 50))
            .totalTimeLabel,
        '22 h 14 m',
      );
    });

    test('reads as minutes under an hour', () {
      expect(withDuration(const Duration(minutes: 14)).totalTimeLabel, '14 m');
      expect(withDuration(Duration.zero).totalTimeLabel, '0 m');
    });
  });

  test('placeholder is flagged as not yet measured', () {
    expect(AboutStats.placeholder.measured, isFalse);
    expect(
      const AboutStats(
        playlists: 1,
        tracks: 1,
        totalDuration: Duration.zero,
        spins: 1,
      ).measured,
      isTrue,
    );
  });
}
