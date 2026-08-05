import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';
import 'metal_panel.dart';

/// A raised graphite label button (painted surface).
///
/// Playlist toolbar LOAD/SAVE/ADD now use [SkinButton] sprites. Keep this for
/// any remaining coded label controls and for unit tests of the surface recipe.
class ChromeButton extends StatefulWidget {
  const ChromeButton({
    super.key,
    required this.text,
    required this.onPressed,
    this.size,
  });

  final String text;
  final VoidCallback? onPressed;

  /// When set, this size IS the constraint — no padding is added on top, so
  /// the label gets the full width (see the pre-skin AUTO button, which
  /// overflowed once horizontal padding ate into a tight 37px).
  final Size? size;

  bool get isEnabled => onPressed != null;

  @override
  State<ChromeButton> createState() => _ChromeButtonState();
}

class _ChromeButtonState extends State<ChromeButton> {
  bool _down = false;

  void _setDown(bool value) {
    // Clearing pressed state must always work — e.g. when onPressed becomes
    // null mid-gesture. Only setting pressed is gated on being enabled.
    if (value && !widget.isEnabled) return;
    if (_down == value) return;
    setState(() => _down = value);
  }

  @override
  Widget build(BuildContext context) {
    final colour =
        widget.isEnabled ? TrampColors.label : TrampColors.labelDim;

    Widget button = MetalPanel(
      surface: _down ? TrampSurface.pressedButton : TrampSurface.raisedButton,
      padding: widget.size == null
          ? const EdgeInsets.symmetric(horizontal: 7)
          : null,
      child: Center(
        child: Text(
          widget.text,
          style: TrampText.chromeLabel.copyWith(color: colour),
        ),
      ),
    );

    if (widget.size != null) {
      button = SizedBox(
        width: widget.size!.width,
        height: widget.size!.height,
        child: button,
      );
    }

    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.text,
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
          child: button,
        ),
      ),
    );
  }
}
