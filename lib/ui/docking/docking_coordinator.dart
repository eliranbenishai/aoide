import 'package:flutter/foundation.dart';
import 'package:flutter/painting.dart';

import '../../domain/tramp_settings.dart';
import '../../theme/tramp_metrics.dart';
import 'dock_layout.dart';

/// Winamp-style snap / sticky-group docking over logical window frames.
///
/// Pure layout math — no OS window APIs. [frameFor] returns pixel rects at
/// the given zoom for `window_manager`.
class DockingCoordinator extends ChangeNotifier {
  DockingCoordinator(DockLayout initial) : _layout = initial;

  static const double snapThreshold = 12.0;
  static const double undockSeparation = 48.0;

  DockLayout _layout;
  DockLayout get layout => _layout;

  void move(WindowId id, Offset topLeft, {required bool shiftUndock}) {
    final current = _topLeft(id);
    final delta = topLeft - current;

    var edges = List<DockEdge>.from(_layout.dockEdges);
    var shouldUndock = shiftUndock;
    if (!shouldUndock && _hasEdge(id, edges)) {
      final proposed = _rectFor(id).shift(delta);
      shouldUndock = _separatesBeyondBreak(id, proposed, edges);
    }
    if (shouldUndock) {
      edges = edges
          .where((e) => e.a != id && e.b != id)
          .toList(growable: false);
    }

    _layout = _layout.copyWith(dockEdges: edges);
    final group = groupOf(id);
    for (final member in group) {
      if (member == id) {
        _layout = _layout.withFrame(
          member,
          _layout.frameOf(member).copyWith(left: topLeft.dx, top: topLeft.dy),
        );
      } else {
        final frame = _layout.frameOf(member);
        _layout = _layout.withFrame(
          member,
          frame.copyWith(
            left: frame.left + delta.dx,
            top: frame.top + delta.dy,
          ),
        );
      }
    }

    _trySnap(id);
    notifyListeners();
  }

  void resizePlaylist(Size logical) {
    final playlist = _layout.playlist;
    _layout = _layout.copyWith(
      playlist: playlist.copyWith(
        width: logical.width,
        height: logical.height,
      ),
    );
    notifyListeners();
  }

  void setShaded(WindowId id, bool shaded) {
    _layout = _layout.withFrame(
      id,
      _layout.frameOf(id).copyWith(shaded: shaded),
    );
    notifyListeners();
  }

  void setVisible(WindowId id, bool visible) {
    _layout = _layout.withFrame(
      id,
      _layout.frameOf(id).copyWith(visible: visible),
    );
    notifyListeners();
  }

  Set<WindowId> groupOf(WindowId id) {
    final edges = _layout.dockEdges;
    final group = <WindowId>{id};
    var grew = true;
    while (grew) {
      grew = false;
      for (final edge in edges) {
        final aIn = group.contains(edge.a);
        final bIn = group.contains(edge.b);
        if (aIn && !bIn) {
          group.add(edge.b);
          grew = true;
        } else if (bIn && !aIn) {
          group.add(edge.a);
          grew = true;
        }
      }
    }
    return group;
  }

  /// Pixel frames at [zoom] for window_manager.
  Rect frameFor(WindowId id, double zoom) {
    final logical = _rectFor(id);
    return Rect.fromLTWH(
      logical.left * zoom,
      logical.top * zoom,
      logical.width * zoom,
      logical.height * zoom,
    );
  }

  Offset _topLeft(WindowId id) {
    final frame = _layout.frameOf(id);
    return Offset(frame.left, frame.top);
  }

  Size _logicalSize(WindowId id) {
    final frame = _layout.frameOf(id);
    final base = switch (id) {
      WindowId.main => TrampMetrics.mainPlayer,
      WindowId.equalizer => TrampMetrics.equalizer,
      WindowId.playlist => Size(
          frame.width ?? TrampMetrics.playlistDefault.width,
          frame.height ?? TrampMetrics.playlistDefault.height,
        ),
    };
    if (frame.shaded) {
      return Size(base.width, TrampMetrics.titleBar);
    }
    return base;
  }

  Rect _rectFor(WindowId id) {
    final frame = _layout.frameOf(id);
    final size = _logicalSize(id);
    return Rect.fromLTWH(frame.left, frame.top, size.width, size.height);
  }

  bool _hasEdge(WindowId id, List<DockEdge> edges) =>
      edges.any((e) => e.a == id || e.b == id);

  /// True when the proposed solo rect for [id] is more than [undockSeparation]
  /// away from any current dock partner along the docked edge.
  bool _separatesBeyondBreak(
    WindowId id,
    Rect proposed,
    List<DockEdge> edges,
  ) {
    for (final edge in edges) {
      if (edge.a != id && edge.b != id) continue;
      final partnerId = edge.a == id ? edge.b : edge.a;
      final partner = _rectFor(partnerId);
      final side = edge.a == id ? edge.side : _opposite(edge.side);
      if (_edgeGap(proposed, partner, side) > undockSeparation) {
        return true;
      }
    }
    return false;
  }

  double _edgeGap(Rect moving, Rect partner, DockSide movingSide) {
    switch (movingSide) {
      case DockSide.bottom:
        return (moving.bottom - partner.top).abs();
      case DockSide.top:
        return (moving.top - partner.bottom).abs();
      case DockSide.right:
        return (moving.right - partner.left).abs();
      case DockSide.left:
        return (moving.left - partner.right).abs();
    }
  }

  DockSide _opposite(DockSide side) {
    switch (side) {
      case DockSide.left:
        return DockSide.right;
      case DockSide.right:
        return DockSide.left;
      case DockSide.top:
        return DockSide.bottom;
      case DockSide.bottom:
        return DockSide.top;
    }
  }

  void _trySnap(WindowId id) {
    final group = groupOf(id);
    final moving = _rectFor(id);
    _SnapCandidate? best;

    for (final otherId in WindowId.values) {
      if (otherId == id) continue;
      if (group.contains(otherId)) continue;
      if (!_layout.frameOf(otherId).visible) continue;
      final other = _rectFor(otherId);
      for (final candidate in _candidates(id, moving, otherId, other)) {
        if (best == null || candidate.distance < best.distance) {
          best = candidate;
        }
      }
    }

    if (best == null) return;

    final snapDelta = best.delta;
    for (final member in group) {
      final frame = _layout.frameOf(member);
      _layout = _layout.withFrame(
        member,
        frame.copyWith(
          left: frame.left + snapDelta.dx,
          top: frame.top + snapDelta.dy,
        ),
      );
    }

    final edges = List<DockEdge>.from(_layout.dockEdges);
    final already = edges.any(
      (e) =>
          (e.a == best!.movingId && e.b == best.targetId) ||
          (e.a == best.targetId && e.b == best.movingId),
    );
    if (!already) {
      edges.add(
        DockEdge(a: best.movingId, b: best.targetId, side: best.side),
      );
      _layout = _layout.copyWith(dockEdges: List.unmodifiable(edges));
    }
  }

  List<_SnapCandidate> _candidates(
    WindowId movingId,
    Rect moving,
    WindowId targetId,
    Rect target,
  ) {
    final out = <_SnapCandidate>[];

    void consider({
      required DockSide side,
      required double distance,
      required Offset delta,
      required bool aligned,
    }) {
      if (distance > snapThreshold || !aligned) return;
      out.add(
        _SnapCandidate(
          movingId: movingId,
          targetId: targetId,
          side: side,
          distance: distance,
          delta: delta,
        ),
      );
    }

    // moving.bottom → target.top
    consider(
      side: DockSide.bottom,
      distance: (moving.bottom - target.top).abs(),
      delta: Offset(0, target.top - moving.bottom),
      aligned: _overlapsOrNear1D(
        moving.left,
        moving.right,
        target.left,
        target.right,
      ),
    );
    // moving.top → target.bottom
    consider(
      side: DockSide.top,
      distance: (moving.top - target.bottom).abs(),
      delta: Offset(0, target.bottom - moving.top),
      aligned: _overlapsOrNear1D(
        moving.left,
        moving.right,
        target.left,
        target.right,
      ),
    );
    // moving.right → target.left
    consider(
      side: DockSide.right,
      distance: (moving.right - target.left).abs(),
      delta: Offset(target.left - moving.right, 0),
      aligned: _overlapsOrNear1D(
        moving.top,
        moving.bottom,
        target.top,
        target.bottom,
      ),
    );
    // moving.left → target.right
    consider(
      side: DockSide.left,
      distance: (moving.left - target.right).abs(),
      delta: Offset(target.right - moving.left, 0),
      aligned: _overlapsOrNear1D(
        moving.top,
        moving.bottom,
        target.top,
        target.bottom,
      ),
    );

    return out;
  }

  bool _overlapsOrNear1D(
    double a0,
    double a1,
    double b0,
    double b1,
  ) {
    final overlap = a0 < b1 && b0 < a1;
    if (overlap) return true;
    return (a0 - b0).abs() <= snapThreshold ||
        (a1 - b1).abs() <= snapThreshold ||
        (a0 - b1).abs() <= snapThreshold ||
        (a1 - b0).abs() <= snapThreshold;
  }
}

class _SnapCandidate {
  const _SnapCandidate({
    required this.movingId,
    required this.targetId,
    required this.side,
    required this.distance,
    required this.delta,
  });

  final WindowId movingId;
  final WindowId targetId;
  final DockSide side;
  final double distance;
  final Offset delta;
}
