import 'package:flutter/widgets.dart';

import 'skin_image.dart';

/// A control whose whole face is skin art: a [SkinImage] with a hit target
/// on top, swapping sprites for the pressed and active states.
///
/// Which sprite shows, in priority order:
///   * [pressedAsset] while a pointer is held down (falls back to idle if unset)
///   * [activeAsset] when [active] is true (falls back to idle if unset)
///   * [idleAsset] otherwise
class SkinButton extends StatefulWidget {
  const SkinButton({
    super.key,
    required this.size,
    required this.idleAsset,
    this.pressedAsset,
    this.activeAsset,
    this.active = false,
    this.onPressed,
    this.overlay,
    required this.semanticLabel,
  });

  final Size size;
  final String idleAsset;
  final String? pressedAsset;
  final String? activeAsset;
  final bool active;
  final VoidCallback? onPressed;

  /// Optional widget painted centred on top of the sprite — for glyphs the
  /// skin PNG cannot supply (e.g. the zoom +/- and the mute speaker).
  final Widget? overlay;
  final String semanticLabel;

  bool get isEnabled => onPressed != null;

  @override
  State<SkinButton> createState() => _SkinButtonState();
}

class _SkinButtonState extends State<SkinButton> {
  bool _down = false;

  void _setDown(bool value) {
    // Clearing pressed must always work — e.g. when onPressed becomes null
    // mid-gesture. Only setting pressed is gated on being enabled.
    if (value && !widget.isEnabled) return;
    if (_down == value) return;
    setState(() => _down = value);
  }

  String get _asset {
    if (_down && widget.pressedAsset != null) return widget.pressedAsset!;
    if (widget.active && widget.activeAsset != null) return widget.activeAsset!;
    return widget.idleAsset;
  }

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.semanticLabel,
      child: MouseRegion(
        cursor: widget.isEnabled
            ? SystemMouseCursors.click
            : SystemMouseCursors.basic,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (_) => _setDown(true),
          onTapUp: (_) => _setDown(false),
          onTapCancel: () => _setDown(false),
          onTap: widget.onPressed,
          child: SizedBox.fromSize(
            size: widget.size,
            child: widget.overlay == null
                ? SkinImage(asset: _asset, logicalSize: widget.size)
                : Stack(
                    fit: StackFit.expand,
                    children: [
                      SkinImage(asset: _asset, logicalSize: widget.size),
                      Center(child: widget.overlay!),
                    ],
                  ),
          ),
        ),
      ),
    );
  }
}
