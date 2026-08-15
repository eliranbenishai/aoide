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
  DockingCoordinator(
    DockLayout initial, {
    this.snapThreshold = 20.0,
    this.stickyMoveGroups = true,
  }) : _layout = initial;

  /// Logical px within which edges snap. Set from [DockSnapStrength].
  double snapThreshold;

  /// When true (default), main title-bar drag translates its dock-edge
  /// cohort ([groupOf]). When false, every window moves alone.
  final bool stickyMoveGroups;

  static const double undockSeparation = 48.0;

  /// Minimum logical drag distance before EQ/PL peel dock edges.
  ///
  /// Must stay above setPosition / configure-event jitter — Linux snap
  /// finalize applies a frame that echoes a sub-pixel `move` with
  /// `snap: false`, which used to clear edges and sever main-drag carry.
  static const double peelDelta = 8.0;

  DockLayout _layout;
  DockLayout get layout => _layout;

  void move(WindowId id, Offset topLeft, {required bool shiftUndock, bool snap = true}) {
    // Freestanding windows: never snap, never peel, never carry partners.
    if (id.freestanding) {
      _layout = _layout.withFrame(
        id,
        _layout.frameOf(id).copyWith(left: topLeft.dx, top: topLeft.dy),
      );
      notifyListeners();
      return;
    }

    final current = _topLeft(id);
    final delta = topLeft - current;

    var edges = List<DockEdge>.from(_layout.dockEdges);
    var shouldUndock = shiftUndock;

    // EQ / playlist peel off dock edges once the user clearly drags them.
    // Main never peels — it translates its dock-edge cohort instead.
    if (!shouldUndock &&
        id != WindowId.main &&
        delta.distanceSquared > peelDelta * peelDelta &&
        _hasEdge(id, edges)) {
      shouldUndock = true;
    }

    // Separation undock: only EQ / playlist finalize (main carries its
    // docked cohort, so solo-vs-partner gap is not meaningful for main).
    if (!shouldUndock &&
        snap &&
        id != WindowId.main &&
        _hasEdge(id, edges)) {
      final size = _logicalSize(id);
      final solo = Rect.fromLTWH(topLeft.dx, topLeft.dy, size.width, size.height);
      shouldUndock = _separatesBeyondBreak(id, solo, edges);
    }
    if (shouldUndock) {
      edges = edges
          .where((e) => e.a != id && e.b != id)
          .toList(growable: false);
    }

    _layout = _layout.copyWith(dockEdges: edges);
    final cohort = moveCohortOf(id);
    for (final member in cohort) {
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

    // Snap only from EQ / playlist finalize — never from main or settings.
    // Live drag passes snap: false so snap does not fight the cursor.
    if (snap && !shiftUndock && id != WindowId.main) {
      _trySnap(id);
    }
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

  /// Edge-connected dock component containing [id] (visible members only).
  ///
  /// Used for snap partner exclusion, “is docked” checks, and main title-bar
  /// drag cohorts ([moveCohortOf]).
  Set<WindowId> groupOf(WindowId id) {
    final edges = _layout.dockEdges;
    final group = <WindowId>{id};
    // Hidden windows stay out of sticky groups so minimize/hide does not
    // freeze snap between the remaining visible windows.
    if (!_layout.frameOf(id).visible) return group;

    var grew = true;
    while (grew) {
      grew = false;
      for (final edge in edges) {
        final aIn = group.contains(edge.a);
        final bIn = group.contains(edge.b);
        if (aIn && !bIn) {
          if (!_layout.frameOf(edge.b).visible) continue;
          group.add(edge.b);
          grew = true;
        } else if (bIn && !aIn) {
          if (!_layout.frameOf(edge.a).visible) continue;
          group.add(edge.a);
          grew = true;
        }
      }
    }
    return group;
  }

  /// Windows that move together when [id]'s title bar is dragged.
  ///
  /// Freestanding → self only. EQ / playlist → edge [groupOf].
  /// Main → edge [groupOf], then any visible window currently flush to that
  /// cohort within [snapThreshold] (covers Linux when dock edges were never
  /// recorded because `onWindowMoved` is missing).
  /// When [stickyMoveGroups] is false, always just [id].
  Set<WindowId> moveCohortOf(WindowId id) {
    if (!stickyMoveGroups) {
      return {id};
    }
    if (id.freestanding) {
      return {id};
    }
    if (id != WindowId.main) {
      return groupOf(id);
    }
    return _mainMoveCohort();
  }

  /// Edge-connected group plus geometrically flush visible satellites.
  Set<WindowId> _mainMoveCohort() {
    final cohort = Set<WindowId>.from(groupOf(WindowId.main));
    if (snapThreshold <= 0) return cohort;

    var grew = true;
    while (grew) {
      grew = false;
      for (final other in WindowId.values) {
        if (other.freestanding || cohort.contains(other)) continue;
        if (!_layout.frameOf(other).visible) continue;
        final otherRect = _rectFor(other);
        for (final member in cohort) {
          final memberRect = _rectFor(member);
          if (_candidates(other, otherRect, member, memberRect).isNotEmpty ||
              _candidates(member, memberRect, other, otherRect).isNotEmpty) {
            cohort.add(other);
            grew = true;
            break;
          }
        }
      }
    }
    return cohort;
  }

  void setVisible(WindowId id, bool visible) {
    _layout = _layout.withFrame(
      id,
      _layout.frameOf(id).copyWith(visible: visible),
    );
    if (!visible) {
      _layout = _layout.copyWith(
        dockEdges: _layout.dockEdges
            .where((e) => e.a != id && e.b != id)
            .toList(growable: false),
      );
    }
    notifyListeners();
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

  /// Keep screen top-lefts stable across a zoom step, then reseat docked
  /// satellites flush on their dock edges (sizes change with zoom).
  ///
  /// Free windows (no dock edges) only get the pixel-TL rebase. Docked EQ /
  /// playlist windows keep their edges and are re-aligned to partners — usually
  /// the main player — so contact stays tight after the scale change.
  void reanchorForZoom({required double fromZoom, required double toZoom}) {
    if (fromZoom <= 0 || toZoom <= 0) return;
    if ((fromZoom - toZoom).abs() < 1e-9) return;

    final scale = fromZoom / toZoom;
    for (final id in WindowId.values) {
      final frame = _layout.frameOf(id);
      _layout = _layout.withFrame(
        id,
        frame.copyWith(
          left: frame.left * scale,
          top: frame.top * scale,
        ),
      );
    }

    // Multiple passes so PL→EQ→main chains reseat after partners move.
    for (var pass = 0; pass < WindowId.values.length; pass++) {
      for (final id in WindowId.values) {
        if (id == WindowId.main || id.freestanding) continue;
        if (!_hasEdge(id, _layout.dockEdges)) continue;
        _applyDockConstraints(id);
      }
    }
    notifyListeners();
  }

  /// Force [id]'s logical top-left onto every dock edge it participates in.
  ///
  /// Primary snap sides are face-adjacent; orthogonal flush sides share an
  /// edge coordinate (e.g. lefts equal). Pick whichever interpretation is
  /// closer to the current geometry so zoom rebase does not shove a
  /// left-flush playlist onto main's right.
  void _applyDockConstraints(WindowId id) {
    final size = _logicalSize(id);
    var left = _layout.frameOf(id).left;
    var top = _layout.frameOf(id).top;

    for (final edge in _layout.dockEdges) {
      if (edge.a != id && edge.b != id) continue;
      final partnerId = edge.a == id ? edge.b : edge.a;
      final selfSide = edge.a == id ? edge.side : _opposite(edge.side);
      final partner = _rectFor(partnerId);
      switch (selfSide) {
        case DockSide.bottom:
          final adj = partner.top - size.height;
          final flush = partner.bottom - size.height;
          top = (top - adj).abs() <= (top - flush).abs() ? adj : flush;
        case DockSide.top:
          final adj = partner.bottom;
          final flush = partner.top;
          top = (top - adj).abs() <= (top - flush).abs() ? adj : flush;
        case DockSide.right:
          final adj = partner.left - size.width;
          final flush = partner.right - size.width;
          left = (left - adj).abs() <= (left - flush).abs() ? adj : flush;
        case DockSide.left:
          final adj = partner.right;
          final flush = partner.left;
          left = (left - adj).abs() <= (left - flush).abs() ? adj : flush;
      }
    }

    final frame = _layout.frameOf(id);
    if (frame.left == left && frame.top == top) return;
    _layout = _layout.withFrame(
      id,
      frame.copyWith(left: left, top: top),
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
      WindowId.settings => TrampMetrics.settings,
      WindowId.about => TrampMetrics.about,
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
    if (id.freestanding || snapThreshold <= 0) return;
    final group = groupOf(id);
    final moving = _rectFor(id);
    _SnapCandidate? best;

    for (final otherId in WindowId.values) {
      if (otherId == id || otherId.freestanding) continue;
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

    final target = _rectFor(best.targetId);
    final snapped = moving.shift(best.delta);
    var snapDelta = best.delta;
    DockSide? orthoSide;
    final ortho = _orthogonalFlush(best.side, snapped, target);
    if (ortho != null) {
      snapDelta += ortho.delta;
      orthoSide = ortho.side;
    }

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
    void addEdge(DockSide side) {
      final already = edges.any(
        (e) =>
            ((e.a == best!.movingId && e.b == best.targetId) ||
                (e.a == best.targetId && e.b == best.movingId)) &&
            (e.side == side || e.side == _opposite(side)),
      );
      if (!already) {
        edges.add(
          DockEdge(a: best!.movingId, b: best.targetId, side: side),
        );
      }
    }

    addEdge(best.side);
    if (orthoSide != null) addEdge(orthoSide);
    _layout = _layout.copyWith(dockEdges: List.unmodifiable(edges));
  }

  /// When the primary snap is top/bottom (or left/right), flush the orthogonal
  /// axis if that edge is already within [snapThreshold].
  ({DockSide side, Offset delta})? _orthogonalFlush(
    DockSide primary,
    Rect snapped,
    Rect target,
  ) {
    switch (primary) {
      case DockSide.top:
      case DockSide.bottom:
        final leftDist = (snapped.left - target.left).abs();
        final rightDist = (snapped.right - target.right).abs();
        if (leftDist <= snapThreshold && leftDist <= rightDist) {
          return (
            side: DockSide.left,
            delta: Offset(target.left - snapped.left, 0),
          );
        }
        if (rightDist <= snapThreshold) {
          return (
            side: DockSide.right,
            delta: Offset(target.right - snapped.right, 0),
          );
        }
        return null;
      case DockSide.left:
      case DockSide.right:
        final topDist = (snapped.top - target.top).abs();
        final bottomDist = (snapped.bottom - target.bottom).abs();
        if (topDist <= snapThreshold && topDist <= bottomDist) {
          return (
            side: DockSide.top,
            delta: Offset(0, target.top - snapped.top),
          );
        }
        if (bottomDist <= snapThreshold) {
          return (
            side: DockSide.bottom,
            delta: Offset(0, target.bottom - snapped.bottom),
          );
        }
        return null;
    }
  }

  List<_SnapCandidate> _candidates(
    WindowId movingId,
    Rect moving,
    WindowId targetId,
    Rect target,
  ) {
    final out = <_SnapCandidate>[];
    final allowSides = movingId != WindowId.playlist;

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
    if (allowSides) {
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
    }

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
