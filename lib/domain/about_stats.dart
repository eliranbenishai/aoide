/// Usage counters shown on the about window's stats readout.
///
/// The counters a listener recognises as *theirs* — how many playlists they
/// keep, how much music that adds up to, how often they have played it.
///
/// Every figure here is measured: the playlist collection answers the first
/// three and playback's **spin** count answers the last, and they reach the
/// about window over the session bus. [unmeasured] is what the window reads
/// before that reading arrives, and [measured] separates the two so a caller
/// can tell them apart without comparing field by field.
class AboutStats {
  const AboutStats({
    required this.playlists,
    required this.tracks,
    required this.totalDuration,
    required this.spins,
  }) : measured = true;

  const AboutStats._unmeasured({
    required this.playlists,
    required this.tracks,
    required this.totalDuration,
    required this.spins,
  }) : measured = false;

  /// No reading has arrived yet. Zeros rather than plausible figures, because
  /// the well must never show a number nothing counted.
  static const unmeasured = AboutStats._unmeasured(
    playlists: 0,
    tracks: 0,
    totalDuration: Duration.zero,
    spins: 0,
  );

  /// **Test fixture only.** The stand-in figures the well used to ship with,
  /// kept so widget and golden tests can render a full-looking readout. No
  /// production code may reference this — see [unmeasured].
  static const placeholder = AboutStats._unmeasured(
    playlists: 12,
    tracks: 1284,
    totalDuration: Duration(days: 3, hours: 22, minutes: 40),
    spins: 4096,
  );

  final int playlists;

  /// Distinct tracks across the collection — a track kept in three playlists
  /// counts once.
  final int tracks;

  /// Summed running time of those distinct tracks.
  final Duration totalDuration;

  /// Times a track has been played to the end.
  final int spins;

  /// False until a reading arrives; true for counters that came from real data.
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
