/// Usage counters shown on the about window's stats readout.
///
/// The counters a listener recognises as *theirs* — how many playlists they
/// keep, how much music that adds up to, how often they have played it.
///
/// Nothing measures these yet: the playlist manager overhaul owns the real
/// counters, so the about window falls back to [placeholder] until then.
/// [measured] separates the two so a caller can tell a real reading from the
/// stand-in without comparing field by field.
class AboutStats {
  const AboutStats({
    required this.playlists,
    required this.tracks,
    required this.totalDuration,
    required this.spins,
  }) : measured = true;

  const AboutStats._placeholder({
    required this.playlists,
    required this.tracks,
    required this.totalDuration,
    required this.spins,
  }) : measured = false;

  /// Plausible stand-in figures — see the class doc.
  static const placeholder = AboutStats._placeholder(
    playlists: 12,
    tracks: 1284,
    totalDuration: Duration(days: 3, hours: 22, minutes: 40),
    spins: 4096,
  );

  final int playlists;
  final int tracks;

  /// Summed running time of every track across the listener's playlists.
  final Duration totalDuration;

  /// Times a track has been played to the end.
  final int spins;

  /// False for [placeholder]; true for counters that came from real data.
  final bool measured;

  String get playlistsLabel => _grouped(playlists);
  String get tracksLabel => _grouped(tracks);
  String get spinsLabel => _grouped(spins);

  /// Coarse running time — two units at most, so it reads at a glance.
  String get totalTimeLabel {
    final days = totalDuration.inDays;
    if (days > 0) return '$days d ${totalDuration.inHours - days * 24} h';
    final hours = totalDuration.inHours;
    if (hours > 0) return '$hours h ${totalDuration.inMinutes - hours * 60} m';
    return '${totalDuration.inMinutes} m';
  }

  static String _grouped(int value) {
    final digits = value.abs().toString();
    final out = StringBuffer(value < 0 ? '-' : '');
    for (var i = 0; i < digits.length; i++) {
      if (i > 0 && (digits.length - i) % 3 == 0) out.write(',');
      out.write(digits[i]);
    }
    return out.toString();
  }
}
