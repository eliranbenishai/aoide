import 'package:path/path.dart' as p;

/// The one canonical form for every path the playlist collection stores or
/// compares — entry identities and cached track paths alike.
///
/// [p.canonicalize] is absolute, resolves `.` / `..`, normalizes separators for
/// the host style, and case-folds **only** on Windows, whose filesystem is
/// case-insensitive; POSIX spellings keep their case because there two spellings
/// really are two files. Rolling our own case rule would have to guess that,
/// and `LookController` already trusts this function for pack roots.
String normalizePlaylistPath(String path) => p.canonicalize(path);

/// One entry in the **playlist collection**: a reference to a playlist file
/// where the listener put it, plus the figures the collection panel paints.
///
/// The collection stores references, never copies — see
/// `docs/adr/0008-playlist-collection-stores-references.md`. [path] is the
/// entry's identity, so it is always [normalizePlaylistPath]d.
class SavedPlaylist {
  SavedPlaylist({
    required String path,
    this.name,
    this.trackCount = 0,
    this.totalDuration = Duration.zero,
    this.modified,
  }) : path = normalizePlaylistPath(path);

  /// Normalized absolute path of the playlist file — the entry's identity.
  final String path;

  /// Listener's override for [displayName]; null means "read the filename".
  final String? name;

  final int trackCount;
  final Duration totalDuration;

  /// Modification time seen when the cached figures were computed. A moved
  /// time means the listener edited the file elsewhere and the figures are due
  /// a recompute.
  final DateTime? modified;

  /// What the collection panel shows: the override, or the file's own name.
  String get displayName {
    final override = name?.trim();
    if (override != null && override.isNotEmpty) return override;
    return p.basenameWithoutExtension(path);
  }

  SavedPlaylist copyWith({
    String? name,
    bool clearName = false,
    int? trackCount,
    Duration? totalDuration,
    DateTime? modified,
  }) {
    return SavedPlaylist(
      path: path,
      name: clearName ? null : (name ?? this.name),
      trackCount: trackCount ?? this.trackCount,
      totalDuration: totalDuration ?? this.totalDuration,
      modified: modified ?? this.modified,
    );
  }

  Map<String, dynamic> toJson() => {
        'path': path,
        if (name != null) 'name': name,
        'trackCount': trackCount,
        'totalDurationMs': totalDuration.inMilliseconds,
        if (modified != null) 'modifiedMs': modified!.millisecondsSinceEpoch,
      };

  factory SavedPlaylist.fromJson(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('SavedPlaylist.path');
    }
    final name = json['name'];
    final count = json['trackCount'];
    final durationMs = json['totalDurationMs'];
    final modifiedMs = json['modifiedMs'];
    return SavedPlaylist(
      path: path,
      name: name is String && name.trim().isNotEmpty ? name : null,
      trackCount: count is num && count >= 0 ? count.toInt() : 0,
      totalDuration: durationMs is num && durationMs >= 0
          ? Duration(milliseconds: durationMs.toInt())
          : Duration.zero,
      modified: modifiedMs is num
          ? DateTime.fromMillisecondsSinceEpoch(modifiedMs.toInt())
          : null,
    );
  }

  /// Alphabetical by what the listener reads, which is [displayName].
  static int compareByDisplayName(SavedPlaylist a, SavedPlaylist b) {
    final byName =
        a.displayName.toLowerCase().compareTo(b.displayName.toLowerCase());
    return byName != 0 ? byName : a.path.compareTo(b.path);
  }

  @override
  bool operator ==(Object other) =>
      other is SavedPlaylist &&
      other.path == path &&
      other.name == name &&
      other.trackCount == trackCount &&
      other.totalDuration == totalDuration &&
      other.modified == modified;

  @override
  int get hashCode =>
      Object.hash(path, name, trackCount, totalDuration, modified);
}
