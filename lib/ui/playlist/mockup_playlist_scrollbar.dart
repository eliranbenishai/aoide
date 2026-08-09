import 'package:flutter/widgets.dart';

import '../chrome/mockup/mockup_hover.dart';

/// Mockup-faithful playlist scrollbar (`.scrollbar` / `.scrollbar i`).
///
/// 14px pill track with horizontal brushed gradient + ridged thumb.
class MockupPlaylistScrollbar extends StatefulWidget {
  const MockupPlaylistScrollbar({
    super.key,
    required this.controller,
    this.width = 14,
  });

  final ScrollController controller;
  final double width;

  @override
  State<MockupPlaylistScrollbar> createState() =>
      _MockupPlaylistScrollbarState();
}

class _MockupPlaylistScrollbarState extends State<MockupPlaylistScrollbar> {
  double? _dragThumbOffset;

  @override
  void initState() {
    super.initState();
    widget.controller.addListener(_onScroll);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void didUpdateWidget(covariant MockupPlaylistScrollbar oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.controller != widget.controller) {
      oldWidget.controller.removeListener(_onScroll);
      widget.controller.addListener(_onScroll);
    }
  }

  @override
  void dispose() {
    widget.controller.removeListener(_onScroll);
    super.dispose();
  }

  void _onScroll() {
    if (mounted) setState(() {});
  }

  _ScrollbarMetrics? _metrics(double trackHeight) {
    final c = widget.controller;
    if (!c.hasClients || !c.position.hasContentDimensions) return null;
    if (!trackHeight.isFinite || trackHeight <= 0) return null;
    final position = c.position;
    final max = position.maxScrollExtent;
    final viewport = position.viewportDimension;
    final content = viewport + max;
    if (content <= 0) return null;

    final thumbHeight = max <= 0
        ? trackHeight
        : (viewport / content * trackHeight).clamp(28.0, trackHeight);
    // Mockup `.scrollbar i` is a fixed decorative thumb (`top:6%; height:32%`)
    // even when the list barely overflows. Cap the visual thumb so static
    // goldens match; still scale with content when the list is long.
    final mockupThumb = trackHeight * 0.32;
    final visualThumbHeight = max <= 0
        ? mockupThumb
        : thumbHeight > mockupThumb
            ? mockupThumb
            : thumbHeight;
    final travel = trackHeight - visualThumbHeight;
    final thumbTop = max <= 0
        ? trackHeight * 0.06
        : (position.pixels / max * travel).clamp(0.0, travel);
    // When nearly fitting, park near mockup's 6% inset.
    final visualTop = max <= trackHeight * 0.05
        ? trackHeight * 0.06
        : thumbTop;
    return _ScrollbarMetrics(
      thumbTop: visualTop,
      thumbHeight: visualThumbHeight,
      maxScrollExtent: max,
      travel: travel,
    );
  }

  void _jumpToThumbTop(double thumbTop, _ScrollbarMetrics m) {
    if (m.maxScrollExtent <= 0 || m.travel <= 0) return;
    final t = (thumbTop / m.travel).clamp(0.0, 1.0);
    widget.controller.jumpTo(t * m.maxScrollExtent);
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: widget.width,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final trackHeight = constraints.maxHeight;
          if (!trackHeight.isFinite || trackHeight <= 0) {
            return const SizedBox.expand();
          }
          final metrics = _metrics(trackHeight);
          final thumbTop = metrics?.thumbTop ?? 0;
          final thumbHeight = metrics?.thumbHeight ?? trackHeight;

          return MockupHover(
            enabled: metrics != null,
            builder: (context, hover) {
              return GestureDetector(
                behavior: HitTestBehavior.opaque,
                onTapDown: metrics == null
                    ? null
                    : (details) {
                        final localY = details.localPosition.dy;
                        final top = (localY - thumbHeight / 2)
                            .clamp(0.0, trackHeight - thumbHeight);
                        _jumpToThumbTop(top, metrics);
                      },
                onVerticalDragStart: metrics == null
                    ? null
                    : (details) {
                        _dragThumbOffset = details.localPosition.dy - thumbTop;
                      },
                onVerticalDragUpdate: metrics == null
                    ? null
                    : (details) {
                        final offset = _dragThumbOffset ?? thumbHeight / 2;
                        final top = (details.localPosition.dy - offset)
                            .clamp(0.0, trackHeight - thumbHeight);
                        _jumpToThumbTop(top, metrics);
                      },
                onVerticalDragEnd: (_) => _dragThumbOffset = null,
                onVerticalDragCancel: () => _dragThumbOffset = null,
                child: CustomPaint(
                  painter: MockupPlaylistScrollbarPainter(
                    thumbTop: thumbTop,
                    thumbHeight: thumbHeight,
                    hover: hover,
                  ),
                  child: const SizedBox.expand(),
                ),
              );
            },
          );
        },
      ),
    );
  }
}

class _ScrollbarMetrics {
  const _ScrollbarMetrics({
    required this.thumbTop,
    required this.thumbHeight,
    required this.maxScrollExtent,
    required this.travel,
  });

  final double thumbTop;
  final double thumbHeight;
  final double maxScrollExtent;
  final double travel;
}

/// Paints track + ridged brushed thumb to match `player-mockup-2.html`.
class MockupPlaylistScrollbarPainter extends CustomPainter {
  const MockupPlaylistScrollbarPainter({
    required this.thumbTop,
    required this.thumbHeight,
    this.hover = 0,
  });

  final double thumbTop;
  final double thumbHeight;
  final double hover;

  @override
  void paint(Canvas canvas, Size size) {
    final trackRRect = RRect.fromRectAndRadius(
      Offset.zero & size,
      Radius.circular(size.width),
    );

    // Track fill: linear-gradient(90deg, #05060a, #12151c 60%, #1e222c)
    final trackPaint = Paint()
      ..shader = const LinearGradient(
        begin: Alignment.centerLeft,
        end: Alignment.centerRight,
        colors: [
          Color(0xFF05060A),
          Color(0xFF12151C),
          Color(0xFF1E222C),
        ],
        stops: [0, 0.6, 1],
      ).createShader(Offset.zero & size);
    canvas.drawRRect(trackRRect, trackPaint);

    // inset 2px 0 4px rgba(0,0,0,0.95)
    final insetShadow = Paint()
      ..shader = const LinearGradient(
        begin: Alignment.centerLeft,
        end: Alignment.centerRight,
        colors: [
          Color(0xF2000000),
          Color(0x00000000),
        ],
        stops: [0, 0.55],
      ).createShader(Offset.zero & size);
    canvas.drawRRect(trackRRect, insetShadow);

    // Edge ticks: inset -1px / 1px 0 0 rgba(226,236,255,0.1)
    final edge = Paint()
      ..color = const Color(0x1AE2ECFF)
      ..strokeWidth = 1
      ..style = PaintingStyle.stroke;
    canvas.drawLine(
      Offset(size.width - 0.5, 1),
      Offset(size.width - 0.5, size.height - 1),
      edge,
    );

    if (thumbHeight <= 0 || !thumbHeight.isFinite) return;

    // Thumb inset 1px left/right (`.scrollbar i`)
    final thumbRect = Rect.fromLTWH(
      1,
      thumbTop,
      size.width - 2,
      thumbHeight,
    );
    final thumbRRect = RRect.fromRectAndRadius(
      thumbRect,
      Radius.circular(thumbRect.width),
    );

    if (hover > 0.001) {
      canvas.drawRRect(
        thumbRRect.inflate(1),
        Paint()
          ..color = Color.fromRGBO(61, 231, 255, 0.3 * hover)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2.5),
      );
    }

    final thumbPaint = Paint()
      ..shader = LinearGradient(
        begin: Alignment.centerLeft,
        end: Alignment.centerRight,
        colors: [
          mockupHoverLift(const Color(0xFF7D8496), hover),
          mockupHoverLift(const Color(0xFF474E5C), hover),
          mockupHoverLift(const Color(0xFF22262F), hover),
        ],
        stops: const [0, 0.52, 1],
      ).createShader(thumbRect);
    canvas.drawRRect(thumbRRect, thumbPaint);

    // inset 0 1px 0 rgba(236,244,255,0.5)
    canvas.drawRRect(
      thumbRRect,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0x80ECF4FF),
            Color(0x00ECF4FF),
          ],
          stops: [0, 0.35],
        ).createShader(thumbRect),
    );

    // Ridged brush band (`.scrollbar i::after`) — 8px centered stripes
    const ridgeHeight = 8.0;
    final ridgeTop = thumbRect.center.dy - ridgeHeight / 2;
    final ridgeRect = Rect.fromLTRB(
      thumbRect.left + 3,
      ridgeTop,
      thumbRect.right - 3,
      ridgeTop + ridgeHeight,
    );
    canvas.save();
    canvas.clipRRect(
      RRect.fromRectAndRadius(ridgeRect, const Radius.circular(1)),
    );
    final ridgeLight = Color.lerp(
      const Color(0x3DE2ECFF),
      const Color(0xA63DE7FF),
      hover,
    )!;
    for (var y = ridgeRect.top; y < ridgeRect.bottom; y += 2) {
      canvas.drawRect(
        Rect.fromLTWH(ridgeRect.left, y, ridgeRect.width, 1),
        Paint()..color = const Color(0x80000000),
      );
      canvas.drawRect(
        Rect.fromLTWH(ridgeRect.left, y + 1, ridgeRect.width, 1),
        Paint()..color = ridgeLight,
      );
    }
    canvas.restore();
  }

  @override
  bool shouldRepaint(covariant MockupPlaylistScrollbarPainter oldDelegate) {
    return oldDelegate.thumbTop != thumbTop ||
        oldDelegate.thumbHeight != thumbHeight ||
        oldDelegate.hover != hover;
  }
}
