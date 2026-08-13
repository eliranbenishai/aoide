/// A track the engine was handed and could not play.
///
/// Distinct from "nothing is playing": the transport was told to play this
/// path, accepted it, and then libmpv could not make sound come out of it —
/// a file that has moved, a share that dropped, a codec that is not there.
/// Without this, a failed open reads exactly like a healthy one, which is how
/// a silent transport ends up showing itself as playing.
class PlaybackFailure {
  const PlaybackFailure({required this.path, required this.message});

  /// The track path the transport had open when the engine gave up.
  final String path;

  /// What the engine said, kept verbatim for the listener and the logs.
  final String message;

  @override
  bool operator ==(Object other) =>
      other is PlaybackFailure && other.path == path && other.message == message;

  @override
  int get hashCode => Object.hash(path, message);

  @override
  String toString() => 'PlaybackFailure($path, $message)';
}
