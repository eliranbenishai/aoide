/// What the listener's **playlist collection** adds up to: the three About
/// figures the collection can answer on its own.
///
/// Deliberately deduplicated. A track kept in three playlists is one track the
/// listener has, so counting it three times would inflate the reading back
/// into the fiction the well used to show. Spins are not here — they come from
/// playback, and the two meet in `AboutStats`.
class CollectionFigures {
  const CollectionFigures({
    required this.playlists,
    required this.tracks,
    required this.totalDuration,
  });

  /// An empty collection, and what a caller reads before anything is kept.
  static const empty = CollectionFigures(
    playlists: 0,
    tracks: 0,
    totalDuration: Duration.zero,
  );

  /// Entries in the collection, **disabled playlists included**: a drive that
  /// is unplugged today must not appear to rewrite the listener's history.
  final int playlists;

  /// Distinct tracks across those entries, compared by normalized path.
  final int tracks;

  /// Summed running time of those distinct tracks. A track whose running time
  /// is not known contributes nothing rather than a guess.
  final Duration totalDuration;

  @override
  bool operator ==(Object other) =>
      other is CollectionFigures &&
      other.playlists == playlists &&
      other.tracks == tracks &&
      other.totalDuration == totalDuration;

  @override
  int get hashCode => Object.hash(playlists, tracks, totalDuration);

  @override
  String toString() =>
      'CollectionFigures(playlists: $playlists, tracks: $tracks, '
      'totalDuration: $totalDuration)';
}
