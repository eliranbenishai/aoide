import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

/// Edge hit-strips that start a native resize on **this** window's HWND.
///
/// [window_manager]'s [DragToResizeArea] always calls
/// [windowManager.startResizing], which is the main player when extras live
/// on the same isolate.
class OsDragToResizeArea extends StatelessWidget {
  const OsDragToResizeArea({
    super.key,
    required this.child,
    required this.startResizing,
    this.resizeEdgeSize = 8,
    this.enableResizeEdges,
  });

  final Widget child;
  final Future<void> Function(ResizeEdge edge) startResizing;
  final double resizeEdgeSize;
  final List<ResizeEdge>? enableResizeEdges;

  Widget _edge(
    ResizeEdge resizeEdge, {
    MouseCursor cursor = SystemMouseCursors.basic,
    double? width,
    double? height,
  }) {
    if (enableResizeEdges != null && !enableResizeEdges!.contains(resizeEdge)) {
      return const SizedBox.shrink();
    }
    return MouseRegion(
      cursor: cursor,
      child: GestureDetector(
        onPanStart: (_) => startResizing(resizeEdge),
        child: SizedBox(width: width, height: height),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        child,
        Positioned.fill(
          child: Column(
            children: [
              Row(
                children: [
                  _edge(
                    ResizeEdge.topLeft,
                    cursor: SystemMouseCursors.resizeUpLeft,
                    width: resizeEdgeSize,
                    height: resizeEdgeSize,
                  ),
                  Expanded(
                    child: _edge(
                      ResizeEdge.top,
                      cursor: SystemMouseCursors.resizeUp,
                      height: resizeEdgeSize,
                    ),
                  ),
                  _edge(
                    ResizeEdge.topRight,
                    cursor: SystemMouseCursors.resizeUpRight,
                    width: resizeEdgeSize,
                    height: resizeEdgeSize,
                  ),
                ],
              ),
              Expanded(
                child: Row(
                  children: [
                    _edge(
                      ResizeEdge.left,
                      cursor: SystemMouseCursors.resizeLeft,
                      width: resizeEdgeSize,
                      height: double.infinity,
                    ),
                    const Expanded(child: SizedBox.shrink()),
                    _edge(
                      ResizeEdge.right,
                      cursor: SystemMouseCursors.resizeRight,
                      width: resizeEdgeSize,
                      height: double.infinity,
                    ),
                  ],
                ),
              ),
              Row(
                children: [
                  _edge(
                    ResizeEdge.bottomLeft,
                    cursor: SystemMouseCursors.resizeDownLeft,
                    width: resizeEdgeSize,
                    height: resizeEdgeSize,
                  ),
                  Expanded(
                    child: _edge(
                      ResizeEdge.bottom,
                      cursor: SystemMouseCursors.resizeDown,
                      height: resizeEdgeSize,
                    ),
                  ),
                  _edge(
                    ResizeEdge.bottomRight,
                    cursor: SystemMouseCursors.resizeDownRight,
                    width: resizeEdgeSize,
                    height: resizeEdgeSize,
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }
}
