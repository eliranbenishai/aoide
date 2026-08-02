import 'dart:ui' as ui;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';

/// The nine region PNGs that make up the playlist panel's chrome, plus the
/// logical [border] thickness at which the corners and edges are drawn.
///
/// Corners are fixed; the four edges tile along their long axis; the [well]
/// tiles both axes so the list area can grow freely without stretching the
/// grain into a flat smear. All art is authored at 2x (logical = px / 2), so a
/// 24 px corner draws at a 12 logical border.
@immutable
class PlaylistSlices {
  const PlaylistSlices({
    required this.nw,
    required this.n,
    required this.ne,
    required this.w,
    required this.e,
    required this.sw,
    required this.s,
    required this.se,
    required this.well,
    required this.border,
  });

  final String nw;
  final String n;
  final String ne;
  final String w;
  final String e;
  final String sw;
  final String s;
  final String se;
  final String well;
  final EdgeInsets border;

  List<String> get assets => [nw, n, ne, w, e, sw, s, se, well];

  static const _dir = 'assets/skin/graphite/playlist';

  /// The built-in graphite playlist chrome, invented in-family from the
  /// main/EQ face grain (see `.scratch/graphite-skin/build_playlist_slices.py`).
  static const graphite = PlaylistSlices(
    nw: '$_dir/nw.png',
    n: '$_dir/n.png',
    ne: '$_dir/ne.png',
    w: '$_dir/w.png',
    e: '$_dir/e.png',
    sw: '$_dir/sw.png',
    s: '$_dir/s.png',
    se: '$_dir/se.png',
    well: '$_dir/well.png',
    border: EdgeInsets.all(12),
  );
}

/// Paints a nine-slice skin ([slices]) that expands to fill its parent and
/// hosts [child] (the scrolling track list) inside the well, inset by the slice
/// [PlaylistSlices.border].
///
/// The chrome is a bezel around a recessed, grained well: corners stay fixed,
/// edges tile along their length, and the well tiles both ways. Because the
/// main canvas is a separate fixed-size panel in the stack, growing this well
/// never distorts it (Task 5 owns the window sizing).
class NineSliceSkin extends StatefulWidget {
  const NineSliceSkin({
    super.key,
    required this.slices,
    this.child,
    this.size,
  });

  final PlaylistSlices slices;
  final Widget? child;

  /// Optional fixed size; when null the skin expands to its parent.
  final Size? size;

  @override
  State<NineSliceSkin> createState() => _NineSliceSkinState();
}

class _NineSliceSkinState extends State<NineSliceSkin> {
  final Map<String, ui.Image> _images = {};
  final Set<String> _pending = {};
  final List<(ImageStream, ImageStreamListener)> _subscriptions = [];

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _resolve();
  }

  @override
  void didUpdateWidget(NineSliceSkin oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.slices != widget.slices) {
      _clear();
      _resolve();
    }
  }

  void _resolve() {
    final config = createLocalImageConfiguration(context);
    for (final asset in widget.slices.assets) {
      if (_images.containsKey(asset) || _pending.contains(asset)) continue;
      _pending.add(asset);
      final stream = AssetImage(asset).resolve(config);
      void onImage(ImageInfo info, bool _) {
        if (!mounted) return;
        _pending.remove(asset);
        setState(() => _images[asset] = info.image);
      }

      final listener = ImageStreamListener(onImage);
      stream.addListener(listener);
      _subscriptions.add((stream, listener));
    }
  }

  void _clear() {
    for (final (stream, listener) in _subscriptions) {
      stream.removeListener(listener);
    }
    _subscriptions.clear();
    _pending.clear();
    _images.clear();
  }

  @override
  void dispose() {
    for (final (stream, listener) in _subscriptions) {
      stream.removeListener(listener);
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final content = Stack(
      fit: StackFit.expand,
      children: [
        const ColoredBox(color: TrampColors.wellDeep),
        Positioned.fill(
          child: CustomPaint(
            painter: NineSlicePainter(
              slices: widget.slices,
              images: _images,
              loadedCount: _images.length,
            ),
          ),
        ),
        if (widget.child != null)
          Positioned.fill(
            child: Padding(
              padding: widget.slices.border,
              child: widget.child,
            ),
          ),
      ],
    );

    if (widget.size != null) {
      return SizedBox.fromSize(size: widget.size, child: content);
    }
    return content;
  }
}

/// Draws the nine regions. Art is 2x, so every region draws at [_scale].
@visibleForTesting
class NineSlicePainter extends CustomPainter {
  NineSlicePainter({
    required this.slices,
    required this.images,
    required this.loadedCount,
  });

  final PlaylistSlices slices;
  final Map<String, ui.Image> images;

  /// Snapshot of [images.length] at construction; shared [images] mutates in
  /// place so [shouldRepaint] must compare this, not the live map length.
  final int loadedCount;

  static const double _scale = 0.5;

  @override
  void paint(Canvas canvas, Size size) {
    final b = slices.border;
    final l = b.left;
    final t = b.top;
    final r = b.right;
    final bo = b.bottom;
    final innerW = size.width - l - r;
    final innerH = size.height - t - bo;
    if (innerW <= 0 || innerH <= 0) return;

    // Well first, then edges and corners on top of the frame.
    _region(canvas, slices.well, Rect.fromLTWH(l, t, innerW, innerH),
        tileX: true, tileY: true);

    _region(canvas, slices.n, Rect.fromLTWH(l, 0, innerW, t), tileX: true);
    _region(canvas, slices.s,
        Rect.fromLTWH(l, size.height - bo, innerW, bo), tileX: true);
    _region(canvas, slices.w, Rect.fromLTWH(0, t, l, innerH), tileY: true);
    _region(canvas, slices.e,
        Rect.fromLTWH(size.width - r, t, r, innerH), tileY: true);

    _region(canvas, slices.nw, Rect.fromLTWH(0, 0, l, t));
    _region(canvas, slices.ne, Rect.fromLTWH(size.width - r, 0, r, t));
    _region(canvas, slices.sw, Rect.fromLTWH(0, size.height - bo, l, bo));
    _region(canvas, slices.se,
        Rect.fromLTWH(size.width - r, size.height - bo, r, bo));
  }

  /// Draws [asset] into [dst], tiling along the requested axes at [_scale];
  /// non-tiled axes fill [dst] once (corners fill both).
  void _region(
    Canvas canvas,
    String asset,
    Rect dst, {
    bool tileX = false,
    bool tileY = false,
  }) {
    final image = images[asset];
    if (image == null) return;
    final src = Rect.fromLTWH(
      0,
      0,
      image.width.toDouble(),
      image.height.toDouble(),
    );
    final paint = Paint()..filterQuality = FilterQuality.medium;

    final tileW = tileX ? image.width * _scale : dst.width;
    final tileH = tileY ? image.height * _scale : dst.height;

    canvas.save();
    canvas.clipRect(dst);
    for (var y = dst.top; y < dst.bottom; y += tileH) {
      for (var x = dst.left; x < dst.right; x += tileW) {
        canvas.drawImageRect(
          image,
          src,
          Rect.fromLTWH(x, y, tileW, tileH),
          paint,
        );
      }
    }
    canvas.restore();
  }

  @override
  bool shouldRepaint(NineSlicePainter old) =>
      old.slices != slices || old.loadedCount != loadedCount;
}
